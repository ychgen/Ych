#include "Init/KrInitInt.h"
#include "ISRs.h"

#include "CPU/Interrupt.h"
#include "CPU/PerCpu.h"
#include "CPU/Halt.h"
#include "CPU/IDT.h"
#include "CPU/CR.h"

#include "Core/KernelState.h"
#include "Core/Krnlmeltdown.h"

#include "KRTL/Krnlstring.h"
#include "KRTL/Krnlmem.h"

#include "Intmgmt/Dfltexcept.h"
#include "Intmgmt/PageFault.h"
#include "Intmgmt/Spurious.h"
#include "Intmgmt/Timer.h"
#include "Intmgmt/NMI.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

// If you are having #PF during early boot (before VMM can reassign #PF handler), set this to TRUE so you can see the error code.
// Otherwise, keep as FALSE. Especially for shipping/distribution compilations!
#define KR_INITINT_USE_DEBUG_PAGE_FAULT FALSE
// Same story but for #GP
#define KR_INITINT_USE_DEBUG_GENERAL_PROTECTION_FAULT FALSE

// The IDT. Every processor shares the IDT, but not handlers, which are tracked within the PerCpu structure
KR_ALIGNED(16) KrInterruptDescriptor IDT[KR_NUMBER_OF_INTERRUPT_DESCRIPTOR_ENTRIES];

// ISR function pointer array (element at i encodes ISR i)
EXTERN VOID (*KrnlISRs[NUM_INT_VECTORS])(VOID);

VOID KrInitInt(VOID)
{
    STATIC BOOL IdtInit = FALSE;
    if (!IdtInit)
    {
        // Punch IDT entries into memory
        for (UINT i = 0; i < NUM_INT_VECTORS; i++)
        {
            KrEncodeInterruptDescriptor(IDT + i, (UINTPTR) KrnlISRs[i], g_KernelState.StateGDT.KernelCodeSegmentSelector, 0, i != 3 ? KR_GATE_TYPE_INTERRUPT : KR_GATE_TYPE_TRAP, 0);
        }
        g_KernelState.AddrIDT = (UINTPTR) IDT;
        IdtInit = TRUE;
    }

    STATIC KrInterruptDescriptorTableRegister IDTR =
    {
        .Limit = KR_INTERRUPT_DESCRIPTOR_TABLE_SIZE - 1,
        .Base  = (UINTPTR) IDT
    };
    KrLoadInterruptDescriptorTable(&IDTR); // LIDT
    ProcessorEnableInterrupts(); // STI

    for (BYTE i = 0; i < KR_PROCESSOR_RESERVED_INTERRUPT_COUNT; i++)
    {
        SetExceptionInterrupt(GetThisCpuID(), i, IntmgmtProcessorException);
    }

    SetExceptionInterrupt(GetThisCpuID(), INT_VECTOR_NON_MASKABLE_INTERRUPT, IntmgmtNonmaskableInterrupt);
    SetExceptionInterrupt(GetThisCpuID(), INT_VECTOR_PAGE_FAULT,             IntmgmtPageFault);
    SetExceptionInterrupt(GetThisCpuID(), KERNEL_TIMER_INTERRUPT_VECTOR,     IntmgmtTimerInterrupt);
    SetExceptionInterrupt(GetThisCpuID(), KERNEL_SPURIOUS_INTERRUPT_VECTOR,  IntmgmtSpuriousInterrupt);
}
