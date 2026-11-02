[BITS 64]

; C handler
EXTERN KrDispatchInterrupt
EXTERN KrnlISRs

%MACRO ISR_WITHOUT_EC 1
GLOBAL KrInterruptServiceRoutine_%1
KrInterruptServiceRoutine_%1:
    PUSH 0  ; Push dummy error code for uniform InterruptFrame struct
    PUSH %1 ; IntNo
    JMP KrInterruptServiceRoutineWrapperCommon
%ENDMACRO

%MACRO ISR_WITH_EC 1
GLOBAL KrInterruptServiceRoutine_%1
KrInterruptServiceRoutine_%1:
    PUSH %1 ; IntNo
    JMP KrInterruptServiceRoutineWrapperCommon
%ENDMACRO

%ASSIGN i 0
%REP 256
    %IF i = 8 || i = 10 || i = 11 || i = 12 || i = 13 || i = 14 || i = 17
        ISR_WITH_EC i
    %ELSE
        ISR_WITHOUT_EC i
    %ENDIF
    %ASSIGN i i+1
%ENDREP

KrnlISRs:
    %ASSIGN i 0
    %REP 256
        DQ KrInterruptServiceRoutine_%+i
        %ASSIGN i i+1
    %ENDREP

KrInterruptServiceRoutineWrapperCommon:
    PUSH RAX
    PUSH RBX
    PUSH RCX
    PUSH RDX
    PUSH RBP
    PUSH RSI
    PUSH RDI
    PUSH R8
    PUSH R9
    PUSH R10
    PUSH R11
    PUSH R12
    PUSH R13
    PUSH R14
    PUSH R15
    
    CLD
    MOV RDI, RSP
    CALL KrDispatchInterrupt
    
    POP R15
    POP R14
    POP R13
    POP R12
    POP R11
    POP R10
    POP R9
    POP R8
    POP RDI
    POP RSI
    POP RBP
    POP RDX
    POP RCX
    POP RBX
    POP RAX

    ADD RSP, 16 ; Pop IntNo + EC
    IRETQ
