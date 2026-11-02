#ifndef YCH_KERNEL_CPU_PER_CPU_H
#define YCH_KERNEL_CPU_PER_CPU_H

#include "CPU/Interrupt.h"
#include "CPU/Spinlock.h"

#define MAX_CONCURRENT_SLEEPS 8

#define TIMER_EVENT_FLAGS_USECONDS_AS_TICKS (1U << 31)

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

typedef struct
{
    VOID (*Handler)(CONST InterruptFrame*);
    struct DeviceNode* HwDevice; // If this interrupt is device-sourced, must be valid. Otherwise set to NULLPTR
    DWORD IRQ;                   // If this interrupt is device-sourced, must be valid. Otherwise set to DWORD_MAX
    UCHAR Vector; // Int vector #. If set to 0x00 (except for actual 0x00 interrupt vector), you must assume empty vector and all rules for other fields invalid.
} PerCpuInterrupt;

typedef struct PerCpu
{
    struct PerCpu* pSelf; // Pointer to self structure
    DWORD ID;             // Processor Local APIC ID
    BOOL bInPageFault;    // Currently in #PF ? Used to detect recursive page faults, i.e. page fault during a page fault.
    TimerInfo TimerInf;   // APIC Timer & per-CPU timer management data

    Spinlock IntmodSpinlock; // Interrupt Modification Spinlock
    PerCpuInterrupt Interrupt[NUM_INT_VECTORS];
} PerCpu;

VOID SetInterrupt(DWORD CpuID, PerCpuInterrupt Data);
VOID SetHardwareInterrupt(DWORD CpuID, UCHAR IntVector, struct DeviceNode* HwDevice, DWORD IRQ, VOID (*Handler)(CONST InterruptFrame*));
VOID SetExceptionInterrupt(DWORD CpuID, UCHAR IntVector, VOID (*Handler)(CONST InterruptFrame*));
VOID SetInterruptHandler(DWORD CpuID, UCHAR Vector, VOID (*Handler)(CONST InterruptFrame*));
VOID UnsetInterrupt(DWORD CpuID, UCHAR IntVector);

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
PerCpu* GetThatCpu(DWORD CpuID);
PerCpu* GetThisCpu(VOID);
DWORD GetThisCpuID(VOID);

#endif // !YCH_KERNEL_CPU_PER_CPU_H
