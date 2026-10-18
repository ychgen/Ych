#ifndef YCH_KERNEL_CORE_CPU_H
#define YCH_KERNEL_CORE_CPU_H

#include "Krnlych.h"

typedef struct
{
    BOOL State;
} Spinlock;

VOID AcquireOrWaitSpinlock(Spinlock* pSpinlock);
VOID RelinquishSpinlock(Spinlock* pSpinlock);

#endif // !YCH_KERNEL_CORE_CPU_H
