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
KR_STATIC_ASSERT(sizeof(KrVirtualMemoryRegion) <= KR_PRIMITIVE_HEAP_STEPPING, "struct KrVirtualMemoryRegion does not fit within a Primitive Heap Stepping.");

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

    if (!KrPrimitiveInit())
    {
        return FALSE;
    }

    // Create root node by hand
    // Root node will cover address 0 through 2MiB.
    // We create this reservation, so no fool can claim this space.
    g_RootVMR.VirtAddrBase = 0;
    g_RootVMR.szPageCount  = 1;
    g_RootVMR.wAcquisitionType = 0;
    g_RootVMR.wFlags = KR_PAGE_FLAG_LARGE_PAGE; // One 2MiB page.
    g_RootVMR.pPrev = NULLPTR;
    g_RootVMR.pNext = NULLPTR;

    return TRUE;
}

BOOL KrMapVirt(UINTPTR pAddrVirt, UINTPTR pAddrPhys, SIZE szRegionSize, WORD wAcquisitionType, WORD wFlags)
{
    KR_UNUSED(pAddrVirt);
    KR_UNUSED(pAddrPhys);
    KR_UNUSED(szRegionSize);
    KR_UNUSED(wAcquisitionType);
    KR_UNUSED(wFlags);

    // TODO: Now we can code this.

    return FALSE;
}

UINTPTR KrPhysToVirt(UINTPTR AddrPhys)
{
    return g_StateVMM.AddrDirectMapBase + AddrPhys;
}

const KrVirtmemmgmtState* GetVirtmemmgmtState(VOID)
{
    return &g_StateVMM;
}
