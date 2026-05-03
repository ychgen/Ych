#include "Memory/Physmemmgmt.h"

#include "Core/KernelState.h"

#include "Memory/BootstrapArena.h"
#include "KRTL/Krnlmem.h"

#define KR_PHYSICAL_PAGE_STATUS_AVAILABLE   0
#define KR_PHYSICAL_PAGE_STATUS_UNAVAILABLE 1

// Current PMM state information.
static KrPhysmemmgmtState g_StatePMM;
// TRUE if PMM is initialized, FALSE otherwise.
static BOOL g_bInitPMM = FALSE;

// The advisory bitmap, once initialized, is mostly static. Its primary job is to prevent reserved pages from being relinquished.
static BYTE*   g_pAdvisoryBitmap;
// The dynamic bitmap that changes with acquisitions and relinquishments.
static BYTE*   g_pPrimaryBitmap;

// Size of the bitmap in bytes, this is the size of both `g_pPrimaryBitmap` and `g_pAdvisoryBitmap`.
static SIZE    g_szBitmap;
// Number of page entries in the bitmap itself, not ones described by the map.
static ULONG   g_NumPages;
// Highest address ever discovered, calculated via `Base + PageCount * PageSize` on a map entry.
static UINTPTR g_PhysAddrHighest;

// Starting from page idStart, it sets N pages to Status.
// An internal function, doesn't care about permissions, sets directly.
static BOOL KrSetPhysicalPageStatus(BYTE* pBitmap, PAGEID idStart, SIZE N, BYTE Status);

BOOL KrInitPhysmemmgmt(void)
{
    if (g_bInitPMM)
    {
        return FALSE;
    }

    // 1st pass ; discovery
    for (QWORD i = 0; i < g_KernelState.MemoryMapInfo.EntryCount; i++)
    {
        KrMemoryDescriptor* pDesc = g_KernelState.MemoryMap + i;
        
        // This is the logical physical pages.
        g_StatePMM.TotalPages += pDesc->PageCount;

        if (KrIsUsableMemoryRegionType(pDesc->Type))
        {
            UINTPTR AddrRegionEnd = pDesc->PhysicalBase + pDesc->PageCount * KR_PAGE_SIZE;
            if (AddrRegionEnd > g_PhysAddrHighest)
            {
                g_PhysAddrHighest = AddrRegionEnd;
            }
        }
        else
        {
            g_StatePMM.UnusablePages += pDesc->PageCount;
        }
    }

    // noPages is based on highest addressable point.
    g_NumPages        = (g_PhysAddrHighest + KR_PAGE_SIZE - 1) / KR_PAGE_SIZE;
    g_szBitmap        = (g_NumPages + 7) / 8;
    g_pAdvisoryBitmap = KrBootstrapArenaAcquire(g_szBitmap);

    if (!g_pAdvisoryBitmap)
    {
        return FALSE;
    }

    // Initialize all pages as unavailable first, this way we are less likely to mess up.
    // 0xFF = All bits =1 which is what UNAVAILABLE is set to, =1.
    KrtlContiguousSetBuffer(g_pAdvisoryBitmap, 0xFF, g_szBitmap);

    // 2nd pass ; mark conventional memory and likewise areas as available.
    for (QWORD i = 0; i < g_KernelState.NumCanonicalMapEntries; i++)
    {
        KrMemoryDescriptor* pDesc = g_KernelState.CanonicalMemoryMap + i;

        SIZE idPage = pDesc->PhysicalBase / KR_PAGE_SIZE;
        if (!g_StatePMM.AcquireHint || g_StatePMM.AcquireHint == KR_INVALID_PAGEID)
        {
            // If no acquisition hint yet set, use this one.
            g_StatePMM.AcquireHint = idPage;
        }
        // Set entire range as available
        KrSetPhysicalPageStatus(g_pAdvisoryBitmap, idPage, pDesc->PageCount, KR_PHYSICAL_PAGE_STATUS_AVAILABLE);
    }

    // Mark kernel-reserved area as unavailable (must be done since kernel lives in conventional memory and code above marks all that as available)
    KrSetPhysicalPageStatus
    (
        g_pAdvisoryBitmap,
        g_KernelState.LoadInfo.AddrPhysicalBase / KR_PAGE_SIZE,
        g_KernelState.LoadInfo.ReserveSize      / KR_PAGE_SIZE,
        KR_PHYSICAL_PAGE_STATUS_UNAVAILABLE
    );

    // Mark all memory under 1MiB as unavailable.
    // There are two reasons for this:
    //   - We should always reserve the first page, so we can have sane null pointer semantics.
    //   - Under 1MiB is IBM PC cluster fuck area. Better to not wake up the 1980s ghosts.
    KrSetPhysicalPageStatus(g_pAdvisoryBitmap, 0, (1024 * 1024) / KR_PAGE_SIZE, KR_PHYSICAL_PAGE_STATUS_UNAVAILABLE);

    // Now we'll create the dynamic bitmap and copy the advisory one as its initial state.
    g_pPrimaryBitmap = KrBootstrapArenaAcquire(g_szBitmap);
    if (!g_pPrimaryBitmap)
    {
        return FALSE;
    }
    KrtlContiguousCopyBuffer(g_pPrimaryBitmap, g_pAdvisoryBitmap, g_szBitmap);

    g_bInitPMM = TRUE;
    return TRUE;
}

