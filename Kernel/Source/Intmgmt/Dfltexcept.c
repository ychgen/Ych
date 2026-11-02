#include "Intmgmt/Dfltexcept.h"

#include "Core/Krnlmeltdown.h"

#include "KRTL/Krnlstring.h"
#include "KRTL/Krnlmem.h"

STATIC CSTR ProcessorExceptionNames[KR_PROCESSOR_RESERVED_INTERRUPT_COUNT] =
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

VOID IntmgmtProcessorException(CONST InterruptFrame* Intfr)
{
    STATIC CONST CHAR Prefix[] = "Critical processor exception: ";
    CSTR ExceptionName = ProcessorExceptionNames[Intfr->InterruptNo];
    
    CONST UINT PrefixLength = sizeof(Prefix) - 1;
    CONST UINT NameLength   = KrtlStringSize(ExceptionName);
    
    CHAR MdDesc[128];
    KrtlContiguousCopyBuffer(MdDesc, Prefix, PrefixLength);
    KrtlContiguousCopyBuffer(MdDesc + PrefixLength, ExceptionName, NameLength + 1); // copy null terminator too
    
    // Kernel meltdown (panic)
    KrProcessorSnapshot Snapshot = InterruptFrameToProcessorSnapshot(Intfr);
    Krnlmeltdown(KR_MDCODE_CRITICAL_PROCESSOR_EXCEPTION, MdDesc, &Snapshot);
}
