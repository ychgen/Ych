#include "CPU/PerCpu.h"

#include "Core/KernelState.h"

#include "CPU/CPUID.h"
#include "CPU/APIC.h"
#include "CPU/Halt.h"
#include "CPU/MSR.h"

#include "Memory/Krnlheap.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

STATIC PerCpu sg_PerCpu[MAX_SMP_PROCESSORS] = {0};

// Returns the node to insert after
STATIC TimerEvent* FindTimerEventInsertPoint(ULONG NrTicks);

BOOL SetTimer(ULONG uSeconds, VOID (*Callback)(VOID), DWORD dwFlags)
{
    ApicSetTimerCount(0); // Stop APIC counter from ticking

    PerCpu* ThisCpu = GetThisCpu();
    ULONG NrTicks = uSeconds * ThisCpu->TimerInf.CalibrationValue;

    TimerEvent* Event = HeapAcquire(sizeof(TimerEvent));
    if (!Event)
    {
        ApicSetTimerCount(ApicGetTimerInitialCount()); // shady, we should record initcr and set to that instead, currently I dont care as it requires some logic because of race conditions
        return FALSE;
    }
    Event->LastTickCount = NrTicks;
    Event->Callback = Callback;
    Event->dwFlags = dwFlags;

    // Rearrange the linked list & figure out event->next
    {
        TimerEvent* InsertAfter;
        if (InsertAfter = FindTimerEventInsertPoint(NrTicks))
        {
            Event->Next = InsertAfter->Next;
            InsertAfter->Next = Event;
        }
        else // InsertAfter == NULLPTR means to insert as root
        {
            Event->Next = ThisCpu->TimerInf.TimerRoot;
            ThisCpu->TimerInf.TimerRoot = Event;
        }
    }

    // Write to LAPIC Timer the smallest tick value, since its sorted, root always has the smallest value
    // We also at most write DWORD_MAX as that's as much as the register can handle. IntmgmtTimerInterrupt() handles big ones.
    ApicSetTimerCount(MACROMIN(DWORD_MAX, ThisCpu->TimerInf.TimerRoot->LastTickCount));
    
    return TRUE;
}

VOID GrabThisCpuStruct(VOID)
{
    // Read LAPIC ID
    DWORD EAX, EBX, ECX, EDX;
    KrCPUID(KR_CPUID_LEAF_PRC_INF_FEAT_BITS, EAX, EBX, ECX, EDX);

    BYTE ApicID = EBX >> 24;
    PerCpu* pPerCpu = sg_PerCpu + ApicID;
    
    pPerCpu->pSelf = pPerCpu;
    pPerCpu->ID = ApicID;

    KrWriteMSR(IA32_GS_BASE, (UINTPTR) pPerCpu); // write active base
    KrWriteMSR(IA32_KERNEL_GS_BASE, (UINTPTR) pPerCpu); // write inactive base
}

PerCpu* GetThisCpu(VOID)
{
    PerCpu* pStruct;
    __asm__ __volatile__ ("movq %%gs:0x0, %0\n\t" : "=r"(pStruct));
    return pStruct;
}

DWORD GetThisCpuID(VOID)
{
    return GetThisCpu()->ID;
}

STATIC TimerEvent* FindTimerEventInsertPoint(ULONG NrTicks)
{
    TimerEvent* Prev = NULLPTR;
    TimerEvent* Event = GetThisCpu()->TimerInf.TimerRoot;
    while (Event)
    {
        if (NrTicks <= Event->LastTickCount)
        {
            return Prev;
        }
        Prev = Event;
        Event = Event->Next;
    }
    return Prev;
}
