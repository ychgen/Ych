OUTPUT_FORMAT("binary")
ENTRY(KrStart)

SECTIONS
{
    . = 0x0;           /* start address of the binary */
    .text :
    {
        KEEP(*(.text.KrStart))
        *(.text*)       /* all other code */
    }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) }
}