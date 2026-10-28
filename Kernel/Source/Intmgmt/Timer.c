#include "Intmgmt/Timer.h"

#include "CPU/PerCpu.h"
#include "CPU/APIC.h"

#include "Memory/Krnlheap.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

VOID IntmgmtTimerInterrupt(const KrInterruptFrame* pIntfr)
{
    KrDisableInterrupts(); // Disable interrupts so we can reorder and rearm timer asap to keep timer accuracy relatively okay

    PerCpu* CONST ThisCpu = GetThisCpu();
    TimerEvent* Event = ThisCpu->TimerInf.TimerRoot;
    
    const DWORD dwInitialTicks = ApicGetTimerInitialCount();
    while (Event)
    {
        if (Event->LastTickCount <= dwInitialTicks)
        {
            // No need to re-order as it is sorted, so those that expire are sequential from root,
            // until you get to ones that aren't expired. You only need to change the root node,
            // which must unconditionally change as we iterate from root to tail and expirations therefore
            // propagate from head towards tail...........

            if (Event->Callback)
            {
                Event->Callback();
            }
            
            TimerEvent* Nxt = Event->Next;
            ThisCpu->TimerInf.TimerRoot = Nxt;
            HeapRelinquish(Event);

            Event = Nxt;
        }
        else
        {
            break; // process in separate loop without an if to avoid further branching, since from forward all are unexpired. little performance thingy
        }
    }
    while (Event)
    {
        Event->LastTickCount -= dwInitialTicks;
        Event = Event->Next;
    }

    KrEnableInterrupts();
    if (ThisCpu->TimerInf.TimerRoot)
    {
        ApicSetTimerCount(MACROMIN(DWORD_MAX, ThisCpu->TimerInf.TimerRoot->LastTickCount));
    }
}
