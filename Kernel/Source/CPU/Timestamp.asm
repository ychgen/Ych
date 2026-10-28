[BITS 64]

GLOBAL GetProcessorTimestampCounter

; QWORD /*rax*/ GetProcessorTimestampCounter(VOID)
GetProcessorTimestampCounter:
    RDTSCP
    SHL RDX, 32
    OR RAX, RDX
    RET
