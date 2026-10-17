#include "Init/KrInitSMP.h"

#include "Init/KrApplicationProcessorStart.h"

#include "Core/Krnlmeltdown.h"
#include "Core/KernelState.h"
#include "Core/Assert.h"

#include "CPU/APIC.h"
#include "CPU/Halt.h"
#include "CPU/GDT.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

#include "KRTL/Krnlmem.h"

#include "Memory/Virtmemmgmt.h"

#define KR_AP_START_ADDRESS     0x8000
#define KR_AP_BOOT_INFO_ADDRESS 0x7000 // Must match what BootstrapAP.s expects

typedef struct
{
    QWORD CommonEntryPointAddress;
    DWORD PaddrKernelCR3;
     WORD StackBase;
     WORD KernelCodeSegmentSelector;
     WORD KernelDataSegmentSelector;
     WORD GdtrAddress; // MUST BE LAST MEMBER! Append above!
     // The reason GdtrAddress must be last member is that SMP init code copies kernel's segment descriptors after it
     // A copy is made because the actual g_KernelState.StateGDT.AddrDescriptorTable lives in high half where DWORD ptr cannot address so our APs cannot use them
} ApBootInfo;

extern char __KR_LINK_APBOOTSTRAP_START[];
extern char __KR_LINK_APBOOTSTRAP_END[];

static QWORD ReadTSC(VOID)
{
    DWORD Lodword;
    DWORD Hidword;
    
    __asm__ __volatile__
    (
        "lfence\n\t"
        "rdtsc\n\t"
        : "=a"(Lodword), "=d"(Hidword)
        :
        : "memory"
    );

    return (((QWORD) Hidword) << 32) | Lodword;
}

static VOID Stall(ULONG ClockCycles)
{
    QWORD InitialTSC = ReadTSC();
    while (ReadTSC() < InitialTSC + ClockCycles)
    {
        KrProcessorPause();
    }
}

VOID KrInitSMP(VOID)
{
    RUNTIME_ENSURE(                                                            // Kernel CR3 must be in lower 4 GiB range for APs to be able to write into CR3
        KrGetKernelAddressSpace()->PaddrRoot <= 0xFFFFFFFF,
        KR_MDCODE_HIGH_CR3_RESIDENT_DEVCHECK,
        "Kernel address space CR3 value (%p) is larger than the 32-bit limit.",
        KrGetKernelAddressSpace()->PaddrRoot
    );

    KrVirtualMemoryRegion* pLowIdMap = VmAcquireRegion( // We need to ID-map lower memory so when APs load CR3, they don't explode
        KrGetKernelAddressSpace(), 0x1000, 0x10000,
        VMR_FLAG_ALLOW_CODE_EXEC | VMR_FLAG_READABLE | VMR_FLAG_STATIC | VMR_FLAG_WRITABLE | VMR_FLAG_NO_DIVISION_CHECK
    );
    RUNTIME_ENSURE(pLowIdMap != NULLPTR,
                   KR_MDCODE_AP_BOOTSTRAP_LOW_ID_MAP_FAIL,
                   "VmAcquireRegion returned NULLPTR.");
    RUNTIME_ENSURE(VmMapStatic(pLowIdMap, 0x1000),
                   KR_MDCODE_AP_BOOTSTRAP_LOW_ID_MAP_FAIL,
                   "VmMapStatic failed.");

    // Copy bootstrap AP trampoline code to low memory
    KrtlContiguousCopyBuffer((VOID*) KrPhysToVirt(KR_AP_START_ADDRESS), __KR_LINK_APBOOTSTRAP_START, __KR_LINK_APBOOTSTRAP_END - __KR_LINK_APBOOTSTRAP_START);

    ApBootInfo* pApBootInfo = (ApBootInfo*) KR_AP_BOOT_INFO_ADDRESS; // we can do this because we id-mapped that, no need for KrPhysToVirt
    pApBootInfo->CommonEntryPointAddress = (QWORD) KrApplicationProcessorStart;
    pApBootInfo->PaddrKernelCR3 = KrGetKernelAddressSpace()->PaddrRoot;
    pApBootInfo->StackBase = 0x6000;
    pApBootInfo->GdtrAddress = 0x1000;
    pApBootInfo->KernelCodeSegmentSelector = g_KernelState.StateGDT.KernelCodeSegmentSelector;
    pApBootInfo->KernelDataSegmentSelector = g_KernelState.StateGDT.KernelDataSegmentSelector;

    KrtlContiguousCopyBuffer(
        (VOID*)((UINTPTR) pApBootInfo->GdtrAddress + GDTR_SIZE_LEGACY),
        (const VOID*) g_KernelState.StateGDT.AddrDescriptorTable,
        g_KernelState.StateGDT.NumberOfDescriptorEntries * GDT_SEGMENT_DESCRIPTOR_ENTRY_SIZE);
    *(WORD*)  (UINTPTR) pApBootInfo->GdtrAddress = g_KernelState.StateGDT.NumberOfDescriptorEntries * GDT_SEGMENT_DESCRIPTOR_ENTRY_SIZE - 1;
    *(DWORD*)((UINTPTR) pApBootInfo->GdtrAddress + sizeof(WORD)) = pApBootInfo->GdtrAddress + GDTR_SIZE_LEGACY;

    KrIpiConfig IpiConfig = {0};
    IpiConfig.eDeliveryMode = IPI_DELIVERY_INIT;
    IpiConfig.eDestMode = IPI_DESTINATION_PHYSICAL;
    IpiConfig.eLevel = IPI_LEVEL_ASSERT;
    IpiConfig.eTrigMode = IPI_TRIGGER_EDGE;
    IpiConfig.eDestShorthand = IPI_SHORTHAND_ALL_XSLF;
    
    ApicIssueIpi(&IpiConfig);
    Stall(50000000); // This guarantees a wait of around ~10 ms for a 5 GHz processor.

    IpiConfig.IntVector = KR_AP_START_ADDRESS >> 12;
    IpiConfig.eDeliveryMode = IPI_DELIVERY_SIPI;
    IpiConfig.eDestMode = IPI_DESTINATION_PHYSICAL;
    IpiConfig.eLevel = IPI_LEVEL_ASSERT;
    IpiConfig.eTrigMode = IPI_TRIGGER_EDGE;
    IpiConfig.eDestShorthand = IPI_SHORTHAND_ALL_XSLF;

    ApicIssueIpi(&IpiConfig);
    Stall(1250000);
    ApicIssueIpi(&IpiConfig);
    Stall(1250000);

    KrdwtpOutCharacter(*(CHAR*) 0x2000);
    KrdwtpOutCharacter(*(CHAR*) 0x2001);
    KrdwtpOutCharacter(*(CHAR*) 0x2002);
    KrdwtpOutCharacter(*(CHAR*) 0x2003);
    KrdwtpOutCharacter('\n');
}
