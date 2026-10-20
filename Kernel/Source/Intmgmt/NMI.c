#include "Intmgmt/NMI.h"

#include "Core/KernelState.h"
#include "Core/Assert.h"

#include "CPU/Spinlock.h"
#include "CPU/PerCpu.h"
#include "CPU/Stall.h"
#include "CPU/APIC.h"
#include "CPU/Halt.h"

#include "KRTL/Intworks.h"
#include "KRTL/Krnlmem.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

static GlobalNmiInfo sg_NmiInfo = {0};
static Spinlock sg_NmiSpinlock = {0};
static UCHAR sg_ExpectAck[MAX_SMP_PROCESSORS / 8] = {0};

static VOID ThisCpuAcknowledgeNMI(VOID);

VOID RaiseGlobalNonmaskableInterrupt(BOOL bInterruptSelf, GlobalNmiReason Reason, VOID* pData, UINT* pOutNrCPUsAcknowledged)
{
    AcquireOrWaitSpinlock(&sg_NmiSpinlock);

    static BOOL sl_InitExpectAck = FALSE;
    if (!sl_InitExpectAck)
    {
        for (UINT i = 0; i < g_KernelState.SmpInfo.ActiveProcessorCount; i++)
        {
            DWORD ID = g_KernelState.SmpInfo.pCpuIDs[i];
            UINT Idx = ID / (MAX_SMP_PROCESSORS / 8);
            UINT Off = ID % (MAX_SMP_PROCESSORS / 8);

            sg_ExpectAck[Idx] |= (1 << Off);
        }
        sl_InitExpectAck = TRUE;
    }

    switch (Reason)
    {
    case NMI_REASON_MELTDOWN:
    {
        RUNTIME_ENSURE(pData != NULLPTR,
            KR_MDCODE_NMI_NO_DATA_PROVIDED,
            "RaiseGlobalNonmaskableInterrupt was called by CPU %u with the reason ID '%u' that mandates a valid pData, but NULLPTR was provided.",
            KrThisCpuID(), Reason);
        sg_NmiInfo.Meltdown = *(NmiMeltdownData*) pData;
        break;
    }
    case NMI_REASON_HARMLESS_TEST:
    {
        break;
    }
    default:
    {
        RUNTIME_ENSURE(FALSE,
            KR_MDCODE_BOGUS_NMI_REASON,
            "RaiseGlobalNonmaskableInterrupt was called by CPU %u with the unknown reason ID '%u'.",
            KrThisCpuID(), Reason
        ); KR_STATIC_UNREACHABLE();
    }
    }
    
    sg_NmiInfo.Reason = Reason;
    sg_NmiInfo.InvokingCPU = KrThisCpuID();
    // self-acknowledge, we do this so this cpu doesn't wait for ack from itself that won't come when bInterruptSelf = FALSE
    {
        UINT Idx = sg_NmiInfo.InvokingCPU / (MAX_SMP_PROCESSORS / 8);
        UINT Off = sg_NmiInfo.InvokingCPU % (MAX_SMP_PROCESSORS / 8);

        sg_NmiInfo.Ack[Idx] |= (1 << Off);
    }

    KrIpiConfig IpiCfg = {
        .dwDestApic = 0, // Irrelevant because we use dst shorthand
        .IntVector = KR_INTERRUPT_VECTOR_NON_MASKABLE_INTERRUPT,
        .eDeliveryMode = IPI_DELIVERY_NMI,
        .eDestMode = 0, // irrelevant
        .eDestShorthand = bInterruptSelf ? IPI_SHORTHAND_ALL_ISLF : IPI_SHORTHAND_ALL_XSLF,
        .eLevel = IPI_LEVEL_ASSERT,
        .eTrigMode = IPI_TRIGGER_EDGE
    };

    // Issue + wait ack ("I'm done" signal) from each CPU
    ApicIssueIpi(&IpiCfg);
    for (UINT i = 0; i < 512; i++)
    {
        if (KrtlBufferEqual(sg_NmiInfo.Ack, sg_ExpectAck, MAX_SMP_PROCESSORS / 8))
        {
            break;
        }
        KrProcessorStall(1250000);
    }
    if (pOutNrCPUsAcknowledged)
    {
        *pOutNrCPUsAcknowledged = 0;
        for (UINT i = 0; i < MAX_SMP_PROCESSORS / 8; i++)
        {
            *pOutNrCPUsAcknowledged += PopulationCount32((UINT) sg_NmiInfo.Ack[i]);
        }
        if (!bInterruptSelf)
        {
            *pOutNrCPUsAcknowledged = *pOutNrCPUsAcknowledged - 1;
        }
    }

    // ALWAYS clear NMI info after it's done!!!!
    KrtlContiguousZeroBuffer(&sg_NmiInfo, sizeof(GlobalNmiInfo));

    RelinquishSpinlock(&sg_NmiSpinlock);
}

VOID IntmgmtNonmaskableInterrupt(const KrInterruptFrame* pIntfr)
{
    switch (sg_NmiInfo.Reason)
    {
    case NMI_REASON_NONE: // Not raised by kernel, skip, and most importantly do not acknowledge
    {
        return; // return, don't break
    }
    // HARD STOP
    case NMI_REASON_MELTDOWN:
    {
        KrDisableInterrupts();

        // we have to ack here as we won't break out of the switch statement
        ThisCpuAcknowledgeNMI();

        KrProcessorHalt(); // indefinite halt
    }
    case NMI_REASON_HARMLESS_TEST:
    {
        break;
    }
    default:
    {
        RUNTIME_ENSURE(FALSE,
            KR_MDCODE_BOGUS_NMI_REASON,
            "IntmgmtNonmaskableInterrupt was received by CPU %u with the unknown reason ID '%u'.",
            KrThisCpuID(), sg_NmiInfo.Reason
        ); KR_STATIC_UNREACHABLE();
    }
    }

    ThisCpuAcknowledgeNMI();
    return;
}

GlobalNmiInfo* GetGlobalNmiInfo(VOID)
{
    return &sg_NmiInfo;
}

static VOID ThisCpuAcknowledgeNMI(VOID)
{
    static Spinlock sl_AckSpinlock = {0};

    UINT Idx = KrThisCpuID() / (MAX_SMP_PROCESSORS / 8);
    UINT Off = KrThisCpuID() % (MAX_SMP_PROCESSORS / 8);

    AcquireOrWaitSpinlock(&sl_AckSpinlock);
    {
        sg_NmiInfo.Ack[Idx] |= (1 << Off);
    }
    RelinquishSpinlock(&sl_AckSpinlock);
}
