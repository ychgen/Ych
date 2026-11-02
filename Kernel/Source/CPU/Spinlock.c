#include "CPU/Spinlock.h"

#include "Core/Krnlmeltdown.h"
#include "Core/Assert.h"

KR_NORETURN VOID KrnldeadlockDetected(CONST Spinlock* pSpinlock, DWORD CpuID)
{
    RUNTIME_ENSURE(FALSE, KR_MDCODE_DEADLOCK_DETECTED,
        "CPU `%u` is waiting to acquire spinlock %p that it already has the lock of. Deadlock detected!",
        CpuID, pSpinlock);
    KR_STATIC_UNREACHABLE();
}

KR_NORETURN VOID UnownedSpinUnlock(CONST Spinlock* pSpinlock, DWORD CpuID)
{
    RUNTIME_ENSURE(FALSE, KR_MDCODE_UNOWNED_SPIN_UNLOCK,
        "CPU `%u` tried to relinquish spinlock %p that it doesn't own the lock of. Lock either unlocked or held by another CPU.",
        CpuID, pSpinlock);
    KR_STATIC_UNREACHABLE();
}
