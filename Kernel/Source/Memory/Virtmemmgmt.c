#include "Memory/Virtmemmgmt.h"

#define KR_PAGE_STRUCTURE_ENTRY_COUNT     512
#define KR_PAGE_STRUCTURE_ENTRY_CONT_SIZE 8
#define KR_PAGE_STRUCTURE_SIZE            ( KR_PAGE_STRUCTURE_ENTRY_COUNT * KR_PAGE_STRUCTURE_ENTRY_CONT_SIZE ) // 512 * 8 = 4 KiB per structure like PML4, PDPT, PD and PT.

typedef struct
{
    UINT bPresent    : 1;
    UINT bWritable   : 1;
    UINT bSupervisor : 1;
    UINT PWT         : 1;
    UINT PCD         : 1;
    UINT PAT_PS      : 1;
    UINT bGlobal     : 1;
    UINT bNoExecute  : 1;
} KrPageTableEntryFlags;
typedef QWORD KrPageTableEntry;

KrPageTableEntry KrEncodePageTableEntry
(
    UINTPTR pPhysicalAddress,
    const KrPageTableEntryFlags* pFlags
)
{
    return (KrPageTableEntry)
        ((QWORD)(pFlags->bNoExecute) << 63) | ((QWORD)(0) << 52) | (((QWORD)(pPhysicalAddress) & 0xFFFFFFFFFF) << 12) |
        ((QWORD)(0) << 9) | ((QWORD)(pFlags->bGlobal) << 8) | ((QWORD)(pFlags->PAT_PS) << 7) | ((QWORD)(0) << 5) |
        ((QWORD)(pFlags->PCD) << 4) | ((QWORD)(pFlags->PWT) << 3) | ((QWORD)(pFlags->bSupervisor) << 2) |
        ((QWORD)(pFlags->bWritable) << 1) | (QWORD)(pFlags->bPresent);
}

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
KR_ALIGNED(KR_PAGE_STRUCTURE_SIZE) KrPageTableEntry g_PML4[KR_PAGE_STRUCTURE_ENTRY_COUNT];

BOOL g_bVirtmemmgmt;

BOOL KrInitVirtmemmgmt(VOID)
{
    if (g_bVirtmemmgmt)
    {
        return FALSE;
    }
    g_bVirtmemmgmt = TRUE;
    
    return TRUE;
}

VOID* KrAcquireVirt(const VOID* pHintAddress, SIZE szRegionSize, DWORD dwAcquisitionType, DWORD dwFlags)
{
    return NULLPTR;
}

BOOL KrRelinquishVirt(const VOID* pBaseAddress, SIZE szRegionSize, DWORD dwOperation)
{
    return FALSE;
}
