/**
 * Private header for Virtmemmgmt.c
 * 
 * `B`oot`S`trap Map Init (BSMapInit.h)
 */

#ifndef YCH_KERNEL_MEMORY_PRIVATE_VMM_BSMAP_INIT_H
#define YCH_KERNEL_MEMORY_PRIVATE_VMM_BSMAP_INIT_H

#include "Memory/PTEV2.h"
#include "PTE.h"

// Converts a virtual address from the reserved area to physical
#define KrReservedVirtToPhys(Virt) (g_KernelState.LoadInfo.AddrPhysicalBase + ((UINTPTR)(Virt) - g_KernelState.LoadInfo.AddrVirtualBase))
// Converts a physical address from the reserved area to virtual
#define KrReservedPhysToVirt(Phys) (g_KernelState.LoadInfo.AddrVirtualBase  + ((UINTPTR)(Phys) - g_KernelState.LoadInfo.AddrPhysicalBase))

// The PML4 (Page Map Level 4) paging structure, always fixed here.
// Contains physical addresses of PDPTs.
KR_ALIGNED(KR_PAGE_STRUCTURE_SIZE)
PTE g_PML4[KR_PAGE_STRUCTURE_ENTRY_COUNT];          // The PML4

KR_ALIGNED(KR_PAGE_STRUCTURE_SIZE)
PTE g_KernelPDPT[KR_PAGE_STRUCTURE_ENTRY_COUNT];    // Referred as g_PML4[KR_KERNEL_RESERVED_PML4_INDEX]

KR_ALIGNED(KR_PAGE_STRUCTURE_SIZE)
PTE g_KernelPD[KR_PAGE_STRUCTURE_ENTRY_COUNT];      // Referred as g_KernelPDPT[KR_KERNEL_PDPT_KERNEL_INDEX]

KR_ALIGNED(KR_PAGE_STRUCTURE_SIZE)
PTE g_FrameBufferPD[KR_PAGE_STRUCTURE_ENTRY_COUNT]; // Referred as g_KernelPDPT[KR_KERNEL_PDPT_FRAME_BUFFER_INDEX]

// Initializes static mapping to map kernel as-is.
static VOID KrInitBootstrapStaticPages(VOID)
{
    const UINT TWOMIB = 2 * 1024 * 1024;

    g_StateVMM.DmapInfo.VirtAddrBase = KR_MAKE_VIRTUAL(KR_DIRECT_MAP_PML4_INDEX, 0, 0, 0, 0);
    g_StateVMM.VirtAddrRecursiveBase = KR_MAKE_VIRTUAL(KR_RECURSIVE_PML4_INDEX, 0, 0, 0, 0);
    g_StateVMM.VirtAddrKernel        = KR_MAKE_VIRTUAL(KR_KERNEL_RESERVED_PML4_INDEX, KR_KERNEL_PDPT_KERNEL_INDEX, 0, 0, 0);
    
    // Set up static mapping (beware, we dont id-map lower-2MiB like Boot Elevate. So this is the moment we abandon idmapping of lower memory.)
    {
        QWORD qwFlags = KR_PTE_PRESENT | KR_PTE_WRITABLE;

        // Recursive map to access PTEs with ease.
        g_PML4      [KR_RECURSIVE_PML4_INDEX          ] = KrpteEncodeEntry(PML4_ENTRY, KrReservedVirtToPhys(g_PML4),       qwFlags, g_pslDefault);
        g_PML4      [KR_KERNEL_RESERVED_PML4_INDEX    ] = KrpteEncodeEntry(PML4_ENTRY, KrReservedVirtToPhys(g_KernelPDPT), qwFlags, g_pslDefault);

        g_KernelPDPT[KR_KERNEL_PDPT_KERNEL_INDEX      ] = KrpteEncodeEntry(PDPT_ENTRY, KrReservedVirtToPhys(g_KernelPD),      qwFlags, g_pslDefault);
        g_KernelPDPT[KR_KERNEL_PDPT_FRAME_BUFFER_INDEX] = KrpteEncodeEntry(PDPT_ENTRY, KrReservedVirtToPhys(g_FrameBufferPD), qwFlags, g_pslDefault);
    }
    
    // We map the kernel using Large Pages
    {
        // Ceil divide
        UINT szKernelPageCount = KR_CEILDIV(g_KernelState.LoadInfo.ReserveSize, TWOMIB);

        QWORD qwFlags = KR_PTE_PRESENT | KR_PTE_WRITABLE;
        for (UINT i = 0; i < szKernelPageCount; i++)
        {
            g_KernelPD[i] = KrpteEncodeEntry(PD2MB_ENTRY, g_KernelState.LoadInfo.AddrPhysicalBase + (i * TWOMIB), qwFlags, g_pslDefault);
        }
    }
    // We map frame buffer using Large Pages as well
    {
        UINT szFrameBufferPageCount = KR_CEILDIV(g_KernelState.FrameBufferInfo.Size, TWOMIB);

        if (!szFrameBufferPageCount)
        {
            szFrameBufferPageCount++; // at least one 2MiB page.
        }
        
        QWORD qwFlags = KR_PTE_PRESENT | KR_PTE_WRITABLE;
        KrPatSelect pslWC = KrSelectPat(KR_PAT_WRITE_COMBINING);
        for (UINT i = 0; i < szFrameBufferPageCount; i++)
        {
            g_FrameBufferPD[i] = KrpteEncodeEntry(PD2MB_ENTRY, g_KernelState.FrameBufferInfo.PhysicalAddress + (i * TWOMIB), qwFlags, pslWC);
        }
    }
}

#endif // !YCH_KERNEL_MEMORY_PRIVATE_VMM_BSMAP_INIT_H
