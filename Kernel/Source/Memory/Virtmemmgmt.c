#include "Memory/Virtmemmgmt.h"
#include "PTE.h"

#include "Core/KernelState.h"

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

/** The root node within the VMR linked list. Unused on its own, RootVMR.pNext is the real one that starts the chain. */
KrVirtualMemoryRegion g_RootVMR;

// The PML4 (Page Map Level 4) paging structure, always fixed here.
// Contains physical addresses of PDPTs.
KR_SECTION(".data") KR_ALIGNED(KR_PAGE_STRUCTURE_SIZE)
KrPageTableEntry g_PML4[KR_PAGE_STRUCTURE_ENTRY_COUNT];          // The PML4

KR_SECTION(".data") KR_ALIGNED(KR_PAGE_STRUCTURE_SIZE)
KrPageTableEntry g_KernelPDPT[KR_PAGE_STRUCTURE_ENTRY_COUNT];    // Referred as g_PML4[KR_KERNEL_RESERVED_PML4_INDEX]

KR_SECTION(".data") KR_ALIGNED(KR_PAGE_STRUCTURE_SIZE)
KrPageTableEntry g_KernelPD[KR_PAGE_STRUCTURE_ENTRY_COUNT];      // Referred as g_KernelPDPT[KR_KERNEL_PDPT_KERNEL_INDEX]

KR_SECTION(".data") KR_ALIGNED(KR_PAGE_STRUCTURE_SIZE)
KrPageTableEntry g_FrameBufferPD[KR_PAGE_STRUCTURE_ENTRY_COUNT]; // Referred as g_KernelPDPT[KR_KERNEL_PDPT_FRAME_BUFFER_INDEX]
/* PTEs above are deliberately put in `.data` so they physically exist within addressable binary. */

// Initializes static mapping to map kernel as-is.
VOID KrInitStaticPages(VOID)
{
    const UINTPTR AddrKrnlPhysBase = g_KernelState.LoadInfo.AddrPhysicalBase;
    const UINTPTR AddrKrnlVirtBase = g_KernelState.LoadInfo.AddrVirtualBase;
    const UINT TWOMIB = 2 * 1024 * 1024;
    
    // Set up static mapping (beware, we dont id-map lower-2MiB like Boot Elevate. So this is the moment we abandon idmapping of lower memory.)
    {
        KrPageTableEntryFlags Flags = {0};
        Flags.bPresent    = TRUE;
        Flags.bSupervisor = TRUE;
        Flags.bWritable   = TRUE;

        UINTPTR AddrKernelPDPT    = AddrKrnlPhysBase + (QWORD)(((UINTPTR) g_KernelPDPT   ) - AddrKrnlVirtBase);
        UINTPTR AddrKernelPD      = AddrKrnlPhysBase + (QWORD)(((UINTPTR) g_KernelPD     ) - AddrKrnlVirtBase);
        UINTPTR AddrFrameBufferPD = AddrKrnlPhysBase + (QWORD)(((UINTPTR) g_FrameBufferPD) - AddrKrnlVirtBase);

        g_PML4      [KR_KERNEL_RESERVED_PML4_INDEX    ] = KrEncodePageTableEntry(AddrKernelPDPT,    Flags);
        g_KernelPDPT[KR_KERNEL_PDPT_KERNEL_INDEX      ] = KrEncodePageTableEntry(AddrKernelPD,      Flags);
        g_KernelPDPT[KR_KERNEL_PDPT_FRAME_BUFFER_INDEX] = KrEncodePageTableEntry(AddrFrameBufferPD, Flags);
    }
    
    // We map the kernel using Large Pages
    {
        // Ceil divide
        UINT szKernelPageCount = KR_CEILDIV(g_KernelState.LoadInfo.ReserveSize, TWOMIB);

        KrPageTableEntryFlags Flags = {0};
        Flags.bPresent    = TRUE;
        Flags.bWritable   = TRUE;
        Flags.bSupervisor = TRUE; // Supervisor (Kernel)
        Flags.PS          = TRUE; // Large Page (2MiB in case of PD)

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
        Flags.bPresent    = TRUE;
        Flags.bWritable   = TRUE;
        Flags.bSupervisor = TRUE; // Supervisor (Kernel). Apps cannot write to frame buffer directly, they must use Kernel Graphics Interface. Kernel conducts the transaction.
        Flags.PS          = TRUE; // Large Page (2MiB in case of PD)

        for (UINT i = 0; i < szFrameBufferPageCount; i++)
        {
            g_FrameBufferPD[i] = KrEncodeLargePageEntry(g_KernelState.FrameBufferInfo.PhysicalAddress + (i * TWOMIB), Flags, 1); // WC
        }
    }

    UINTPTR AddrPhysicalPM4 = AddrKrnlPhysBase + (((UINTPTR) g_PML4) - AddrKrnlVirtBase);
    __asm__ __volatile__ ("mov %0, %%cr3\n\t" : : "r"(AddrPhysicalPM4) : "memory");
}

BOOL KrInitVirtmemmgmt(VOID)
{
    if (g_PML4[KR_KERNEL_RESERVED_PML4_INDEX])
    {
        return FALSE;
    }

    // Static pages initialization
    KrInitStaticPages();

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
