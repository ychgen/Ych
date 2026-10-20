[BITS 64]

GLOBAL AtomicExchangeOr8
GLOBAL AtomicExchangeAdd32

GLOBAL AtomicOr8

; BYTE /*rax*/ AtomicExchangeOr8(BYTE* /*rdi*/ pByte, BYTE /*sil*/ OrValue)
AtomicExchangeOr8:
    MOV AL, [RDI]
.Try:
    MOV DL, AL
    OR  DL, SIL
    LOCK CMPXCHG [RDI], DL
    JNZ .Try
    MOVZX EAX, DL   
    RET

; DWORD /*eax*/ AtomicExchangeAdd32(DWORD* /*rdi*/ pDoubleword, DWORD /*esi*/ ValueToAdd)
AtomicExchangeAdd32:
    MOV EAX, ESI
    LOCK XADD [RDI], EAX
    RET

; VOID AtomicOr8(BYTE* /*rdi*/ pByte, BYTE /*sil*/ OrValue)
AtomicOr8:
    LOCK OR [RDI], SIL
    RET
