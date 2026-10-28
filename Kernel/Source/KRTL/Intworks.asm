[BITS 64]

GLOBAL PopulationCount8
GLOBAL PopulationCount32

; DWORD /*eax*/ PopulationCount8(BYTE /*dil*/ ByteInput)
PopulationCount8:
    XOR EAX, EAX
    MOVZX EDI, DIL
    POPCNT AX, DI
    RET

; DWORD /*eax*/ PopulationCount32(DWORD /*edi*/ dwInput)
PopulationCount32:
    POPCNT EAX, EDI
    RET
