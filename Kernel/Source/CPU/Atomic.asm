[BITS 64]

GLOBAL AtomicExchangeAndAdd32

; DWORD /*eax*/ AtomicExchangeAndAdd32(DWORD* /*rdi*/ pDoubleword, DWORD /*esi*/ ValueToAdd)
AtomicExchangeAndAdd32:
    LOCK XADD [RDI], ESI
    MOV EAX, ESI
    RET
