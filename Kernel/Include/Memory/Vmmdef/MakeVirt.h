#ifndef YCH_KERNEL_MEMORY_PRIVATE_VMM_MAKE_VIRT_H
#define YCH_KERNEL_MEMORY_PRIVATE_VMM_MAKE_VIRT_H

/**
 * @brief Constructs a virtual address from PML4, PDPT, PD and PT indices and an offset (up to 4KiB, 2MiB or 1GiB depending on the page).
 * If you are referring to a Huge Page (1GiB), leave IndexPD and IndexPT as 0 and your Offset can be larger (up to 1GiB).
 * If you are referring to a Large Page (2MiB), leave IndexPT as 0 and your Offset can be larger (up to 2MiB).
 * If you provide garbage parameters, it's completely your fault, do not blame me.
 * If you provide an Offset larger than the sort of page you want can address, it will bleed into indices and fuck everything up.
 * These are my kindest warnings to you, if you need validation, do checks and stuff before using this macro. I suffered enough writing it.
 */
#define KR_MAKE_VIRTUAL(IndexPML4, IndexPDPT, IndexPD, IndexPT, Offset) \
    ( ( UINTPTR ) ( ((QWORD)((-((QWORD)((IndexPML4) >> 8) & ((QWORD)(1))) & ((QWORD)(0xFFFF000000000000))))) | \
    ( ( ( QWORD ) ( IndexPML4 ) ) & 0x1FF ) << 39 ) | ( ( ( ( QWORD ) ( IndexPDPT ) ) & 0x1FF ) << 30 ) | \
    ( ( ( ( QWORD ) ( IndexPD ) ) & 0x1FF ) << 21 ) | ( ( ( ( QWORD ) ( IndexPT ) ) & 0x1FF ) << 12 ) | \
    ( ( ( ( QWORD ) ( Offset ) ) ) ) )

#endif // !YCH_KERNEL_MEMORY_PRIVATE_VMM_MAKE_VIRT_H
