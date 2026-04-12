OUTPUT_FORMAT("binary")
ENTRY(KrKernelStart)

SECTIONS
{
    . = 0xFFFFFFFF80000000;

    .text ALIGN(16) :
    {
        KEEP(*(.text.KrKernelStart))
        *(.text*)
    }

    .rodata ALIGN(16) :
    {
        *(.rodata*)
    }

    .data ALIGN(16) :
    {
        *(.data*)
    }

    .bss ALIGN(16) :
    {
        __KR_LINK_BSS_START = .;
        *(COMMON)
        *(.bss*)
        __KR_LINK_BSS_END = .;
    }
}