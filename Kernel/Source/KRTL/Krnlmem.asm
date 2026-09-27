[bits 64]

global KrtlContiguousCopyBuffer
global KrtlContiguousMoveBuffer

; VOID* KrtlContiguousCopyBuffer(VOID* restrict pDest, const VOID* restrict pSrc, SIZE N);
KrtlContiguousCopyBuffer:
    MOV RCX, RDX
    MOV RAX, RDI
    REP MOVSB
    RET

; VOID* KrtlContiguousMoveBuffer(VOID*          pDest, const VOID*          pSrc, SIZE N);
KrtlContiguousMoveBuffer:
    mov     rax, rdi
    test    rdx, rdx
    jz      .L_end
    cmp     rdi, rsi
    je      .L_end

    ; dest < src => copy forward
    jb      .L_forward

    ; dest > src => copy backward
    lea     rcx, [rsi + rdx]
    lea     r8,  [rdi + rdx]

.L_backward:
    dec     rcx
    dec     r8
    mov     r9b, [rcx]
    mov     [r8], r9b
    dec     rdx
    jnz     .L_backward
    ret

.L_forward:
    mov     rcx, rdx
    cld
    rep     movsb

.L_end:
    ret
