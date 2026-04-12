#include "Memory/Physmemmgmt.h"

#include "Core/KernelState.h"

#include "Memory/BootstrapArena.h"
#include "KRTL/Krnlmem.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"
#include "CPU/Halt.h"

// The advisory bitmap, once initialized, is mostly static. Its primary job is to prevent reserved pages from being relinquished.
BYTE*   g_pAdvisoryBitmap;
// The dynamic bitmap that changes with acquisitions and relinquishments.
BYTE*   g_pBitmap;
// Size of the bitmap in bytes.
USIZE   g_szBitmap;
// Number of page entries in the bitmap itself, not ones described by the map.
USIZE   g_noPages;
// Highest address ever discovered, calculated via `Base + PageCount * PageSize` on a map entry.
UINTPTR g_AddrHighestDiscovered;
// Page acquisition hint for performance.
PAGEID  g_idPageAcqHint;

// Starting from page idStart, it sets N pages to Status.
BOOL KrSetPhysicalPageStatus(BYTE* pBitmap, PAGEID idStart, USIZE N, BYTE Status);

BOOL KrInitPhysmemmgmt(void)
{
    if (g_pBitmap || g_pAdvisoryBitmap)
    {
        return FALSE;
    }
    g_szBitmap = 0;

    g_KernelState.StatePMM.PageSize      = g_KernelState.MemoryMapInfo.PageSize;
    g_KernelState.StatePMM.TotalPages    =    0;
    g_KernelState.StatePMM.UnusablePages =    0;
    g_KernelState.StatePMM.AcquiredPages =    0;

    // 1st pass ; discovery
    for (QWORD i = 0; i < g_KernelState.MemoryMapInfo.EntryCount; i++)
    {
        KrMemoryDescriptor* pDesc = g_KernelState.MemoryMap + i;
        
        // This is the logical physical pages.
        g_KernelState.StatePMM.TotalPages += pDesc->PageCount;

        switch (pDesc->Type)
        {
        case KR_MEMORY_TYPE_RESERVED:
        case KR_MEMORY_TYPE_RUNTIME_SERVICES_CODE:
        case KR_MEMORY_TYPE_RUNTIME_SERVICES_DATA:
        case KR_MEMORY_TYPE_UNUSABLE_MEMORY:
        case KR_MEMORY_TYPE_ACPI_MEMORY_NVS:
        case KR_MEMORY_TYPE_MMIO:
        case KR_MEMORY_TYPE_MMIO_PORT_SPACE:
        case KR_MEMORY_TYPE_PAL_CODE:
        case KR_MEMORY_TYPE_PERSISTENT_MEMORY:
        {
            g_KernelState.StatePMM.UnusablePages += pDesc->PageCount;
            break;
        }
        // These are usable.
        default:
        {
            UINTPTR AddrRegionEnd = pDesc->PhysicalBase + pDesc->PageCount * g_KernelState.StatePMM.PageSize;
            if (AddrRegionEnd > g_AddrHighestDiscovered)
            {
                g_AddrHighestDiscovered = AddrRegionEnd;
            }
            break;
        }
        }
    }

    // noPages is based on highest addressable point.
    g_noPages         = (g_AddrHighestDiscovered + g_KernelState.StatePMM.PageSize - 1) / g_KernelState.StatePMM.PageSize;
    g_szBitmap        = (g_noPages + 7) / 8;
    g_pAdvisoryBitmap = KrBootstrapArenaAcquire(g_szBitmap);

    if (!g_pAdvisoryBitmap)
    {
        return FALSE;
    }

    // Initialize all pages as unavailable first, this way we are less likely to mess up.
    // 0xFF = All bits =1 which is what UNAVAILABLE is set to, =1.
    KrtlContiguousSetBuffer(g_pAdvisoryBitmap, 0xFF, g_szBitmap);

    // 2nd pass ; mark conventional memory and likewise areas as available.
    for (QWORD i = 0; i < g_KernelState.MemoryMapInfo.EntryCount; i++)
    {
        KrMemoryDescriptor* pDesc = g_KernelState.MemoryMap + i;

        switch (pDesc->Type)
        {
        case KR_MEMORY_TYPE_LOADER_CODE:
        case KR_MEMORY_TYPE_LOADER_DATA:
        case KR_MEMORY_TYPE_BOOT_SERVICES_CODE:
        case KR_MEMORY_TYPE_BOOT_SERVICES_DATA:
        case KR_MEMORY_TYPE_CONVENTIONAL_MEMORY:
        case KR_MEMORY_TYPE_ACPI_RECLAIM_MEMORY:
        {
            USIZE idPage = pDesc->PhysicalBase / g_KernelState.StatePMM.PageSize;
            if (!g_idPageAcqHint || g_idPageAcqHint == KR_INVALID_PAGEID)
            {
                // If no acquisition hint yet set, use this one.
                g_idPageAcqHint = idPage;
            }
            // Set entire range as available
            KrSetPhysicalPageStatus(g_pAdvisoryBitmap, idPage, pDesc->PageCount, KR_PHYSICAL_PAGE_STATUS_AVAILABLE);
            break;
        }
        }
    }

    // Mark kernel-reserved area as unavailable (must be done since kernel lives in conventional memory and code above marks all that as available)
    KrSetPhysicalPageStatus
    (
        g_pAdvisoryBitmap,
        g_KernelState.LoadInfo.AddrPhysicalBase / g_KernelState.StatePMM.PageSize,
        g_KernelState.LoadInfo.ReserveSize      / g_KernelState.StatePMM.PageSize,
        KR_PHYSICAL_PAGE_STATUS_UNAVAILABLE
    );

    // Mark all memory under 1MiB as unavailable.
    // There are two reasons for this:
    //   - We should always reserve the first page, so we can have sane null pointer semantics.
    //   - Under 1MiB is IBM PC legacy mess. Better to not wake up the 1980s ghosts.
    KrSetPhysicalPageStatus(g_pAdvisoryBitmap, 0, (1024 * 1024) / g_KernelState.StatePMM.PageSize, KR_PHYSICAL_PAGE_STATUS_UNAVAILABLE);

    // Now we'll create the dynamic bitmap and copy the advisory one as its initial state.
    g_pBitmap = KrBootstrapArenaAcquire(g_szBitmap);
    if (!g_pBitmap)
    {
        return FALSE;
    }
    KrtlContiguousCopyBuffer(g_pBitmap, g_pAdvisoryBitmap, g_szBitmap);

    return TRUE;
}

