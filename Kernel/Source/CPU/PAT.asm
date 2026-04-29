[bits 64]

global KrLoadPatMsr

; C function declared in PAT.h : `VOID KrLoadPatMsr(QWORD qwPatMsr)`
KrLoadPatMsr:
    ; RDI contains qwPatMsr (System V AMD64 calling convention)

    ; We really do not want to be interrupted during this trust me.
    CLI

    ; Load PAT stage 1, disable caching
    MOV RAX, CR0
    AND RAX, ~(1 << 29) ; NW unset
    OR  RAX,  (1 << 30) ; CD set (disables caching)
    MOV CR0, RAX
    WBINVD ; Write-Back and Invalidate Cache

    ; Load PAT stage 2, configure the PAT MSR
    MOV ECX, 0x277 ; IA32_PAT_MSR
    MOV EAX, EDI ; EAX = LODWORD of RDI
    SHR RDI, 32
    MOV EDX, EDI ; EDX = HIDWORD of original RDI (due to SHR above we can do this)
    WRMSR

    ; Load PAT stage 3, reenable caching
    MOV RAX, CR0
    AND RAX, ~(1 << 30) ; CD unset (enables caching)
    MOV CR0, RAX

    STI
    RET
