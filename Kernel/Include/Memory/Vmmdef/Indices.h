#ifndef YCH_KERNEL_MEMORY_PRIVATE_VMM_INDICES_H
#define YCH_KERNEL_MEMORY_PRIVATE_VMM_INDICES_H

#define DMAP_PML4_IDX        256 // Direct mapping starts at this PML4 index.
#define KRNL_PML4_IDX        511 // This index (along with the PDPT one) was chosen to give the kernel binary the top 2 GiB of the address space for -mcmodel=kernel.

/* Entries for KRNL_PML4_IDX */
#define KRNL_BINARY_PDPT_IDX 510
#define KRNL_VIDEO_FBUF_IDX  511

#endif // !YCH_KERNEL_MEMORY_PRIVATE_VMM_INDICES_H
