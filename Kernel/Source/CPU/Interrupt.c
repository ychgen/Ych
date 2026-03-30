#include "CPU/Interrupt.h"
#include "CPU/IDT.h"

#include "Init/KrKernelStart.h"
#include "Memory/Krnlmem.h"

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

void KrUnhandledInterrupt(const KrInterruptFrame* pInterruptFrame)
{
}

void KrInitializeInterruptSystem(void)
{
    // Zero array
    KrContiguousZeroBuffer(g_pInterruptHandlers, KR_NUMBER_OF_INTERRUPT_DESCRIPTOR_ENTRIES * sizeof(KrInterruptHandler));
}

void KrDispatchInterrupt(const KrInterruptFrame* pInterruptFrame)
{
    if (pInterruptFrame->InterruptNo >= KR_NUMBER_OF_INTERRUPT_DESCRIPTOR_ENTRIES)
    {
        return;
    }
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

bool KrRegisterInterruptHandler(uint8_t interruptNo, KrInterruptHandler pHandler)
{
    if (g_pInterruptHandlers[interruptNo])
    {
        return false;
    }
    g_pInterruptHandlers[interruptNo] = pHandler;
    return true;
}

bool KrUnregisterInterruptHandler(uint8_t interruptNo)
{
    if (g_pInterruptHandlers[interruptNo])
    {
        g_pInterruptHandlers[interruptNo] = NULL;
        return true;
    }
    return false;
}
