[bits 64]

global KrInByteFromPort
global KrInWordFromPort
global KrOutByteToPort
global KrOutWordToPort

; BYTE KrInByteFromPort(WORD wPort)
KrInByteFromPort:
    MOV DX, DI
    IN  AL, DX
    RET

; WORD KrInWordFromPort(WORD wPort)
KrInWordFromPort:
    MOV DX, DI
    IN  AX, DX
    RET

; VOID KrOutByteToPort(WORD wPort, BYTE Byte)
KrOutByteToPort:
    MOV DX, DI
    MOV AL, SIL
    OUT DX, AL
    RET

; VOID KrOutWordToPort(WORD wPort, WORD Word)
KrOutWordToPort:
    MOV DX, DI
    MOV AX, SI
    OUT DX, AX
    RET
