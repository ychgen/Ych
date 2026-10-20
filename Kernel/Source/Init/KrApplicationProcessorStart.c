#include "Init/KrApplicationProcessorStart.h"

#include "Core/KernelState.h"
#include "CPU/Interrupt.h"
#include "CPU/Atomic.h"
#include "CPU/PerCpu.h"
#include "CPU/APIC.h"
#include "CPU/Halt.h"
#include "CPU/GDT.h"
#include "CPU/IDT.h"

#include "Memory/Virtmemmgmt.h"
#include "Memory/Krnlheap.h"

#include "KRTL/Krnlstring.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

static UCHAR sg_ProcessorEntryBlacklist[MAX_SMP_PROCESSORS] = {0};

VOID KrApplicationProcessorStart(VOID)
{
    // Make sure the per-cpu is loaded
    KrGrabThisCpuStruct();

    if (sg_ProcessorEntryBlacklist[KrThisCpuID()])
    {
        g_KernelState.SmpInfo.bProcessorReinitDetected = TRUE;
        KrProcessorHalt();
    }
    sg_ProcessorEntryBlacklist[KrThisCpuID()] = TRUE; // Make sure this CPU cannot be re-entrant
    
    // Load GDT
    {
        KrGlobalDescriptorTableRegister GDTR;
        GDTR.Base = g_KernelState.StateGDT.AddrDescriptorTable;
        GDTR.Limit = g_KernelState.StateGDT.NumberOfDescriptorEntries * GDT_SEGMENT_DESCRIPTOR_ENTRY_SIZE - 1;       
        KrLoadGlobalDescriptorTable(&GDTR, g_KernelState.StateGDT.KernelCodeSegmentSelector, g_KernelState.StateGDT.KernelDataSegmentSelector);
        KrGrabThisCpuStruct(); // Must be called ONCE AGAIN because LGDT invalidates GS base!
    }
    // Load IDT
    {
        KrInterruptDescriptorTableRegister IDTR;
        IDTR.Base = g_KernelState.AddrIDT;
        IDTR.Limit = KR_INTERRUPT_DESCRIPTOR_TABLE_SIZE - 1;
        KrLoadInterruptDescriptorTable(&IDTR);
        KrEnableInterrupts();
    }

    if (!KrApicInit(FALSE))
    {
        KrProcessorHalt();
    }

    AtomicExchangeAndAdd32(&g_KernelState.SmpInfo.ActiveProcessorCount, 1);
    g_KernelState.SmpInfo.ProcInfo[KrThisCpuID()].RFD = TRUE; // Reported for duty!

    KrProcessorHalt();
}
