[org  0x100000]
[bits 64]

%include "../BootContract/Defines.asm"

PAGE_HIERARCHY_ENTRY_COUNT equ 512
PAGE_HIERARCHY_ENTRY_SIZE  equ 8
PAGE_HIERARCHY_TOTAL_SIZE  equ (PAGE_HIERARCHY_ENTRY_COUNT * PAGE_HIERARCHY_ENTRY_SIZE)
BOOT_INFO_MAGIC_OFFSET     equ 0
BOOT_INFO_FBUF_OFFSET      equ 68
BOOT_INFO_FBUFSZ_OFFSET    equ (BOOT_INFO_FBUF_OFFSET + 8)

PAT_UC      equ 0 ; Uncacheable
PAT_WC      equ 1 ; Write-Combining
PAT_WT      equ 4 ; Write-Through
PAT_WP      equ 5 ; Write-Protect
PAT_WB      equ 6 ; Write-Back
PAT_UCMINUS equ 7 ; Uncacheable-

; Bootloader ensures that:
;   RCX = Size of the KrSystemInfoPack structure.
;   RSI = Pointer to the KrSystemInfoPack structure.
Bootelvt: ; `Boot Elevate` entry point
    CLI ; Being pedantic about interrupts does not hurt anyone.

    ; Keep a copy of the KrSystemInfoPack, will be lost when we change to paging
    MOV [abs BOOT_INFO_PACK_SIZE], RCX
    MOV RDI, BOOT_INFO
    REP MOVSB

    MOV EAX, [BOOT_INFO + BOOT_INFO_MAGIC_OFFSET]
    CMP EAX, SYSTEM_INFO_PACK_MAGIC
    JNE ProcessorHalt

    ; Setup PAT stage 1, disable caching
    MOV RAX, CR0
    OR  RAX,  (1 << 30) ; CD set (disable caching)
    AND RAX, ~(1 << 29) ; NW unset
    MOV CR0, RAX
    WBINVD
    ; Setup PAT
    MOV ECX, 0x277 ; PAT MSR
    ; Initialize to standard value with 1 thing different: Entry 4 uses WC
    MOV EAX, (PAT_UC << 24) | (PAT_UCMINUS << 16) | (PAT_WT << 8) | (PAT_WB << 0) ; LODWORD (keep as-is for backward compatibility)
    MOV EDX, (PAT_UC << 24) | (PAT_UCMINUS << 16) | (PAT_WT << 8) | (PAT_WC << 0) ; HIDWORD (modifiable. Bootelvt uses entry 4 for WC and maps frame buffer as such)
    WRMSR ; Writes EDX:EAX to PAT MSRR
    ; Enable caching again
    MOV RAX, CR0
    AND RAX, ~(1 << 30) ; CD unset (enable caching)
    MOV CR0, RAX

    MOV RDX, PML4
    MOV RAX, KRNL_PDPT
    OR  RAX, 0x03 ; P + W
    MOV [RDX + 511 * PAGE_HIERARCHY_ENTRY_SIZE], RAX ; PML4[511] = KRNL_PDPT
    MOV RAX, IDPAGE_PDPT
    OR  RAX, 0x03 ; P + W
    MOV [RDX], RAX ; PML4[0] = IDPAGE_PDPT

    MOV RDX, KRNL_PDPT
    MOV RAX, KRNL_PD
    OR  RAX, 0x03 ; P + W
    MOV [RDX + 510 * PAGE_HIERARCHY_ENTRY_SIZE], RAX ; KRNL_PDPT[510] = KRNL_PD
    MOV RAX, FBUF_PD
    OR  RAX, 0x03 ; P + W
    MOV [RDX + 511 * PAGE_HIERARCHY_ENTRY_SIZE], RAX ; KRNL_PDPT[511] = FBUF_PD

    MOV RDX, KRNL_PD
    MOV RBX, 0x200000 ; Kernel loaded at physical 2MiB
    MOV RCX, KERNEL_RESERVE_SIZE / 0x200000 ; Kernel reserves 128MiB, 128MiB / 2MiB = 64 Large Pages
    .PopulateKernelPages:
        MOV RAX, RBX
        OR  RAX, 0x83 ; P + W + PS(2MiB)
        MOV [RDX], RAX
        ADD RDX, PAGE_HIERARCHY_ENTRY_SIZE
        ADD RBX, 0x200000 ; Next 2MiB of kernel.
        LOOP .PopulateKernelPages
    
    XOR RDX, RDX
    MOV RAX, [BOOT_INFO + BOOT_INFO_FBUFSZ_OFFSET] ; Frame Buffer Size
    MOV RBX,  0x200000
    DIV RBX ; FBSIZE / 2MiB in RAX after thus

    CMP RDX, 0
    JE .Proceed
    INC RAX

