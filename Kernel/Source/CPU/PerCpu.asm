[BITS 64]

GLOBAL GetThisCpu

; PerCpu* /*rax*/ GetThisCpu(VOID)
GetThisCpu:
    MOV RAX, GS:[0x0]
    RET
