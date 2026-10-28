[BITS 64]

GLOBAL ByteInputFromPort
GLOBAL WordInputFromPort
GLOBAL DwordInputFromPort

GLOBAL ByteOutputToPort
GLOBAL WordOutputToPort
GLOBAL DwordOutputToPort

; BYTE ByteInputFromPort(WORD wPort)
ByteInputFromPort:
    MOV DX, DI
    IN  AL, DX
    RET

; WORD WordInputFromPort(WORD wPort)
WordInputFromPort:
    MOV DX, DI
    IN  AX, DX
    RET

; DWORD /*eax*/ DwordInputFromPort(WORD /*di*/ wPort)
DwordInputFromPort:
    MOV DX, DI
    IN  EAX, DX
    RET

; VOID ByteOutputToPort(WORD wPort, BYTE Byte)
ByteOutputToPort:
    MOV DX, DI
    MOV AL, SIL
    OUT DX, AL
    RET

; VOID WordOutputToPort(WORD wPort, WORD Word)
WordOutputToPort:
    MOV DX, DI
    MOV AX, SI
    OUT DX, AX
    RET

; VOID DwordOutputToPort(WORD /*di*/ wPort, DWORD /*esi*/ Doubleword)
DwordOutputToPort:
    MOV DX, DI
    MOV EAX, ESI
    OUT DX, EAX
    RET
