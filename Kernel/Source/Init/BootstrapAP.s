SECTION .apboostrap

PAT_UC      equ 0 ; Uncacheable
PAT_WC      equ 1 ; Write-Combining
PAT_WT      equ 4 ; Write-Through
PAT_WP      equ 5 ; Write-Protect
PAT_WB      equ 6 ; Write-Back
PAT_UCMINUS equ 7 ; Uncacheable-

struc ApBootInfo
    .CommonEntryPointAddress: resq 1
    .PaddrKernelCR3: resd 1
    .StackBase: resw 1
    .KernelCodeSegmentSelector: resw 1
    .KernelDataSegmentSelector: resw 1
    .GdtrAddress: resw 1
endstruc
.Size equ $ - ApBootInfo

BootInfoAddr equ 0x7000

[BITS 16]
[ORG  0x8000]

; No interrupts and FLAGS.DF = forward
CLI
CLD

; Acquire 64-byte stack
MOV AX, -64
LOCK XADD WORD [BootInfoAddr + ApBootInfo.StackBase], AX
MOV SP, AX

; Load GDT
XOR EAX, EAX ; zero 32-bit register
MOV AX, [BootInfoAddr + ApBootInfo.GdtrAddress]
LGDT [EAX]

; Load CR3
MOV EAX, [BootInfoAddr + ApBootInfo.PaddrKernelCR3]
MOV CR3, EAX

; Enable PSE + PAE + PGE + OSFXSR
MOV EAX, CR4
OR  EAX, (1 << 4) | (1 << 5) | (1 << 7) | (1 << 9)
MOV CR4, EAX

; Assert LME (Long Mode Enable)
MOV ECX, 0xC0000080 ; IA32_EFER
RDMSR ; EDX:EAX
OR EAX, (1 << 8) ; LME
WRMSR

;;;;; THIS SECTION HAS BEEN COPIED OVER FROM Boot/Bootelvt.asm AS-IS TO KEEP IN ACCORDANCE WITH THE BSP
; Setup PAT stage 1, disable caching
MOV EAX, CR0
OR  EAX,  (1 << 30) ; CD set (disable caching)
AND EAX, ~(1 << 29) ; NW unset
MOV CR0, EAX
WBINVD
; Setup PAT
MOV ECX, 0x277 ; PAT MSR
; Initialize to standard value with 1 thing different: Entry 4 uses WC
MOV EAX, (PAT_UC << 24) | (PAT_UCMINUS << 16) | (PAT_WT << 8) | (PAT_WB << 0) ; LODWORD (keep as-is for backward compatibility)
MOV EDX, (PAT_UC << 24) | (PAT_UCMINUS << 16) | (PAT_WT << 8) | (PAT_WC << 0) ; HIDWORD (modifiable. Bootelvt uses entry 4 for WC and maps frame buffer as such)
WRMSR ; Writes EDX:EAX to PAT MSR
; Enable caching again
MOV EAX, CR0
AND EAX, ~(1 << 30) ; CD unset (enable caching)
MOV CR0, EAX

; Enable PE + WP + PG
MOV EAX, CR0
OR  EAX, (1 << 0) | (1 << 16) | (1 << 31)
MOV CR0, EAX

; x86 and its CS-reload shenanigans
PUSH WORD [BootInfoAddr + ApBootInfo.KernelCodeSegmentSelector]
PUSH LmEntry
RETF

[BITS 64]

LmEntry:
    ; Segment registers fun as always
    MOV AX, [BootInfoAddr + ApBootInfo.KernelDataSegmentSelector]
    MOV DS, AX
    MOV ES, AX
    MOV FS, AX
    MOV GS, AX
    MOV SS, AX

    MOVZX ESP, SP ; Ensure our 16 bit SP keeps it's glory (presumably something else should be assigned to this immediately within KrApplicationProcessorStart because you will run out immediately)
    AND ESP, ~0x7 ; align to 8 byte boundary as expected by ABI (not 16 because 16 is expected at the moment of CALL which on entry makes it 8 aligned. Since we JMP we need 8 alignment)
    JMP [BootInfoAddr + ApBootInfo.CommonEntryPointAddress] ; And there goes the AP into the C world, see you around in Kernel/Source/Init/KrApplicationProcessorStart.c

; in case of return (dont)
Halt:
    CLI
    HLT
    JMP Halt
