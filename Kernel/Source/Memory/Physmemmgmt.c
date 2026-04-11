// TODO: Implement 2nd bitmap that stays constant and advisory, so checks about releasing reserved pages can be done.
#include "Memory/Physmemmgmt.h"

#include "Core/KernelState.h"

#include "Memory/BootstrapArena.h"
#include "KRTL/Krnlmem.h"

BYTE*   g_pBitmap               = NULLPTR;
USIZE   g_szBitmap              = 0;
USIZE   g_idPageAcqHint         = KR_INVALID_PAGEID;
UINTPTR g_AddrHighestDiscovered = 0;

// Starting from page idStart, it sets N pages to Status.
BOOL KrSetPhysicalPageStatus(PAGEID idStart, USIZE N, BYTE Status);

BOOL KrInitPhysmemmgmt(void)
{
    if (g_pBitmap)
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
        }
        
        UINTPTR addrRegionEnd = pDesc->PhysicalBase + pDesc->PageCount * g_KernelState.StatePMM.PageSize;
        if (addrRegionEnd > g_AddrHighestDiscovered)
        {
            g_AddrHighestDiscovered = addrRegionEnd;
        }
    }

    g_szBitmap = (g_KernelState.StatePMM.TotalPages + 7) / 8;
    g_pBitmap  = KrBootstrapArenaAcquire(g_szBitmap);

    if (!g_pBitmap)
    {
        return FALSE;
    }

    // Initialize all pages as unavailable first, this way we are less likely to mess up.
    // 0xFF = All bits =1 which is what UNAVAILABLE is set to, =1.
    KrtlContiguousSetBuffer(g_pBitmap, 0xFF, g_szBitmap);

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
            if (g_idPageAcqHint == KR_INVALID_PAGEID)
            {
                // If no acquisition hint yet set, use this one.
                g_idPageAcqHint = idPage;
            }
            // Set entire range as available
            KrSetPhysicalPageStatus(idPage, pDesc->PageCount, KR_PHYSICAL_PAGE_STATUS_AVAILABLE);
            break;
        }
        }
    }

    // Mark kernel-reserved area as unavailable (must be done since kernel lives in conventional memory and code above marks all that as available)
    KrSetPhysicalPageStatus
    (
        g_KernelState.LoadInfo.AddrPhysicalBase / g_KernelState.StatePMM.PageSize,
        g_KernelState.LoadInfo.ReserveSize      / g_KernelState.StatePMM.PageSize,
        KR_PHYSICAL_PAGE_STATUS_UNAVAILABLE
    );

    // Mark all memory under 1MiB as unavailable.
    // There are two reasons for this:
    //   - We should always reserve the first page, so we can have sane null pointer semantics.
    //   - Under 1MiB is IBM PC legacy mess. Better to not wake up the 1980s ghosts.
    KrSetPhysicalPageStatus(0, (1024 * 1024) / g_KernelState.StatePMM.PageSize, KR_PHYSICAL_PAGE_STATUS_UNAVAILABLE);

    return TRUE;
}

PAGEID KrAcquirePhysicalPage(PAGEID idHint)
{
    PAGEID PageID = idHint == KR_INVALID_PAGEID ? (g_idPageAcqHint == KR_INVALID_PAGEID ? 0 : g_idPageAcqHint) : idHint;
    PAGEID InitialSearchID = PageID;
    BOOL   bIsReroll = FALSE;

    if (PageID / 8 >= g_szBitmap)
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
                if (KrSetPhysicalPageStatus(PageID, 1, KR_PHYSICAL_PAGE_STATUS_UNAVAILABLE))
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

BOOL KrReleasePhysicalPage(PAGEID idPage)
{
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

BOOL KrSetPhysicalPageStatus(PAGEID idStart, USIZE N, BYTE Status)
{
    if (idStart + N > g_KernelState.StatePMM.TotalPages)
    {
        return FALSE;
    }

    USIZE iCurrent = idStart;
    BYTE* pRegion = g_pBitmap + (iCurrent / 8);

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
    if (idPage == KR_INVALID_PAGEID || idPage >= g_KernelState.StatePMM.TotalPages)
    {
        return NULLPTR;
    }
    return (void*)(idPage * g_KernelState.StatePMM.PageSize);
}