DWORD KrAcquirePhysicalPages(PAGEID idHint, DWORD dwAcquisitionMethod, PAGEID* pOutIDs, UINT uToAcquire)
{
    if (!uToAcquire)
    {
        return 0;
    }
    if (dwAcquisitionMethod > KR_PMM_ACQUIRE_DENSE)
    {
        return 0;
    }
    // This is a bit of a lie until near the end of the function where they are actually claimed.
    // Until then it acts more like a counter.
    UINT uNoAcquired = 0;

    PAGEID PageID = idHint == KR_INVALID_PAGEID ? (g_StatePMM.AcquireHint == KR_INVALID_PAGEID ? 0 : g_StatePMM.AcquireHint) : idHint;
    PAGEID InitialSearchID = PageID;
    BOOL   bIsReroll = FALSE;

    if (PageID >= g_NumPages)
    {
        return KR_INVALID_PAGEID;
    }
    
Hunt:
    for (SIZE i = PageID / 8; i < g_szBitmap && uNoAcquired < uToAcquire && (bIsReroll ? PageID < InitialSearchID : TRUE); i++)
    {
        BYTE* pRegion = g_pPrimaryBitmap + i;
        for (BYTE BitOffset = PageID % 8; BitOffset < 8 && uNoAcquired < uToAcquire; BitOffset++, PageID++)
        {
            BYTE RegionData = *pRegion;
            if (!(RegionData & (1 << BitOffset)) && !KrIsPhysicalPageReserved(PageID))
            {
                if
                (
                    uNoAcquired
                    &&
                    dwAcquisitionMethod == KR_PMM_ACQUIRE_DENSE
                    &&
                    pOutIDs[uNoAcquired - 1] != PageID - 1
                )
                {
                    uNoAcquired = 0;
                }
                pOutIDs[uNoAcquired++] = PageID;
            }
        }
    }

    if ((!bIsReroll && InitialSearchID != 0) && uNoAcquired < uToAcquire)
    {
        PageID = 0;
        bIsReroll = TRUE;
        goto Hunt;
    }

    if (!uNoAcquired)
    {
        return 0;
    }

    if (dwAcquisitionMethod == KR_PMM_ACQUIRE_DENSE)
    {
        KrSetPhysicalPageStatus(g_pPrimaryBitmap, *pOutIDs, uNoAcquired, KR_PHYSICAL_PAGE_STATUS_UNAVAILABLE);
    }
    else
    {
        for (UINT i = 0; i < uNoAcquired;)
        {
            UINT j;
            for (j = i + 1; j < uNoAcquired && pOutIDs[i] + 1 == pOutIDs[j]; j++);

            KrSetPhysicalPageStatus(g_pPrimaryBitmap, pOutIDs[i], j - i, KR_PHYSICAL_PAGE_STATUS_UNAVAILABLE);
            i = j;
        }
    }

    g_StatePMM.AcquireHint = pOutIDs[uNoAcquired - 1] + 1;
    return uNoAcquired;
}

