#include "CPU/PerCpu.h"

#include "Core/KernelState.h"
#include "Core/Assert.h"

#include "CPU/CPUID.h"
#include "CPU/APIC.h"
#include "CPU/Halt.h"
#include "CPU/MSR.h"

#include "KRTL/Intworks.h"

#include "Memory/Krnlheap.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

STATIC PerCpu sg_PerCpu[MAX_SMP_PROCESSORS] = {0};

// Returns the node to insert after
STATIC TimerEvent* FindTimerEventInsertPoint(TimerEvent* Root, ULONG NrTicks);

STATIC VOID SleepDoneCallback(VOID* Aux);

VOID SetInterrupt(DWORD CpuID, PerCpuInterrupt Data)
{
    PerCpu* Cpu = GetThatCpu(CpuID);
    
    AcquireOrWaitSpinlock(&Cpu->IntmodSpinlock);
    {
        Cpu->Interrupt[Data.Vector] = Data;
    }
    RelinquishSpinlock(&Cpu->IntmodSpinlock);
}

VOID SetHardwareInterrupt(DWORD CpuID, UCHAR IntVector, struct DeviceNode* HwDevice, DWORD IRQ, VOID (*Handler)(CONST InterruptFrame*))
{
    SetInterrupt(CpuID, (PerCpuInterrupt) {
        .Handler = Handler,
        .HwDevice = HwDevice,
        .IRQ = IRQ,
        .Vector = IntVector
    });
}

VOID SetExceptionInterrupt(DWORD CpuID, UCHAR IntVector, VOID (*Handler)(CONST InterruptFrame*))
{
    SetInterrupt(CpuID, (PerCpuInterrupt) {
        .Handler = Handler,
        .HwDevice = NULLPTR,
        .IRQ = DWORD_MAX,
        .Vector = IntVector
    });
}

VOID SetInterruptHandler(DWORD CpuID, UCHAR Vector, VOID (*Handler)(CONST InterruptFrame*))
{
    PerCpu* Cpu = GetThatCpu(CpuID);
    
    AcquireOrWaitSpinlock(&Cpu->IntmodSpinlock);
    {
        Cpu->Interrupt[Vector].Handler = Handler;
    }
    RelinquishSpinlock(&Cpu->IntmodSpinlock);
}

VOID UnsetInterrupt(DWORD CpuID, UCHAR IntVector)
{
    PerCpu* Cpu = GetThatCpu(CpuID);
    
    AcquireOrWaitSpinlock(&Cpu->IntmodSpinlock);
    {
        Cpu->Interrupt[IntVector] = (PerCpuInterrupt) {
            .Handler = NULLPTR,
            .HwDevice = NULLPTR,
            .IRQ = DWORD_MAX,
            .Vector = 0x00
        };
    }
    RelinquishSpinlock(&Cpu->IntmodSpinlock);
}

BOOL SetTimer(ULONG uSeconds, VOID (*Callback)(VOID*), VOID* UserParam, DWORD dwFlags)
{
    ApicSetTimerCount(0); // Stop APIC counter from ticking

    PerCpu* ThisCpu = GetThisCpu();
    ULONG NrTicks = uSeconds;
    if (!(dwFlags & TIMER_EVENT_FLAGS_USECONDS_AS_TICKS))
    {
        NrTicks *= ThisCpu->TimerInf.CalibrationValue;
    }
    else
    {
        // Function-input, not metadata for the node, so remove if present
        dwFlags &= ~(TIMER_EVENT_FLAGS_USECONDS_AS_TICKS);
    }

    TimerEvent* Event = HeapAcquire(sizeof(TimerEvent));
    if (!Event)
    {
        ApicSetTimerCount(ApicGetTimerInitialCount()); // shady, we should record initcr and set to that instead, currently I dont care as it requires some logic because of race conditions
        return FALSE;
    }
    Event->LastTickCount = NrTicks;
    Event->Callback = Callback;
    Event->UserParam = UserParam;
    Event->dwFlags = dwFlags;

    // Rearrange the linked list & figure out event->next
    TimerEvent** PtrToRootPtr = ThisCpu->TimerInf.bInTimerInt ? &ThisCpu->TimerInf.TimerDeferredRoot : &ThisCpu->TimerInf.TimerRoot;
    {
        TimerEvent* InsertAfter;
        if (InsertAfter = FindTimerEventInsertPoint(*PtrToRootPtr, NrTicks))
        {
            Event->Next = InsertAfter->Next;
            InsertAfter->Next = Event;
        }
        else // InsertAfter == NULLPTR means to insert as root
        {
            Event->Next = *PtrToRootPtr;
            *PtrToRootPtr = Event;
        }
    }

    if (!ThisCpu->TimerInf.bInTimerInt)
    {
        // Write to LAPIC Timer the smallest tick value, since its sorted, root always has the smallest value
        // We also at most write DWORD_MAX as that's as much as the register can handle. IntmgmtTimerInterrupt() handles big ones.
        ApicSetTimerCount(MACROMIN(DWORD_MAX, ThisCpu->TimerInf.TimerRoot->LastTickCount));
    }

    return TRUE;
}

VOID Sleep(ULONG uSeconds)
{
    PerCpu* ThisCpu = GetThisCpu();
    RUNTIME_ASSERT(!ThisCpu->TimerInf.bInTimerInt, KR_MDCODE_SLEEP_INSIDE_TIMER_CALLBACK, "CPU %u had Sleep() called inside a timer callback.", ThisCpu->ID);
    
    UCHAR Ballot = ThisCpu->TimerInf.SleepCounter == 0 ? 0 : __builtin_ctz((UINT) ThisCpu->TimerInf.SleepCounter);
    RUNTIME_ENSURE(Ballot < MAX_CONCURRENT_SLEEPS, KR_MDCODE_ABSURD_CONCURRENT_SLEEPS, "Per-CPU maximum concurrent sleeps (%u) overrun.", MAX_CONCURRENT_SLEEPS);

    ThisCpu->TimerInf.SleepCounter |= (1 << Ballot); // ballot valid
    SetTimer(uSeconds, SleepDoneCallback, (VOID*) Ballot, 0);
    
    while (TRUE)
    {
        ProcessorHalt();
        // Code below will run once CPU gets out of halt
        if (!(ThisCpu->TimerInf.SleepCounter & (1 << Ballot)))
        {
            break; // Break out of the halt loop, SleepDoneCallback cleared our ballot
        }
        // rehalt, something else woke the CPU up
    }
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

PerCpu* GetThatCpu(DWORD CpuID)
{
    return sg_PerCpu + CpuID;
}

DWORD GetThisCpuID(VOID)
{
    return GetThisCpu()->ID;
}

STATIC TimerEvent* FindTimerEventInsertPoint(TimerEvent* Root, ULONG NrTicks)
{
    TimerEvent* Prev = NULLPTR;
    TimerEvent* Event = Root;
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

STATIC VOID SleepDoneCallback(VOID* Aux)
{
    UCHAR Ballot = (UCHAR) Aux;
    GetThisCpu()->TimerInf.SleepCounter &= ~(1 << Ballot);
}
