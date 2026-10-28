#ifndef YCH_KERNEL_CPU_PER_CPU_H
#define YCH_KERNEL_CPU_PER_CPU_H

#include "Krnlych.h"

typedef struct TimerEvent
{
    struct TimerEvent* Next; // Ensured that this->LastTickCount <= Next->LastTickCount
    ULONG LastTickCount; // Remaining tick count by the latest timer interrupt
    VOID (*Callback)(VOID);
    DWORD dwFlags;
} TimerEvent;

typedef struct
{
    // How many local APIC ticks must be elapsed to wait about 1 microsecond.
    DWORD CalibrationValue;

    /** Timer Event Queue Singly Linked List. Always sorted by LastRemainingTickCount. */
    TimerEvent* TimerRoot;
} TimerInfo;

typedef struct PerCpu
{
    struct PerCpu* pSelf; // Pointer to self structure
    DWORD ID;             // Processor Local APIC ID
    BOOL bInPageFault;    // Currently in #PF ? Used to detect recursive page faults, i.e. page fault during a page fault.
    TimerInfo TimerInf;   // APIC Timer & per-CPU timer management data
} PerCpu;

/**
 * @brief Sets a timer event on this processor.
 * Per-CPU! Each processor has its own timer queue and management.
 * 
 * @param uSeconds In nanoseconds, the amount of time for this timer event to fire.
 * @param Callback The callback routine to call when the timer fires, can be NULLPTR if no callback is desired.
 * @param dwFlags Various flags using TIMER_EVENT_FLAG_XXX macros.
 */
BOOL SetTimer(ULONG uSeconds, VOID (*Callback)(VOID), DWORD dwFlags);

VOID GrabThisCpuStruct(VOID);
PerCpu* GetThisCpu(VOID);
DWORD GetThisCpuID(VOID);

#endif // !YCH_KERNEL_CPU_PER_CPU_H
