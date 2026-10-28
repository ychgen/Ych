[BITS 64]

GLOBAL AcquireOrWaitSpinlock
GLOBAL RelinquishSpinlock

; VOID AcquireOrWaitSpinlock(Spinlock* /*rdi*/ pSpinlock)
AcquireOrWaitSpinlock:
    CLI ; Do not acknowledge interrupts during spinning (deadlocks highly plausible otherwise)
    .TryAcquireLock:
        LOCK BTS DWORD [RDI], 0
        JC .SpinWaitLock
        RET
    .SpinWaitLock:
        PAUSE
        TEST DWORD [RDI], 1
        JNZ .SpinWaitLock
        JMP .TryAcquireLock

; VOID RelinquishSpinlock(Spinlock* /*rdi*/ pSpinlock)
RelinquishSpinlock:
    MOV DWORD [RDI], 0
    STI
    RET