PAGEID KrAcquirePhysicalPage(PAGEID idHint)
{
    PAGEID PageID = KR_INVALID_PAGEID;
    DWORD  dwAcquired = KrAcquirePhysicalPages(idHint, KR_PMM_ACQUIRE_SPARSE, &PageID, 1);
    return dwAcquired ? PageID : KR_INVALID_PAGEID;
}

BOOL KrRelinquishPhysicalPage(PAGEID idPage)
{
    // Cannot relinquish pages reserved during Physmemmgmt initialization or explicitly marked as reserved afterward.
    if (KrIsPhysicalPageReserved(idPage))
    {
        return FALSE;
    }

    SIZE Index = idPage / 8;
    if (Index >= g_szBitmap)
    {
        return FALSE;
    }
    BYTE BitOffset = idPage % 8;

    BYTE RegionData = g_pPrimaryBitmap[Index];
    if (RegionData & (1 << BitOffset))
    {
        g_pPrimaryBitmap[Index] &= ~(1 << BitOffset);
        g_StatePMM.AcquiredPages--;
        return TRUE;
    }

    return FALSE;
}

static BOOL KrSetPhysicalPageStatus(BYTE* pBitmap, PAGEID idStart, SIZE N, BYTE Status)
{
    if (idStart + N > g_NumPages)
    {
        return FALSE;
    }

    SIZE iCurrent = idStart;
    BYTE* pRegion = pBitmap + (iCurrent / 8);

    // Unaligned start
    SIZE BitOffset = iCurrent % 8;
    if (BitOffset)
    {
        BYTE Value = *pRegion;
        for (; BitOffset < 8 && N; BitOffset++, N--, iCurrent++)
        {
            if (Status)
            {
                Value |= (1 << BitOffset);
            }
            else
            {
                Value &= ~(1 << BitOffset);
            }
        }
        *pRegion++ = Value;
    }

    // Bulk writes
    while (N >= 64)
    {
        *((QWORD*) pRegion) = Status ? 0xFFFFFFFFFFFFFFFF : 0;
        pRegion += 8;
        iCurrent += 64;
        N -= 64;
    }
    while (N >= 32)
    {
        *((DWORD*) pRegion) = Status ? 0xFFFFFFFF : 0;
        pRegion += 4;
        iCurrent += 32;
        N -= 32;
    }
    while (N >= 16)
    {
        *((WORD*) pRegion) = Status ? 0xFFFF : 0;
        pRegion += 2;
        iCurrent += 16;
        N -= 16;
    }
    while (N >= 8)
    {
        *pRegion++ = Status ? 0xFF : 0x00;
        iCurrent += 8;
        N -= 8;
    }

    // Check unaligned end
    if (N)
    {
        BYTE Value = *pRegion;
        for (BitOffset = 0; N; BitOffset++, N--, iCurrent++)
        {
            if (Status)
            {
                Value |= (1 << BitOffset);
            }
            else
            {
                Value &= ~(1 << BitOffset);
            }
        }
        *pRegion = Value;
    }

    return TRUE;
}

UINTPTR KrGetPhysicalPageAddress(PAGEID idPage)
{
    if (idPage == KR_INVALID_PAGEID || idPage >= g_NumPages)
    {
        return (UINTPTR) NULLPTR;
    }
    return idPage * KR_PAGE_SIZE;
}

PAGEID KrGetPhysicalPageID(UINTPTR PhysAddr)
{
    if (PhysAddr > g_PhysAddrHighest)
    {
        return KR_INVALID_PAGEID;
    }

    return PhysAddr / KR_PAGE_SIZE;
}

BOOL KrReservePhysicalPage(PAGEID PageID)
{
    if (PageID == KR_INVALID_PAGEID || PageID >= g_NumPages)
    {
        return FALSE;
    }

    SIZE Index  = PageID / 8;
    SIZE Offset = PageID % 8;

    // First check if the page is acquired at all.
    if (!((g_pPrimaryBitmap[Index]) & (1 << Offset)))
    {
        return FALSE;
    }

    g_pAdvisoryBitmap[Index] |= (1 << Offset);
    g_StatePMM.UnusablePages++;

    return TRUE;
}

