[BITS 64]

GLOBAL PopulationCount32

; DWORD /*eax*/ PopulationCount32(DWORD /*edi*/ dwInput)
PopulationCount32:
    POPCNT EAX, EDI
    RET
