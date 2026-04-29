#ifndef YCH_KERNEL_CPU_INTERRUPT_H
#define YCH_KERNEL_CPU_INTERRUPT_H

#include "CPU/ProcessorSnapshot.h"

#define KR_PROCESSOR_RESERVED_INTERRUPT_COUNT           32
#define KR_INTERRUPTNO_DIVIDE_BY_ZERO                   0
#define KR_INTERRUPTNO_DEBUG_EXCEPTION                  1
#define KR_INTERRUPTNO_NON_MASKABLE_INTERRUPT           2
#define KR_INTERRUPTNO_BREAKPOINT                       3
#define KR_INTERRUPTNO_OVERFLOW                         4
#define KR_INTERRUPTNO_BOUND_RANGE_EXCEEDED             5
#define KR_INTERRUPTNO_INVALID_OPCODE                   6
#define KR_INTERRUPTNO_DEVICE_NOT_AVAILABLE             7
#define KR_INTERRUPTNO_DOUBLE_FAULT                     8
#define KR_INTERRUPTNO_COPROCESSOR_SEGMENT_OVERRUN      9
#define KR_INTERRUPTNO_INVALID_TSS                      10
#define KR_INTERRUPTNO_SEGMENT_NOT_PRESENT              11
#define KR_INTERRUPTNO_STACK_SEGMENT_FAULT              12
#define KR_INTERRUPTNO_GENERAL_PROTECTION_FAULT         13
#define KR_INTERRUPTNO_PAGE_FAULT                       14
//                                             reserved 15
#define KR_INTERRUPTNO_X87_FPU_FLOATING_POINT_ERROR     16
#define KR_INTERRUPTNO_ALIGNMENT_CHECK                  17
#define KR_INTERRUPTNO_MACHINE_CHECK                    18
#define KR_INTERRUPTNO_SIMD_FLOATING_POINT_EXCEPTION    19
#define KR_INTERRUPTNO_VIRTUALIZATION_EXCEPTION         20
#define KR_INTERRUPTNO_CONTROL_PROTECTION_EXCEPTION     21

// Disables maskable interrupts.
#define KrDisableInterrupts() __asm__ __volatile__("cli\n\t")
// Enables interrupts.
#define KrEnableInterrupts()  __asm__ __volatile__("sti\n\t")

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
} KrInterruptFrame;
KrProcessorSnapshot KrInterruptFrameToProcessorSnapshot(const KrInterruptFrame* pInterruptFrame);

typedef VOID(*KrInterruptHandler)(const KrInterruptFrame* pInterruptFrame);

VOID KrInitializeInterruptSystem(VOID);

// Called by ISRs after setting up the InterruptFrame.
VOID KrDispatchInterrupt(const KrInterruptFrame* pInterruptFrame);

/**
 * @brief Registers an interrupt handler for a specific interrupt vector.
 * 
 * @param IntNo The interrupt vector that the handler will handle.
 * @param pHandler Pointer to the handler function.
 * @param bOverwrite If TRUE, if any handler is registered already, it will be overwritten. If FALSE and a handler is registered, the function will fail.
 * @return TRUE if registered, FALSE if already registered (and bOverwrite is FALSE) or registration failure.
 */
BOOL KrRegisterInterruptHandler(ULONG IntNo, KrInterruptHandler pHandler, BOOL bOverwrite);

// USE CAREFULLY!
BOOL KrUnregisterInterruptHandler(ULONG IntNo);

#endif // !YCH_KERNEL_CPU_INTERRUPT_H