BOOL KrSetPhysicalPageAcquisitionHint(PAGEID PageID)
{
    if (PageID >= g_NumPages)
    {
        return FALSE;
    }
    g_StatePMM.AcquireHint = PageID;
    return TRUE;
}

BOOL KrIsPhysicalPageReserved(PAGEID idPage)
{
    if (idPage >= g_NumPages)
    {
        return FALSE;
    }
    return (g_pAdvisoryBitmap[idPage / 8]) & (1 << (idPage % 8));
}

CSTR  KrMemoryRegionTypeToString(DWORD dwType)
{
    switch (dwType)
    {
    case KR_MEMORY_TYPE_RESERVED             : return "KR_MEMORY_TYPE_RESERVED";
    case KR_MEMORY_TYPE_LOADER_CODE          : return "KR_MEMORY_TYPE_LOADER_CODE";
    case KR_MEMORY_TYPE_LOADER_DATA          : return "KR_MEMORY_TYPE_LOADER_DATA";
    case KR_MEMORY_TYPE_BOOT_SERVICES_CODE   : return "KR_MEMORY_TYPE_BOOT_SERVICES_CODE";
    case KR_MEMORY_TYPE_BOOT_SERVICES_DATA   : return "KR_MEMORY_TYPE_BOOT_SERVICES_DATA";
    case KR_MEMORY_TYPE_RUNTIME_SERVICES_CODE: return "KR_MEMORY_TYPE_RUNTIME_SERVICES_CODE";
    case KR_MEMORY_TYPE_RUNTIME_SERVICES_DATA: return "KR_MEMORY_TYPE_RUNTIME_SERVICES_DATA";
    case KR_MEMORY_TYPE_CONVENTIONAL_MEMORY  : return "KR_MEMORY_TYPE_CONVENTIONAL_MEMORY";
    case KR_MEMORY_TYPE_UNUSABLE_MEMORY      : return "KR_MEMORY_TYPE_UNUSABLE_MEMORY";
    case KR_MEMORY_TYPE_ACPI_RECLAIM_MEMORY  : return "KR_MEMORY_TYPE_ACPI_RECLAIM_MEMORY";
    case KR_MEMORY_TYPE_ACPI_MEMORY_NVS      : return "KR_MEMORY_TYPE_ACPI_MEMORY_NVS";
    case KR_MEMORY_TYPE_MMIO                 : return "KR_MEMORY_TYPE_MMIO";
    case KR_MEMORY_TYPE_MMIO_PORT_SPACE      : return "KR_MEMORY_TYPE_MMIO_PORT_SPACE";
    case KR_MEMORY_TYPE_PAL_CODE             : return "KR_MEMORY_TYPE_PAL_CODE";
    case KR_MEMORY_TYPE_PERSISTENT_MEMORY    : return "KR_MEMORY_TYPE_PERSISTENT_MEMORY";
    default: break;
    }
    return "(NULLPTR)";
}

BOOL KrIsUsableMemoryRegionType(DWORD dwType)
{
    switch (dwType)
    {
    case KR_MEMORY_TYPE_LOADER_CODE:
    case KR_MEMORY_TYPE_LOADER_DATA:
    case KR_MEMORY_TYPE_BOOT_SERVICES_CODE:
    case KR_MEMORY_TYPE_BOOT_SERVICES_DATA:
    case KR_MEMORY_TYPE_CONVENTIONAL_MEMORY:
    case KR_MEMORY_TYPE_ACPI_RECLAIM_MEMORY:
    {
        return TRUE;
    }
    default:
    {
        break;
    }
    }

    return FALSE;
}

const KrPhysmemmgmtState* KrGetPhysmemmgmtState(VOID)
{
    return &g_StatePMM;
}

BOOL KrIsPhysmemmgmtInitialized(VOID)
{
    return g_bInitPMM;
}
