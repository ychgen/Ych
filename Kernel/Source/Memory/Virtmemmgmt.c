#include "Memory/Virtmemmgmt.h"

#include "Core/Krnlmeltdown.h"
#include "Core/KernelState.h"

#include "CPU/CPUID.h"
#include "CPU/MSR.h"

#include "Memory/BootstrapArena.h"
#include "Memory/Physmemmgmt.h"

#include "KRTL/Krnlmem.h"

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

KrVirtmemmgmtState    g_StateVMM;
KrVirtualMemoryRegion g_RootVMR;

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

    if (!KrInitDirectMap())
    {
        return FALSE;
    }

    return TRUE;
}

BOOL KrMapVirt(const VOID* pVirt, const VOID* pPhys, SIZE szRegionSize, DWORD dwAcquisitionType, DWORD dwFlags)
{
    return FALSE;
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
