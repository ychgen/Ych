#ifndef YCH_KERNEL_MEMORY_PRIVATE_VMM_SMAP_INIT_H
#define YCH_KERNEL_MEMORY_PRIVATE_VMM_SMAP_INIT_H

#include "PTE.h"

// Converts a virtual address from the reserved area to physical
#define KrReservedVirtToPhys(Virt) (g_KernelState.LoadInfo.AddrPhysicalBase + ((UINTPTR)(Virt) - g_KernelState.LoadInfo.AddrVirtualBase))
// Converts a physical address from the reserved area to virtual
#define KrReservedPhysToVirt(Phys) (g_KernelState.LoadInfo.AddrVirtualBase  + ((UINTPTR)(Phys) - g_KernelState.LoadInfo.AddrPhysicalBase))

// The PML4 (Page Map Level 4) paging structure, always fixed here.
// Contains physical addresses of PDPTs.
KR_ALIGNED(KR_PAGE_STRUCTURE_SIZE)
KrPageTableEntry g_PML4[KR_PAGE_STRUCTURE_ENTRY_COUNT];          // The PML4

KR_ALIGNED(KR_PAGE_STRUCTURE_SIZE)
KrPageTableEntry g_KernelPDPT[KR_PAGE_STRUCTURE_ENTRY_COUNT];    // Referred as g_PML4[KR_KERNEL_RESERVED_PML4_INDEX]

KR_ALIGNED(KR_PAGE_STRUCTURE_SIZE)
KrPageTableEntry g_KernelPD[KR_PAGE_STRUCTURE_ENTRY_COUNT];      // Referred as g_KernelPDPT[KR_KERNEL_PDPT_KERNEL_INDEX]

KR_ALIGNED(KR_PAGE_STRUCTURE_SIZE)
KrPageTableEntry g_FrameBufferPD[KR_PAGE_STRUCTURE_ENTRY_COUNT]; // Referred as g_KernelPDPT[KR_KERNEL_PDPT_FRAME_BUFFER_INDEX]

// Initializes static mapping to map kernel as-is.
static VOID KrInitStaticPages(VOID)
{
    const UINT TWOMIB = 2 * 1024 * 1024;

    g_StateVMM.VirtAddrDmapBase      = KR_MAKE_VIRTUAL(KR_DIRECT_MAP_PML4_INDEX, 0, 0, 0, 0);
    g_StateVMM.VirtAddrRecursiveBase = KR_MAKE_VIRTUAL(KR_RECURSIVE_PML4_INDEX, 0, 0, 0, 0);
    g_StateVMM.VirtAddrKernel        = KR_MAKE_VIRTUAL(KR_KERNEL_RESERVED_PML4_INDEX, KR_KERNEL_PDPT_KERNEL_INDEX, 0, 0, 0);
    
    // Set up static mapping (beware, we dont id-map lower-2MiB like Boot Elevate. So this is the moment we abandon idmapping of lower memory.)
    {
        KrPageTableEntryFlags Flags = {0};
        Flags.bPresent  = TRUE;
        Flags.bWritable = TRUE;

        // Recursive map to access PTEs with ease.
        g_PML4      [KR_RECURSIVE_PML4_INDEX          ] = KrEncodePageTableEntry(KrReservedVirtToPhys(g_PML4),          Flags);

        g_PML4      [KR_KERNEL_RESERVED_PML4_INDEX    ] = KrEncodeLargePageEntry(KrReservedVirtToPhys(g_KernelPDPT),    Flags, 0);
        g_KernelPDPT[KR_KERNEL_PDPT_KERNEL_INDEX      ] = KrEncodeLargePageEntry(KrReservedVirtToPhys(g_KernelPD),      Flags, 0);
        g_KernelPDPT[KR_KERNEL_PDPT_FRAME_BUFFER_INDEX] = KrEncodeLargePageEntry(KrReservedVirtToPhys(g_FrameBufferPD), Flags, 0);

    }
    
    // We map the kernel using Large Pages
    {
        // Ceil divide
        UINT szKernelPageCount = KR_CEILDIV(g_KernelState.LoadInfo.ReserveSize, TWOMIB);

        KrPageTableEntryFlags Flags = {0};
        Flags.bPresent  = TRUE;
        Flags.bWritable = TRUE;
        Flags.PS        = TRUE; // Large Page (2MiB in case of PD)

        for (UINT i = 0; i < szKernelPageCount; i++)
        {
            g_KernelPD[i] = KrEncodeLargePageEntry(g_KernelState.LoadInfo.AddrPhysicalBase + (i * TWOMIB), Flags, 0);
        }
    }
    // We map frame buffer using Large Pages as well
    {
        UINT szFrameBufferPageCount = KR_CEILDIV(g_KernelState.FrameBufferInfo.Size, TWOMIB);

        if (!szFrameBufferPageCount)
        {
            szFrameBufferPageCount++; // at least one 2MiB page.
        }
        
        KrPageTableEntryFlags Flags = {0};
        Flags.bPresent  = TRUE;
        Flags.bWritable = TRUE;
        Flags.PS        = TRUE; // Large Page (2MiB in case of PD)

        for (UINT i = 0; i < szFrameBufferPageCount; i++)
        {
            g_FrameBufferPD[i] = KrEncodeLargePageEntry(g_KernelState.FrameBufferInfo.PhysicalAddress + (i * TWOMIB), Flags, 1); // WC
        }
    }
}

#endif // !YCH_KERNEL_MEMORY_PRIVATE_VMM_SMAP_INIT_H
