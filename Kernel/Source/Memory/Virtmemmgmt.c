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

// Include these here (depends on symbol defs and stuff from above)
#include "PrivateVMM/Smapinit.h" // for KrInitStaticPages()
#include "PrivateVMM/DmapInit.h" // for KrInitDirectMap()

BOOL KrInitVirtmemmgmt(VOID)
{
    // See if we are already initialized or sumn
    if (g_PML4[KR_KERNEL_RESERVED_PML4_INDEX])
    {
        return FALSE;
    }

    // This keeps our current mapping and unmapping identity-mapped lower 2MiB (L bozo).
    KrInitStaticPages();

    // Take control of paging.
    UINTPTR AddrPhysicalPML4 = KrReservedVirtToPhys(g_PML4);
    __asm__ __volatile__ ("mov %0, %%cr3\n\t" : : "r"(AddrPhysicalPML4) : "memory");

    if (!KrInitDirectMap())
    {
        return FALSE;
    }

    // Let's create... nodes...


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
