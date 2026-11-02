#include "CPU/Interrupt.h"

#include "CPU/PerCpu.h"
#include "CPU/Halt.h"
#include "CPU/APIC.h"
#include "CPU/IDT.h"

#include "Core/KernelState.h"
#include "KRTL/Krnlmem.h"

#include "Intmgmt/Spurious.h"

KrProcessorSnapshot InterruptFrameToProcessorSnapshot(CONST InterruptFrame* pInterruptFrame)
{
#define _fcpy(x) .x = pInterruptFrame->x
    KrProcessorSnapshot snapshot = {
        _fcpy(R15), _fcpy(R14), _fcpy(R13), _fcpy(R12), _fcpy(R11), _fcpy(R10), _fcpy(R9), _fcpy(R8),
        _fcpy(RDI), _fcpy(RSI), _fcpy(RSP), _fcpy(RBP), _fcpy(RDX), _fcpy(RCX), _fcpy(RBX), _fcpy(RAX), 
        _fcpy(RFLAGS), _fcpy(RIP)
    };
#undef _fcpy
    return snapshot;
}

STATIC VOID KrUnhandledInterrupt(CONST InterruptFrame* pInterruptFrame)
{
    // First 32 are reserved by CPU
    // KrInitInt normally registers these to KrCriticalProcessorInterrupt, but just in case, HLT.
    // Exclude int $3, it is breakpoint, also normally handled by KrBreakpointInterrupt.
    if (pInterruptFrame->InterruptNo < KR_PROCESSOR_RESERVED_INTERRUPT_COUNT || pInterruptFrame->InterruptNo == 3)
    {
        ProcessorIndefiniteHalt();
    }
    // After this point, we are sure this is IRS >31, so a non-CPU reserved IRQ.
}

VOID KrDispatchInterrupt(CONST InterruptFrame* pInterruptFrame)
{
    static uint16_t cnt = 0;
    if (pInterruptFrame->InterruptNo < KR_NUMBER_OF_INTERRUPT_DESCRIPTOR_ENTRIES)
    {
        VOID (*Handler)(CONST InterruptFrame*) = GetThisCpu()->Interrupt[pInterruptFrame->InterruptNo].Handler;
        if (Handler)
        {
            Handler(pInterruptFrame);
        }
        else
        {
            KrUnhandledInterrupt(pInterruptFrame);
        }
    }

    // Is this an IRQ? If so, since handler's been called, send an EOI.
    if (pInterruptFrame->InterruptNo >= KR_PROCESSOR_RESERVED_INTERRUPT_COUNT && pInterruptFrame->InterruptNo != KERNEL_SPURIOUS_INTERRUPT_VECTOR)
    {
        // Send EOI signal to APIC, if we don't, it will go radio silent.
        ApicIssueEndOfInt();
    }
}