PAGEID KrAcquirePhysicalPage(PAGEID idHint)
{
    PAGEID PageID = idHint == KR_INVALID_PAGEID ? (g_idPageAcqHint == KR_INVALID_PAGEID ? 0 : g_idPageAcqHint) : idHint;
    PAGEID InitialSearchID = PageID;
    BOOL   bIsReroll = FALSE;

    if (PageID >= g_noPages)
    {
        return KR_INVALID_PAGEID;
    }
    
Hunt:
    for (USIZE i = PageID / 8; i < g_szBitmap && (bIsReroll ? PageID < InitialSearchID : TRUE); i++)
    {
        BYTE* pRegion = g_pBitmap + i;
        for (BYTE BitOffset = PageID % 8; BitOffset < 8; BitOffset++, PageID++)
        {
            BYTE RegionData = *pRegion;
            if (!(RegionData & (1 << BitOffset)))
            {
                if (KrSetPhysicalPageStatus(g_pBitmap, PageID, 1, KR_PHYSICAL_PAGE_STATUS_UNAVAILABLE))
                {
                    g_idPageAcqHint = PageID + 1;
                    return PageID;
                }
                // continue seeking if this one couldnt be acquired
            }
        }
    }

    if (!bIsReroll)
    {
        PageID = 0;
        bIsReroll = TRUE;
        goto Hunt;
    }

    return KR_INVALID_PAGEID;
}

BOOL KrRelinquishPhysicalPage(PAGEID idPage)
{
    // Cannot relinquish pages reserved during Physmemmgmt initialization.
    if (KrIsPhysicalPageReserved(idPage))
    {
        return FALSE;
    }

    USIZE Index = idPage / 8;
    if (Index >= g_szBitmap)
    {
        return FALSE;
    }
    BYTE BitOffset = idPage % 8;

    BYTE RegionData = g_pBitmap[Index];
    if (RegionData & (1 << BitOffset))
    {
        g_pBitmap[Index] &= ~(1 << BitOffset);
        return TRUE;
    }

    return FALSE;
}

// internal function, doesn't care about permissions, sets directly.
BOOL KrSetPhysicalPageStatus(BYTE* pBitmap, PAGEID idStart, USIZE N, BYTE Status)
{
    if (idStart + N > g_noPages)
    {
        return FALSE;
    }

    USIZE iCurrent = idStart;
    BYTE* pRegion = pBitmap + (iCurrent / 8);

    // Unaligned start
    USIZE BitOffset = iCurrent % 8;
    if (BitOffset)
    {
        BYTE Value = *pRegion;
        for (BitOffset; BitOffset < 8 && N; BitOffset++, N--, iCurrent++)
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

    // Bulk write
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

void* KrGetPhysicalPageAddress(PAGEID idPage)
{
    if (idPage == KR_INVALID_PAGEID || idPage >= g_noPages)
    {
        return NULLPTR;
    }
    return (void*)(idPage * g_KernelState.StatePMM.PageSize);
}

// This function allows an already acquired page to be marked as reserved, making it immune to relinquishments.
// Reserved pages can never be unreserved, this is a one way function.
BOOL KrReservePhysicalPage(PAGEID idPage)
{
    if (idPage >= g_noPages)
    {
        return FALSE;
    }
    USIZE Index = idPage / 8;
    USIZE Offset = idPage % 8;
    // First check if the page is acquired at all.
    if (!((g_pBitmap[Index]) & (1 << Offset)))
    {
        return FALSE;
    }
    g_pAdvisoryBitmap[Index] |= (1 << Offset);
    return TRUE;
}

BOOL KrIsPhysicalPageReserved(PAGEID idPage)
{
    if (idPage >= g_noPages)
    {
        return FALSE;
    }
    return (g_pAdvisoryBitmap[idPage / 8]) & (1 << (idPage % 8));
}
