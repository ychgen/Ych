OUTPUT_FORMAT("binary")
ENTRY(KrKernelStart)

SECTIONS
{
    . = 0xFFFFFFFF80000000;

    .text ALIGN(4096) :
    {
        KEEP(*(.text.KrKernelStart))
        *(.text*)
    }

    .rodata ALIGN(4096) :
    {
        *(.rodata*)
    }

    .data ALIGN(4096) :
    {
        *(.data*)
    }

    .bss ALIGN(4096) :
    {
        __KR_LINK_BSS_START = .;
        *(COMMON)
        *(.bss*)
        __KR_LINK_BSS_END = .;
    }
}