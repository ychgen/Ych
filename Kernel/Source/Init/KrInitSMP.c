#include "Init/KrInitSMP.h"

#include "Core/Krnlmeltdown.h"
#include "Core/KernelState.h"

#include "CPU/APIC.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

#include "KRTL/Krnlmem.h"

#include "Memory/Virtmemmgmt.h"

#define KR_AP_START_ADDRESS 0x8000
#define KR_AP_START_VECTOR  ((KR_AP_START_ADDRESS) >> 12)

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
        __asm__ __volatile__ ("pause\n\t");
    }
}

VOID KrInitSMP(VOID)
{
    // Copy bootstrap AP trampoline code to low memory
    KrtlContiguousCopyBuffer((VOID*) KrPhysToVirt(KR_AP_START_ADDRESS), __KR_LINK_APBOOTSTRAP_START, __KR_LINK_APBOOTSTRAP_END - __KR_LINK_APBOOTSTRAP_START);

    KrIpiConfig IpiConfig = {0};
    IpiConfig.eDeliveryMode = IPI_DELIVERY_INIT;
    IpiConfig.eDestMode = IPI_DESTINATION_PHYSICAL;
    IpiConfig.eLevel = IPI_LEVEL_ASSERT;
    IpiConfig.eTrigMode = IPI_TRIGGER_EDGE;
    IpiConfig.eDestShorthand = IPI_SHORTHAND_ALL_XSLF;
    
    ApicIssueIpi(&IpiConfig);
    Stall(50000000); // This guarantees a wait of around 10 ms for a 5 GHz processor.

    IpiConfig.IntVector = KR_AP_START_VECTOR;
    IpiConfig.eDeliveryMode = IPI_DELIVERY_SIPI;
    IpiConfig.eDestMode = IPI_DESTINATION_PHYSICAL;
    IpiConfig.eLevel = IPI_LEVEL_ASSERT;
    IpiConfig.eTrigMode = IPI_TRIGGER_EDGE;
    IpiConfig.eDestShorthand = IPI_SHORTHAND_ALL_XSLF;

    ApicIssueIpi(&IpiConfig);
    Stall(1250000);
    ApicIssueIpi(&IpiConfig);
    Stall(1250000);
}
