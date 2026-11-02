#ifndef YCH_KERNEL_CORE_CPU_H
#define YCH_KERNEL_CORE_CPU_H

#include "Krnlych.h"

typedef struct
{
    DWORD dwState; // Bit 0 : Locked
} Spinlock;

VOID AcquireOrWaitSpinlock(Spinlock* pSpinlock);
VOID RelinquishSpinlock(Spinlock* pSpinlock);

KR_NORETURN VOID KrnldeadlockDetected(CONST Spinlock* pSpinlock, DWORD CpuID);
KR_NORETURN VOID UnownedSpinUnlock(CONST Spinlock* pSpinlock, DWORD CpuID);

#endif // !YCH_KERNEL_CORE_CPU_H
