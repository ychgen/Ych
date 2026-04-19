#include "Memory/Virtmemmgmt.h"
#include "PTE.h"

#include "Core/Krnlmeltdown.h"
#include "Core/KernelState.h"

#include "Memory/BootstrapArena.h"
#include "Memory/Physmemmgmt.h"

#include "KRTL/Krnlmem.h"

// Converts a virtual address from the reserved area to physical
#define KrReservedVirtToPhys(Virt) (g_KernelState.LoadInfo.AddrPhysicalBase + ((UINTPTR)(Virt) - g_KernelState.LoadInfo.AddrVirtualBase))
// Converts a physical address from the reserved area to virtual
#define KrReservedPhysToVirt(Phys) (g_KernelState.LoadInfo.AddrVirtualBase  + ((UINTPTR)(Phys) - g_KernelState.LoadInfo.AddrPhysicalBase))

#define KR_DIRECT_MAP_PML4_INDEX          256 // Direct mapping starts at this PML4 index.
#define KR_RECURSIVE_PML4_INDEX           510 // PML4[KR_RECURSIVE_PML4_INDEX] = PHYSICAL_OF(PML4)
#define KR_KERNEL_RESERVED_PML4_INDEX     511

#define KR_KERNEL_PDPT_KERNEL_INDEX       510
#define KR_KERNEL_PDPT_FRAME_BUFFER_INDEX 511

typedef struct KrVirtualMemoryRegion
{
    VOID* pBaseAddress;
    SIZE  szPageCount;
    DWORD dwAcquisitionType;
    DWORD dwFlags;

    struct KrVirtualMemoryRegion* pPrev; // Previous Node
    struct KrVirtualMemoryRegion* pNext; // Next Node
} KrVirtualMemoryRegion;

// VMM State
KrVirtmemmgmtState g_StateVMM;

/** The root node within the VMR linked list. */
KrVirtualMemoryRegion g_RootVMR;

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

QWORD* KrGetPTE(const VOID* pVirtual)
{
    return (QWORD*)(g_StateVMM.AddrRecursiveMapBase + ((((UINTPTR) pVirtual) >> 9) & 0x7FFFFFFFF8));
}

#include "DmapInit.h" // Include here (depends on symbol defs from above)

// Initializes static mapping to map kernel as-is.
VOID KrInitStaticPages(VOID)
{
    const UINT TWOMIB = 2 * 1024 * 1024;
    
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

        g_StateVMM.AddrDirectMapBase    = KR_MAKE_VIRTUAL(KR_DIRECT_MAP_PML4_INDEX, 0, 0, 0, 0);
        g_StateVMM.AddrRecursiveMapBase = KR_MAKE_VIRTUAL(KR_RECURSIVE_PML4_INDEX, 0, 0, 0, 0);
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

BOOL KrInitVirtmemmgmt(VOID)
{
    // See if we are already initialized or sumn
    if (g_PML4[KR_KERNEL_RESERVED_PML4_INDEX])
    {
        return FALSE;
    }

    // This keeps our current mapping and unmapping identity-mapped lower 2MiB.
    KrInitStaticPages();

    // Take control of paging.
    UINTPTR AddrPhysicalPML4 = KrReservedVirtToPhys(g_PML4);
    __asm__ __volatile__ ("mov %0, %%cr3\n\t" : : "r"(AddrPhysicalPML4) : "memory");

    if (!KrInitDirectMap())
    {
        return FALSE;
    }

    return TRUE;
}

VOID* KrAcquireVirt(const VOID* pHintAddress, SIZE szRegionSize, DWORD dwAcquisitionType, DWORD dwFlags)
{
    KR_UNUSED(pHintAddress);
    KR_UNUSED(szRegionSize);
    KR_UNUSED(dwAcquisitionType);
    KR_UNUSED(dwFlags);
    return NULLPTR;
}

BOOL KrRelinquishVirt(const VOID* pBaseAddress, SIZE szRegionSize, DWORD dwOperation)
{
    KR_UNUSED(pBaseAddress);
    KR_UNUSED(szRegionSize);
    KR_UNUSED(dwOperation);
    return FALSE;
}

VOID* KrPhysicalToVirtual(VOID* pAddrPhysical)
{
    return (BYTE*) g_StateVMM.AddrDirectMapBase + (UINTPTR) pAddrPhysical;
}

const KrVirtmemmgmtState* GetVirtmemmgmtState(VOID)
{
    return &g_StateVMM;
}
