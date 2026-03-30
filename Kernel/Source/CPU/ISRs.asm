[bits 64]

; C handler
extern KrDispatchInterrupt

%macro ISR_WITHOUT_EC 1
global KrInterruptServiceRoutine_%1
KrInterruptServiceRoutine_%1:
    PUSH 0  ; Push error code 0 indicating no error
    PUSH %1 ; IntNo
    JMP KrInterruptServiceRoutineWrapperCommon
%endmacro

%macro ISR_WITH_EC 1
global KrInterruptServiceRoutine_%1
KrInterruptServiceRoutine_%1:
    PUSH %1 ; IntNo
    JMP KrInterruptServiceRoutineWrapperCommon
%endmacro

%assign i 0
%rep 256
    %if i = 8 || i = 10 || i = 11 || i = 12 || i = 13 || i = 14 || i = 17
        ISR_WITH_EC i
    %else
        ISR_WITHOUT_EC i
    %endif
    %assign i i+1
%endrep

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
