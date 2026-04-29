#include "CPU/Interrupt.h"

#include "CPU/Halt.h"
#include "CPU/APIC.h"
#include "CPU/IDT.h"

#include "Core/KernelState.h"
#include "KRTL/Krnlmem.h"

KrProcessorSnapshot KrInterruptFrameToProcessorSnapshot(const KrInterruptFrame* pInterruptFrame)
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

KrInterruptHandler g_pInterruptHandlers[KR_NUMBER_OF_INTERRUPT_DESCRIPTOR_ENTRIES];

VOID KrUnhandledInterrupt(const KrInterruptFrame* pInterruptFrame)
{
    // First 32 are reserved by CPU
    // KrInitInt normally registers these to KrCriticalProcessorInterrupt, but just in case, HLT.
    // Exclude int $3, it is breakpoint, also normally handled by KrBreakpointInterrupt.
    if (pInterruptFrame->InterruptNo < KR_PROCESSOR_RESERVED_INTERRUPT_COUNT || pInterruptFrame->InterruptNo == 3)
    {
        KrProcessorHalt();
    }
    // After this point, we are sure this is IRS >31, so a non-CPU reserved IRQ.
}

VOID KrInitializeInterruptSystem(VOID)
{
    // Zero array
    KrtlContiguousZeroBuffer(g_pInterruptHandlers, KR_NUMBER_OF_INTERRUPT_DESCRIPTOR_ENTRIES * sizeof(KrInterruptHandler));
}

VOID KrDispatchInterrupt(const KrInterruptFrame* pInterruptFrame)
{
    if (pInterruptFrame->InterruptNo < KR_NUMBER_OF_INTERRUPT_DESCRIPTOR_ENTRIES)
    {
        KrInterruptHandler handler = g_pInterruptHandlers[pInterruptFrame->InterruptNo];
        if (handler)
        {
            handler(pInterruptFrame);
        }
        else
        {
            KrUnhandledInterrupt(pInterruptFrame);
        }
    }

    // Is this an IRQ? If so, since handler's been called, send an EOI.
    if (pInterruptFrame->InterruptNo >= KR_PROCESSOR_RESERVED_INTERRUPT_COUNT)
    {
        // Send EOI signal to APIC, if we don't, it will go radio silent.
        Krx2SignalEndOfInterrupt();
    }
}

BOOL KrRegisterInterruptHandler(ULONG IntNo, KrInterruptHandler pHandler, BOOL bOverwrite)
{
    if (IntNo > 0xFF)
    {
        return FALSE;
    }
    if (g_pInterruptHandlers[IntNo] && !bOverwrite)
    {
        return FALSE;
    }
    g_pInterruptHandlers[IntNo] = pHandler;
    return TRUE;
}

BOOL KrUnregisterInterruptHandler(ULONG IntNo)
{
    if (IntNo > 0xFF)
    {
        return FALSE;
    }
    if (g_pInterruptHandlers[IntNo])
    {
        g_pInterruptHandlers[IntNo] = NULLPTR;
        return TRUE;
    }
    return FALSE;
}
