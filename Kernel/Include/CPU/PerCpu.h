#ifndef YCH_KERNEL_CPU_PER_CPU_H
#define YCH_KERNEL_CPU_PER_CPU_H

#include "Krnlych.h"

#define MAX_CONCURRENT_SLEEPS 8

#define TIMER_EVENT_FLAGS_INTERNAL_TREAT_USECONDS_AS_TICKS (1U << 31)

typedef struct TimerEvent
{
    struct TimerEvent* Next; // Ensured that this->LastTickCount <= Next->LastTickCount
    ULONG LastTickCount; // Remaining tick count by the latest timer interrupt
    VOID (*Callback)(VOID* UserParam);
    VOID* UserParam;
    DWORD dwFlags;
} TimerEvent;

typedef struct
{
    // How many local APIC ticks must be elapsed to wait about 1 microsecond.
    DWORD CalibrationValue;

    /** Timer Event Queue Singly Linked List. Always sorted by LastRemainingTickCount. */
    TimerEvent* TimerRoot;
    /** Timer Event Inproc-Await-Insertion Queue Singly Linked List. Always sorted by LastRemainingTickCount. */
    TimerEvent* TimerDeferredRoot;

    BOOL bInTimerInt; // Currently in timer interrupt frame?
    UCHAR SleepCounter;
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
 * @param UserParam Pointer to be provided to the callback function
 * @param dwFlags Various flags using TIMER_EVENT_FLAG_XXX macros.
 */
BOOL SetTimer(ULONG uSeconds, VOID (*Callback)(VOID*), VOID* UserParam, DWORD dwFlags);
// Uses SetTimer, this is a blocking function that won't return until uSeconds passes.
VOID Sleep(ULONG uSeconds);

VOID GrabThisCpuStruct(VOID);
PerCpu* GetThisCpu(VOID);
DWORD GetThisCpuID(VOID);

#endif // !YCH_KERNEL_CPU_PER_CPU_H
