#include "Init/KrInitSMP.h"

#include "Init/KrApplicationProcessorStart.h"

#include "Core/Krnlmeltdown.h"
#include "Core/KernelState.h"
#include "Core/Assert.h"

#include "CPU/PerCpu.h"
#include "CPU/Stall.h"
#include "CPU/APIC.h"
#include "CPU/Halt.h"
#include "CPU/GDT.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

#include "KRTL/Krnlmem.h"
#include "Intmgmt/NMI.h"

#include "Memory/Virtmemmgmt.h"
#include "Memory/Krnlheap.h"

#define PER_CORE_STACK_SIZE (64 * 1024) // 64 KiB

#define KR_AP_START_ADDRESS     0x8000
#define KR_AP_BOOT_INFO_ADDRESS 0x7000 // Must match what BootstrapAP.s expects

typedef struct KR_PACKED
{
    QWORD CommonEntryPointAddress;
    QWORD InitialStacksAddress;
    DWORD PaddrKernelCR3;
     WORD StackBase;
     WORD KernelCodeSegmentSelector;
     WORD KernelDataSegmentSelector;
     WORD GdtrAddress; // MUST BE LAST MEMBER! Append above!
     // The reason GdtrAddress must be last member is that SMP init code copies kernel's segment descriptors after it
     // A copy is made because the actual g_KernelState.StateGDT.AddrDescriptorTable lives in high half where DWORD ptr cannot address so our APs cannot use them
} ApBootInfo;
static UINTPTR sg_InitialStacks[MAX_SMP_PROCESSORS] = {0};

extern char __KR_LINK_APBOOTSTRAP_START[];
extern char __KR_LINK_APBOOTSTRAP_END[];

VOID KrInitSMP(VOID)
{
    // The Bootstrap Processor is currently active
    g_KernelState.SmpInfo.ActiveProcessorCount = 1;
    g_KernelState.SmpInfo.ProcInfo[KrThisCpuID()].RFD = TRUE;

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
    pApBootInfo->CommonEntryPointAddress = (UINTPTR) KrApplicationProcessorStart;
    pApBootInfo->InitialStacksAddress = (UINTPTR) sg_InitialStacks;
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

    for (UINT i = 0; i < MAX_SMP_PROCESSORS; i++)
    {
        if (g_KernelState.SmpInfo.ProcInfo[i].E_V && !g_KernelState.SmpInfo.ProcInfo[i].BSP)
        {
            UINTPTR VaddrStackBottom = (UINTPTR) HeapAlignedAcquire(PER_CORE_STACK_SIZE, 8);
            RUNTIME_ENSURE(VaddrStackBottom,
                KR_MDCODE_AP_STACK_ALLOC_FAILURE,
                "Could not acquire a heap for AP `%u`.\n",
                i
            );
            
            sg_InitialStacks[i] = VaddrStackBottom + PER_CORE_STACK_SIZE;
            // Touch it a little bit so it is demand-paged in
            // Because the AP might not have gotten its per CPU yet so touching the stack is unsafe
            // This access will forcefully map the first 4096 bytes so AP can easily acquire its per-cpu struct
            *(volatile BYTE*)(sg_InitialStacks[i] - 1) = 0;
        }
    }

    KrIpiConfig IpiConfig = {0};
    IpiConfig.eDeliveryMode = IPI_DELIVERY_INIT;
    IpiConfig.eDestMode = IPI_DESTINATION_PHYSICAL;
    IpiConfig.eLevel = IPI_LEVEL_ASSERT;
    IpiConfig.eTrigMode = IPI_TRIGGER_EDGE;
    IpiConfig.eDestShorthand = IPI_SHORTHAND_ALL_XSLF;
    
    ApicIssueIpi(&IpiConfig);
    KrProcessorStall(50000000);

    IpiConfig.IntVector = KR_AP_START_ADDRESS >> 12;
    IpiConfig.eDeliveryMode = IPI_DELIVERY_SIPI;
    IpiConfig.eDestMode = IPI_DESTINATION_PHYSICAL;
    IpiConfig.eLevel = IPI_LEVEL_ASSERT;
    IpiConfig.eTrigMode = IPI_TRIGGER_EDGE;
    IpiConfig.eDestShorthand = IPI_SHORTHAND_ALL_XSLF;

    ApicIssueIpi(&IpiConfig);
    KrProcessorStall(1250000);

    UINT NrTimesAwaitedCPUs = 0;
DoCpuInitCheck:
    if (NrTimesAwaitedCPUs >= 16)
    {
        RUNTIME_ENSURE(FALSE,
            KR_MDCODE_SMP_BRINGUP_FAILURE,
            "SMP initialization failure and timeout. Expected `%u` processors to report for duty, only `%u` did.",
            g_KernelState.SmpInfo.IdealProcessorCount,
            g_KernelState.SmpInfo.ActiveProcessorCount
        ); KR_STATIC_UNREACHABLE();
    }
    if (g_KernelState.SmpInfo.ActiveProcessorCount != g_KernelState.SmpInfo.IdealProcessorCount)
    {
        NrTimesAwaitedCPUs++;
        KrProcessorStall(1250000);
        goto DoCpuInitCheck;
    }

    RUNTIME_ENSURE(!g_KernelState.SmpInfo.bProcessorReinitDetected,
        KR_MDCODE_SMP_BRINGUP_FAILURE,
        "One or more processors reinitialized!");

    g_KernelState.SmpInfo.pCpuIDs = (UINT*) HeapAcquire(g_KernelState.SmpInfo.ActiveProcessorCount * sizeof(UINT));
    for (UINT i = 0, vi = 0; i < MAX_SMP_PROCESSORS && vi < g_KernelState.SmpInfo.ActiveProcessorCount; i++)
    {
        if (g_KernelState.SmpInfo.ProcInfo[i].RFD)
        {
            g_KernelState.SmpInfo.pCpuIDs[vi++] = g_KernelState.SmpInfo.ProcInfo[i].LocalApicID;
        }
    }

    KrdwtpOutFormatText("All `%u` processors have reported for duty.\n", g_KernelState.SmpInfo.ActiveProcessorCount);
    UINT NrAck;
    RaiseGlobalNonmaskableInterrupt(FALSE, NMI_REASON_HARMLESS_TEST, NULLPTR, &NrAck);

    if (NrAck == g_KernelState.SmpInfo.ActiveProcessorCount - 1) // -1 because bInterruptSelf = FALSE for above call to RaiseGlobalNonmaskableInterrupt
    {
        KrdwtpOutFormatText("Sanitychk: %u application processors acknowledged the harmless test NMI.\n", NrAck);
    }
    else
    {
        RUNTIME_ENSURE(FALSE,
            KR_MDCODE_SMP_SANITY_CHECK_FAILURE,
            "Sanitychk: A total of %u application processors did not acknowledge the harmless test NMI!\n",
            g_KernelState.SmpInfo.ActiveProcessorCount - NrAck
        ); KR_STATIC_UNREACHABLE();
    }

    KrdwtpOutColoredText("SMP initialization successful!\n", KRDWTP_COLOR_GREEN, KRDWTP_BACKGROUND);
}
