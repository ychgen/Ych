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
#include "Memory/Virtmemmgmt.h"

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

VOID KrPageFaultInterrupt(const KrInterruptFrame* pInterruptFrame)
{
    QWORD CR2;
    __asm__ __volatile__ ("mov %%cr2, %0" : "=r"(CR2));
    KrdwtpOutFormatText("#PF -> CR2 = 0x%X ; ErrorCode = 0x%X\nRecorded Pages %u\n", CR2, pInterruptFrame->ErrorCode, GetVirtmemmgmtState()->TotalPages);
    KrProcessorHalt();
}

VOID KrGeneralProtectionFaultInterrupt(const KrInterruptFrame* pInterruptFrame)
{
    WORD CS, SS, DS, ES, FS, GS;
    QWORD CR0, CR3, CR4;

    __asm__ __volatile__ ("mov %%cr0, %0" : "=r"(CR0));
    __asm__ __volatile__ ("mov %%cr3, %0" : "=r"(CR3));
    __asm__ __volatile__ ("mov %%cr4, %0" : "=r"(CR4));

    __asm__ __volatile__ ("mov %%cs, %0" : "=r"(CS));
    __asm__ __volatile__ ("mov %%ss, %0" : "=r"(SS));
    __asm__ __volatile__ ("mov %%ds, %0" : "=r"(DS));
    __asm__ __volatile__ ("mov %%es, %0" : "=r"(ES));
    __asm__ __volatile__ ("mov %%fs, %0" : "=r"(FS));
    __asm__ __volatile__ ("mov %%gs, %0" : "=r"(GS));

    KrdwtpOutColoredText("!!!GENERAL PROTECTION FAULT!!!\n", KRDWTP_COLOR_RED, KRDWTP_BACKGROUND);
    KrdwtpOutFormatText
    (
        " -> CR0 = %Ru\n"
        " -> CR3 = %Ru\n"
        " -> CR4 = %Ru\n"
        " ->  CS = %u\n"
        " ->  SS = %u\n"
        " ->  DS = %u\n"
        " ->  ES = %u\n"
        " ->  FS = %u\n"
        " ->  GS = %u\n"
        " ->  EC = %Ru\n",
        CR0, CR3, CR4, CS, SS, DS, ES, FS, GS, pInterruptFrame->ErrorCode
    );
    KrProcessorHalt();
}

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
    KrRegisterInterruptHandler(KR_INTERRUPTNO_BREAKPOINT, KrBreakpointInterrupt);
    KrRegisterInterruptHandler(KR_INTERRUPTNO_GENERAL_PROTECTION_FAULT, KrGeneralProtectionFaultInterrupt);
    KrRegisterInterruptHandler(KR_INTERRUPTNO_PAGE_FAULT, KrPageFaultInterrupt);

    // Interrupts 0-31 are reserved by the CPU itself and almost all are critical errors.
    // We register special interrupts above and KrRegisterInterruptHandler will just return false for them,
    // so our original ones are intact.
    for (int i = 0; i < KR_PROCESSOR_RESERVED_INTERRUPT_COUNT; i++)
    {
        KrRegisterInterruptHandler(i, KrCriticalProcessorInterrupt);
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
