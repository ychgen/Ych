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
    // We as the BSP being the Dictator of the Platform currently are about to summon unemployed uncs to make the state great again.

    *(volatile BYTE*) KrPhysToVirt(0x7008) = 0;

    // Copy bootstrap AP trampoline code to low memory
    KrtlContiguousCopyBuffer((VOID*) KrPhysToVirt(KR_AP_START_ADDRESS), __KR_LINK_APBOOTSTRAP_START, __KR_LINK_APBOOTSTRAP_END - __KR_LINK_APBOOTSTRAP_START);

    Krx2ApicIpiConfig IpiConfig = {0};
    IpiConfig.eDeliveryMode = KRX2APIC_IPI_DELIVERY_INIT;
    IpiConfig.eDestMode = KRX2APIC_IPI_DESTINATION_PHYSICAL;
    IpiConfig.eLevel = KRX2APIC_IPI_LEVEL_ASSERT;
    IpiConfig.eTrigMode = KRX2APIC_IPI_TRIGGER_EDGE;
    IpiConfig.eDestShorthand = KRX2APIC_IPI_SHORTHAND_ALL_XSLF;
    
    Krx2ApicIssueIPI(&IpiConfig);
    Stall(50000000); // This guarantees a wait of around 10 ms for a 5 GHz processor.

    IpiConfig.IntVector = KR_AP_START_VECTOR;
    IpiConfig.eDeliveryMode = KRX2APIC_IPI_DELIVERY_SIPI;
    IpiConfig.eDestMode = KRX2APIC_IPI_DESTINATION_PHYSICAL;
    IpiConfig.eLevel = KRX2APIC_IPI_LEVEL_ASSERT;
    IpiConfig.eTrigMode = KRX2APIC_IPI_TRIGGER_EDGE;
    IpiConfig.eDestShorthand = KRX2APIC_IPI_SHORTHAND_ALL_XSLF;

    Krx2ApicIssueIPI(&IpiConfig);
    Stall(1250000);
    Krx2ApicIssueIPI(&IpiConfig);
    Stall(1250000);

    for (int i = 0; i < 8; i++)
    {
        KrdwtpOutCharacter(*((CHAR*) KrPhysToVirt(0x7000 + i)));
    }
    KrdwtpOutCharacter('\n');
    KrdwtpOutColoredText("Value at memory address value change detected!\n", KRDWTP_COLOR_ORANGE, KRDWTP_BACKGROUND);
    KrdwtpOutFormatText("Address 0x7008 was atomically incremented %u times by parties that are not friendly.\n", *(volatile BYTE*) KrPhysToVirt(0x7008));
}
