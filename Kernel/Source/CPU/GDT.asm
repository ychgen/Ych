[bits 64]

global KrLoadGlobalDescriptorTable

; C function specification: void KrLoadGlobalDescriptorTable(const KrGlobalDescriptorTableRegister* rData, uint16_t sslCode, uint16_t sslData)
KrLoadGlobalDescriptorTable:
    ; RDI = Ptr to GDTR
    ; RSI = sslCode
    ; RDX = sslData

    ; Load GDT
    LGDT [RDI]

    ; Reload CS
    MOVZX RAX, SI
    PUSH RAX
    LEA RAX, [REL .ReloadedCS]
    PUSH RAX
    RETFQ

    .ReloadedCS:
        ; Non CS segments reload
        MOV AX, DX
        MOV DS, AX
        MOV ES, AX
        MOV FS, AX
        MOV GS, AX
        MOV SS, AX

        RET
