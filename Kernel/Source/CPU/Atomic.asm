[BITS 64]

GLOBAL AtomicExchangeAndAdd32

; DWORD /*eax*/ AtomicExchangeAndAdd32(DWORD* /*rdi*/ pDoubleword, DWORD /*esi*/ ValueToAdd)
AtomicExchangeAndAdd32:
    MOV EAX, ESI
    LOCK XADD [RDI], EAX
    RET
