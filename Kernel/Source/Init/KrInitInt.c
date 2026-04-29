#include "Init/KrInitInt.h"
#include "ISRs.h"

#include "CPU/Interrupt.h"
#include "CPU/Halt.h"
#include "CPU/IDT.h"

#include "Core/KernelState.h"
#include "Core/Krnlmeltdown.h"

#include "KRTL/Krnlstring.h"
#include "KRTL/Krnlmem.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

KR_ALIGNED(16) KrInterruptDescriptor g_krInterruptDescriptorTable[KR_NUMBER_OF_INTERRUPT_DESCRIPTOR_ENTRIES];

static CSTR g_pCriticalProcessorExceptionNames[KR_PROCESSOR_RESERVED_INTERRUPT_COUNT] =
{
    "Divide-by-Zero",
    "Debug Exception",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "BOUND Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 FPU Floating-Point Error",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    // complete up to 32, these are reserved for possible future use by architecture
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"
};

VOID KrCriticalProcessorInterrupt(const KrInterruptFrame* pInterruptFrame);
VOID KrBreakpointInterrupt(const KrInterruptFrame* pInterruptFrame);

VOID KrInitInt(VOID)
{
    EncodeAllISRs(); // 0 to 2 and 4 to 255
    KrEncodeInterruptDescriptor(g_krInterruptDescriptorTable + 3, (uint64_t) KrInterruptServiceRoutine_3, g_KernelState.StateGDT.KernelCodeSegmentSelector, 0, KR_GATE_TYPE_TRAP, 0);

    KrInterruptDescriptorTableRegister IDTR =
    {
        .Limit = KR_INTERRUPT_DESCRIPTOR_TABLE_SIZE - 1,
        .Base  = (uint64_t) g_krInterruptDescriptorTable
    };

    KrLoadInterruptDescriptorTable(&IDTR); // LIDT
    KrEnableInterrupts(); // STI

    // Update kernel state
    g_KernelState.AddrIDT = (uintptr_t) g_krInterruptDescriptorTable;

    KrInitializeInterruptSystem();
    KrRegisterInterruptHandler(KR_INTERRUPTNO_BREAKPOINT, KrBreakpointInterrupt, FALSE);

    // Interrupts 0-31 are reserved by the CPU itself and almost all are critical errors.
    // We register special interrupts above and KrRegisterInterruptHandler will just return false for them,
    // so our original ones are intact.
    for (BYTE i = 0; i < KR_PROCESSOR_RESERVED_INTERRUPT_COUNT; i++)
    {
        KrRegisterInterruptHandler(i, KrCriticalProcessorInterrupt, FALSE);
    }
}

VOID KrCriticalProcessorInterrupt(const KrInterruptFrame* pInterruptFrame)
{
    CHAR  pDescMsg[] = "Unhandled critical processor exception: ";
    CSTR  pDescKod   = g_pCriticalProcessorExceptionNames[pInterruptFrame->InterruptNo];
    
    const SIZE szMsg = sizeof(pDescMsg) - 1;
    const SIZE szKod = KrtlStringSize(pDescKod);
    
    char pDesc[128];
    KrtlContiguousCopyBuffer(pDesc, pDescMsg, szMsg);
    KrtlContiguousCopyBuffer(pDesc + szMsg, pDescKod, szKod+1); // copy null terminator too
    
    // Kernel meltdown (panic)
    KrProcessorSnapshot snapshot = KrInterruptFrameToProcessorSnapshot(pInterruptFrame);
    Krnlmeltdown(KR_MDCODE_CRITICAL_PROCESSOR_EXCEPTION, pDesc, &snapshot);
}

VOID KrBreakpointInterrupt(const KrInterruptFrame* pInterruptFrame)
{
    // TODO: Log
    (VOID)(pInterruptFrame);
}