.Proceed:

    MOV RDX, FBUF_PD
    MOV RBX, [BOOT_INFO + BOOT_INFO_FBUF_OFFSET] ; Physical Frame Buffer Address
    MOV RCX, RAX ; No. 2MiB pages, calculated above
    .PopulateFrameBufferPages:
        MOV RAX, RBX
        OR  RAX, 0x1083 ; P + W + PS(2MiB) + PAT entry 4 (PAT:1,PWT:0,PCD:0) [WC]
        MOV [RDX], RAX
        ADD  RDX, PAGE_HIERARCHY_ENTRY_SIZE
        ADD  RBX, 0x200000 ; Next 2MiB
        LOOP .PopulateFrameBufferPages

    MOV RDX, IDPAGE_PDPT
    MOV RAX, IDPAGE_PD
    OR  RAX, 0x03 ; P + W
    MOV [RDX], RAX ; IDPAGE_PDPT[0] = IDPAGE_PD

    MOV RDX, IDPAGE_PD
    MOV RAX, IDPAGE_PT
    OR  RAX, 0x03 ; P + W
    MOV [RDX], RAX ; IDPAGE_PD[0] = IDPAGE_PT

    ; Identity map lower 2MiB of memory since we are at there for now (keeps RIP as Register Instrucion Pointer and not Rest in Peace)
    MOV RDX, IDPAGE_PT
    XOR RBX, RBX
    MOV RCX, 0x200000 / 0x1000 ; 2MiB/4KiB=512
    .PopulateIDMapPages:
        MOV  RAX, RBX
        OR   RAX, 0x3 ; P + W
        MOV [RDX], RAX
        ADD  RDX, PAGE_HIERARCHY_ENTRY_SIZE
        ADD  RBX, 0x1000 ; Next 4KiB
        LOOP .PopulateIDMapPages

    MOV RAX, CR4
    OR  RAX, (1 << 4) | (1 << 5) ; Page Size Extension + Physical Address Extension
    MOV CR4, RAX

    MOV RAX, PML4
    MOV CR3, RAX  ; Load PML4 into CR3 (specifies physical address! fine because we are identity-mapped as of now.)

    MOV  RSP, (KERNEL_VIRTUAL_ADDRESS + KERNEL_RESERVE_SIZE - 16) ; Stack placed at the top of kernel reserved area.
                                                        ; YchBootContract specifies: When bootloader transfers control to the Kernel
                                                        ; the stack is set to or near ADDR_LOAD_KERNEL(phys or virt) + KERNEL_RESERVE_SIZE.
                                                        ; The reserved stack space is always 2MiB at the very end no matter exact location.
                                                        ; In accordance to this, we put it quite near the top. 
    MOV  RAX, KERNEL_VIRTUAL_ADDRESS
    MOV  RDI, BOOT_INFO ; KrKernelStart(const KrSystemInfoPack*)'s Parameter.
    CALL RAX ; KrKernelStart (Important: use `CALL`, if you use `JMP`, your stack is fucked.)

    ; Kernel returned? Somebody is drunk.
    JMP ProcessorHalt

ProcessorHalt:
    CLI
    .InternalHaltCycle:
        HLT
        JMP .InternalHaltCycle

BOOT_INFO_PACK_SIZE: dq 0
BOOT_INFO:           times 4096 db 0 ; Reserve 4KiB for KrSystemInfoPack. Should be more than enough...

align 4096
PML4:        times PAGE_HIERARCHY_TOTAL_SIZE db 0
align 4096
KRNL_PDPT:   times PAGE_HIERARCHY_TOTAL_SIZE db 0
align 4096
KRNL_PD:     times PAGE_HIERARCHY_TOTAL_SIZE db 0
align 4096
FBUF_PD:     times PAGE_HIERARCHY_TOTAL_SIZE db 0

align 4096
IDPAGE_PDPT: times PAGE_HIERARCHY_TOTAL_SIZE db 0
align 4096
IDPAGE_PD:   times PAGE_HIERARCHY_TOTAL_SIZE db 0
align 4096
IDPAGE_PT:   times PAGE_HIERARCHY_TOTAL_SIZE db 0
