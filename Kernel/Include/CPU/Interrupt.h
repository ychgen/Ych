#ifndef YCH_KERNEL_CPU_INTERRUPT_H
#define YCH_KERNEL_CPU_INTERRUPT_H

#include "CPU/ProcessorSnapshot.h"

#define NUM_INT_VECTORS 256

/** Processor Exceptions */
#define KR_PROCESSOR_RESERVED_INTERRUPT_COUNT                32
#define INT_VECTOR_DIVIDE_BY_ZERO                    0
#define INT_VECTOR_DEBUG_EXCEPTION                   1
#define INT_VECTOR_NON_MASKABLE_INTERRUPT            2
#define INT_VECTOR_BREAKPOINT                        3
#define INT_VECTOR_OVERFLOW                          4
#define INT_VECTOR_BOUND_RANGE_EXCEEDED              5
#define INT_VECTOR_INVALID_OPCODE                    6
#define INT_VECTOR_DEVICE_NOT_AVAILABLE              7
#define INT_VECTOR_DOUBLE_FAULT                      8
#define INT_VECTOR_COPROCESSOR_SEGMENT_OVERRUN       9
#define INT_VECTOR_INVALID_TSS                      10
#define INT_VECTOR_SEGMENT_NOT_PRESENT              11
#define INT_VECTOR_STACK_SEGMENT_FAULT              12
#define INT_VECTOR_GENERAL_PROTECTION_FAULT         13
#define INT_VECTOR_PAGE_FAULT                       14
//                                                  reserved 15
#define INT_VECTOR_X87_FPU_FLOATING_POINT_ERROR     16
#define INT_VECTOR_ALIGNMENT_CHECK                  17
#define INT_VECTOR_MACHINE_CHECK                    18
#define INT_VECTOR_SIMD_FLOATING_POINT_EXCEPTION    19
#define INT_VECTOR_VIRTUALIZATION_EXCEPTION         20
#define INT_VECTOR_CONTROL_PROTECTION_EXCEPTION     21

// Disables maskable interrupts.
#define ProcessorDisableInterrupts() __asm__ __volatile__("cli\n\t")
// Enables interrupts.
#define ProcessorEnableInterrupts()  __asm__ __volatile__("sti\n\t")

typedef struct
{
    QWORD R15, R14, R13, R12, R11, R10, R9, R8;
    QWORD RDI, RSI, RBP, RDX, RCX, RBX, RAX;

    ULONG InterruptNo;
    ULONG ErrorCode;

    QWORD RIP;
    QWORD CS;
    QWORD RFLAGS;
    QWORD RSP;
    QWORD SS;
} InterruptFrame;
KrProcessorSnapshot InterruptFrameToProcessorSnapshot(const InterruptFrame* pInterruptFrame);

// Called by ISRs after setting up the InterruptFrame.
VOID KrDispatchInterrupt(const InterruptFrame* pInterruptFrame);

#endif // !YCH_KERNEL_CPU_INTERRUPT_H
