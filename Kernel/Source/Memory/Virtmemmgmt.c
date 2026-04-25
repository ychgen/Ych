#include "Memory/Virtmemmgmt.h"

#include "Core/Krnlmeltdown.h"
#include "Core/KernelState.h"

#include "CPU/CPUID.h"
#include "CPU/MSR.h"

#include "Memory/BootstrapArena.h"
#include "Memory/PrimitiveHeap.h"
#include "Memory/Physmemmgmt.h"

#include "KRTL/Krnlmem.h"

// Important stuff check (dead serious)
KR_STATIC_ASSERT(sizeof(KrVirtualMemoryRegion) <= KR_PRIMITIVE_HEAP_STEPPING, "struct `KrVirtualMemoryRegion` does not fit within a Primitive Heap 'Stepping'.");

#define KR_DIRECT_MAP_PML4_INDEX          256 // Direct mapping starts at this PML4 index.
#define KR_RECURSIVE_PML4_INDEX           510 // PML4[KR_RECURSIVE_PML4_INDEX] = PHYSICAL_OF(PML4)
#define KR_KERNEL_RESERVED_PML4_INDEX     511

#define KR_KERNEL_PDPT_KERNEL_INDEX       510
#define KR_KERNEL_PDPT_FRAME_BUFFER_INDEX 511

KrVirtmemmgmtState    g_StateVMM;
KrVirtualMemoryRegion g_RootVMR;

// Include these here (they depend on stuff from above)
#include "PrivateVMM/Smapinit.h" // for KrInitStaticPages()
#include "PrivateVMM/DmapInit.h" // for KrInitDirectMap()

inline UINT KrGetRegionPageGranularity(WORD wRegionFlags)
{
    return wRegionFlags & KR_PAGE_FLAG_LARGE_PAGE ? 0x200000 : 0x1000;
}

BOOL KrInitVirtmemmgmt(VOID)
{
    // See if we are already initialized or sumn
    if (g_PML4[KR_KERNEL_RESERVED_PML4_INDEX])
    {
        return FALSE;
    }

    // Huge Page Support Check & NX Support Check + Activation
    {
        DWORD EAX, EBX, ECX, EDX;
        KrCPUID(KR_CPUID_LEAF_EXT_FEAT_INFO, EAX, EBX, ECX, EDX);

        if (EDX & KR_CPUID_FEAT_EDX_PDPE1GB)
        {
            g_StateVMM.bHugePageSupport = TRUE;
        }

        if (EDX & KR_CPUID_FEAT_EDX_NX_BIT)
        {
            QWORD msrEFER = KrReadModelSpecificRegister(KR_MSR_IA32_EFER);
            msrEFER |= KR_MSR_IA32_EFER_NXE;
            KrWriteModelSpecificRegister(KR_MSR_IA32_EFER, msrEFER);
            // Used by encode PTE functions
            g_StateVMM.bNoExecuteSupport = TRUE;
        }
    }

    // This keeps our current mapping and unmapping identity-mapped lower 2MiB (L bozo).
    KrInitStaticPages();

    // Take control of paging.
    UINTPTR AddrPhysicalPML4 = KrReservedVirtToPhys(g_PML4);
    __asm__ __volatile__ ("mov %0, %%cr3\n\t" : : "r"(AddrPhysicalPML4) : "memory");

    // Our paging configuration must be active before calling this. It depends on using pages it maps like scaffolding once it falls out of the bootstrap arena.
    // It also directly works on g_PML4 and such.
    if (!KrInitDirectMap())
    {
        return FALSE;
    }

    // Initialize `Primitive Heap`, needed to allocate VirtualMemoryRegion nodes.
    if (!KrPrimitiveInit())
    {
        return FALSE;
    }

    // Create root node by hand
    // Root node will cover address 0 through 2MiB.
    // We create this reservation, so no fool can claim this space.
    // Reason is: No one should map NULLPTR otherwise I am coming after them.
    // Reason for anything other than NULLPTR: my dick wanted it that way so it is the way it is.
    g_RootVMR.VirtAddrBase = 0; // Virtual address 0x0000000000000000, so like, NULLPTR and shit.
    g_RootVMR.szPageCount  = 1; // 1 page.
    g_RootVMR.wAcquisitionType = KR_PAGE_ACQUIRE_RESERVE; // Do not commit, duh.
    g_RootVMR.wFlags = KR_PAGE_FLAG_HARD_RESERVE | KR_PAGE_FLAG_LARGE_PAGE; // One `2MiB` page. Also hard reserve so page is inaccessible.
    g_RootVMR.uProcID = 0; // 0 = Invalid/HardHard Reserve, 1 = Kernel.
    g_RootVMR.pPrev = NULLPTR; // We are the root node, duh.
    g_RootVMR.pNext = NULLPTR; // Soon to come.

    // TODO: Map nicely when we indeed have Code and Data segment and stuff separated with our custom format.
    if (!KrMapVirt(g_StateVMM.VirtAddrKernel, g_KernelState.LoadInfo.AddrPhysicalBase, g_KernelState.LoadInfo.ReserveSize / 0x200000, KR_PAGE_ACQUIRE_RESERVE, KR_PAGE_FLAG_HARD_RESERVE | KR_PAGE_FLAG_LARGE_PAGE))
    {
        return FALSE;
    }

    return TRUE;
}

KrMapResult KrMapVirt(UINTPTR pAddrVirt, UINTPTR pAddrPhys, SIZE szRegionSize, WORD wAcquisitionType, WORD wFlags)
{
    // Check contradictory flags
    if ((wFlags & KR_PAGE_FLAG_WRITE_COMBINE) && (wFlags & KR_PAGE_FLAG_UNCACHEABLE))
    {
        return KR_MAP_RESULT_CONTRADICTION;
    }
    if (wAcquisitionType != KR_PAGE_ACQUIRE_RESERVE && (wFlags & KR_PAGE_FLAG_HARD_RESERVE))
    {
        return KR_MAP_RESULT_CONTRADICTION;
    }

    const UINT PageGranularity = KrGetRegionPageGranularity(wFlags);
    const SIZE PageCount = KR_CEILDIV(szRegionSize, PageGranularity);
    const SIZE ByteCount = PageCount * PageGranularity; 
    const UINTPTR pAddrVirtEnd = pAddrVirt + ByteCount;

    KrVirtualMemoryRegion* pRegion = &g_RootVMR;
    while (pRegion)
    {
        const UINT RegionPageGranularity = KrGetRegionPageGranularity(pRegion->wFlags);
        const UINTPTR pRegionEnd = pRegion->VirtAddrBase + pRegion->szPageCount * RegionPageGranularity;
        
        if (pAddrVirt == pRegion->VirtAddrBase || (pAddrVirt > pRegion->VirtAddrBase && pAddrVirtEnd < pRegionEnd))
        {
            break;
        }

        pRegion = pRegion->pNext;
    }

    // Region set means that we didn't exhaust the entire chain, therefore the check condition failed.
    if (pRegion)
    {
        return KR_MAP_RESULT_SPACE_ALREADY_TAKEN;
    }

    return FALSE;
}

UINTPTR KrPhysToVirt(UINTPTR AddrPhys)
{
    return g_StateVMM.VirtAddrDmapBase + AddrPhys;
}

const KrVirtmemmgmtState* GetVirtmemmgmtState(VOID)
{
    return &g_StateVMM;
}
