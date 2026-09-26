#include "Memory/Physmemmgmt.h"

#include "Core/Krnlmeltdown.h"
#include "Core/KernelState.h"

#include "Memory/BootstrapArena.h"
#include "Memory/Virtmemmgmt.h"

#include "KRTL/Krnlmem.h"

#define PAGE_FREE      0
#define PAGE_ALLOCATED 1

// Current PMM state information.
static KrPhysmemmgmtState g_StatePMM = {0};

// Starting from page idStart, it sets N pages to Status. An internal function, doesn't care about permissions, sets directly.
static BOOL KrSetPhysicalPageStatus(BYTE* pBitmap, PAGEID idStart, SIZE N, BYTE Status);

static VOID KrBulkSetPagesMeta(PAGEID StartID, UINT N, const KrPhysicalPageMeta* pMeta);
static BOOL KrIsValidPageID(PAGEID PageID);

BOOL KrInitPhysmemmgmt(void)
{
    if (g_StatePMM.InitStage > PMM_INIT_STAGE_NONE)
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
            if (AddrRegionEnd > g_StatePMM.Private.PaddrHighest)
            {
                g_StatePMM.Private.PaddrHighest = AddrRegionEnd;
            }
        }
        else
        {
            g_StatePMM.UnusablePages += pDesc->PageCount;
        }
    }

    // noPages is based on highest addressable usable memory point.
    g_StatePMM.Private.NrPages    = (g_StatePMM.Private.PaddrHighest + KR_PAGE_SIZE - 1) / KR_PAGE_SIZE;
    g_StatePMM.Private.BitmapSize = (g_StatePMM.Private.NrPages + 7) / 8;
    g_StatePMM.Private.pAdvisoryBitmap = KrBootstrapArenaAcquire(g_StatePMM.Private.BitmapSize);

    if (!g_StatePMM.Private.pAdvisoryBitmap)
    {
        return FALSE;
    }

    // Initialize all pages as unavailable first, this way we are less likely to mess up.
    // 0xFF = All bits =1 which is what UNAVAILABLE is set to, =1.
    KrtlContiguousSetBuffer(g_StatePMM.Private.pAdvisoryBitmap, 0xFF, g_StatePMM.Private.BitmapSize);

    // 2nd pass ; mark conventional memory and likewise areas as available.
    for (QWORD i = 0; i < g_KernelState.NumCanonicalMapEntries; i++)
    {
        KrMemoryDescriptor* pDesc = g_KernelState.CanonicalMemoryMap + i;

        SIZE idPage = pDesc->PhysicalBase / KR_PAGE_SIZE;
        if (!g_StatePMM.AcquireHint || g_StatePMM.AcquireHint == IVLDPGID)
        {
            // If no acquisition hint yet set, use this one.
            g_StatePMM.AcquireHint = idPage;
        }
        // Set entire range as available
        KrSetPhysicalPageStatus(g_StatePMM.Private.pAdvisoryBitmap, idPage, pDesc->PageCount, PAGE_FREE);
    }

    // Mark kernel-reserved area as unavailable (must be done since kernel lives in conventional memory and code above marks all that as available)
    KrSetPhysicalPageStatus
    (
        g_StatePMM.Private.pAdvisoryBitmap,
        g_KernelState.LoadInfo.AddrPhysicalBase / KR_PAGE_SIZE,
        g_KernelState.LoadInfo.ReserveSize      / KR_PAGE_SIZE,
        PAGE_ALLOCATED
    );

    // Mark all memory under 1MiB as unavailable.
    // There are two reasons for this:
    //   - We should always reserve the first page, so we can have sane null pointer semantics.
    //   - Under 1MiB is IBM PC cluster fuck area. Better to not wake up the 1980s ghosts.
    KrSetPhysicalPageStatus(g_StatePMM.Private.pAdvisoryBitmap, 0, (1024 * 1024) / KR_PAGE_SIZE, PAGE_ALLOCATED);

    // Now we'll create the dynamic bitmap and copy the advisory one as its initial state.
    g_StatePMM.Private.pPrimaryBitmap = KrBootstrapArenaAcquire(g_StatePMM.Private.BitmapSize);
    if (!g_StatePMM.Private.pPrimaryBitmap)
    {
        return FALSE;
    }
    KrtlContiguousCopyBuffer(g_StatePMM.Private.pPrimaryBitmap, g_StatePMM.Private.pAdvisoryBitmap, g_StatePMM.Private.BitmapSize);

    g_StatePMM.InitStage = PMM_INIT_STAGE_BASIC;
    return TRUE;
}

BOOL KrInitPhysMetaArray(VOID)
{
    if (g_StatePMM.InitStage < PMM_INIT_STAGE_BASIC || g_StatePMM.InitStage > PMM_INIT_STAGE_BOOKKEEPING)
    {
        return FALSE;
    }

    const UINT MetasPerPhysicalPage    = KR_PAGE_SIZE / sizeof(KrPhysicalPageMeta);
    const UINT NeededPageCountForArray = KR_CEILDIV(g_StatePMM.TotalPages, MetasPerPhysicalPage);

    PAGEID BasePage;
    DWORD dwNumAcqPages = KrAcquirePhysicalPages(
        &BasePage,
        NeededPageCountForArray,
        PAGE_TYPE_BOOKKEEPING,
        PAGE_ACQ_DENSE | PAGE_ACQ_BASE_OUT_ONLY,
        IVLDPGID
    );

    if (dwNumAcqPages < NeededPageCountForArray)
    {
        for (PAGEID ID = BasePage; ID < BasePage + dwNumAcqPages; ID++)
        {
            KrRelinquishPhysicalPage(ID);
        }
        return FALSE;
    }

    g_StatePMM.Private.PaddrMetaArray = KrGetPhysicalPageAddress(BasePage);
    g_StatePMM.Private.VaddrMetaArray = KrPhysToVirt(g_StatePMM.Private.PaddrMetaArray);

    // Baseline zero state
    KrtlContiguousZeroBuffer((VOID*) g_StatePMM.Private.VaddrMetaArray, dwNumAcqPages * KR_PAGE_SIZE);

    // Every page that has been acquired up to this point will be marked as reserved
    for (SIZE i = 0; i < g_StatePMM.Private.BitmapSize; i++)
    {
        const BYTE PagesState = g_StatePMM.Private.pPrimaryBitmap[i];
        for (BYTE j = 0; j < 8; j++)
        {
            if (PagesState & (1 << j))
            {
                const PAGEID ID = i * 8 + j;
                KrPhysicalPageMeta* pMeta = KrGetPageMeta(ID);

                pMeta->RefCount = 1;
                pMeta->Type = PAGE_TYPE_RESERVED;
                pMeta->Flags = PAGE_FLAG_PINNED;
            }
        }
    }

    return TRUE;
}

DWORD KrAcquirePhysicalPages(PAGEID* pOutIDs, UINT uToAcquire, BYTE PageType, DWORD dwAcquisitionMethod, PAGEID HintID)
{
    if (!uToAcquire)
    {
        return 0;
    }
    if (((dwAcquisitionMethod & PAGE_ACQ_SPARSE) && (dwAcquisitionMethod & PAGE_ACQ_DENSE)) ||
        ((dwAcquisitionMethod & PAGE_ACQ_SPARSE) && (dwAcquisitionMethod & PAGE_ACQ_BASE_OUT_ONLY)))
    {
        return 0;
    }

    // This is a bit of a lie until near the end of the function where they are actually claimed.
    // Until then it acts more like a counter.
    UINT uNoAcquired = 0;

    PAGEID PageID = HintID == IVLDPGID ? (g_StatePMM.AcquireHint == IVLDPGID ? 0 : g_StatePMM.AcquireHint) : HintID;
    PAGEID InitialSearchID = PageID;
    BOOL   bIsReroll = FALSE;

    if (!KrIsValidPageID(PageID))
    {
        return 0;
    }
    
Hunt:
    // NOTE: Logic code block itself inside these two for loops do not check if uToAcquire was reached, because the outer loops handle it.
    for (SIZE i = PageID / 8; i < g_StatePMM.Private.BitmapSize && uNoAcquired < uToAcquire && (bIsReroll ? PageID < InitialSearchID : TRUE); i++)
    {
        BYTE* pRegion = g_StatePMM.Private.pPrimaryBitmap + i;
        for (BYTE BitOffset = PageID % 8; BitOffset < 8 && uNoAcquired < uToAcquire; BitOffset++, PageID++)
        {
            BYTE RegionData = *pRegion;

            // Page unavailable. This block contains the ruined logic for DENSE as well. SPARSE simply does not care.
            if (RegionData & (1 << BitOffset) || KrIsPhysicalPageReserved(PageID))
            {
                if (dwAcquisitionMethod & PAGE_ACQ_DENSE)
                {
                    uNoAcquired = 0;
                }
            }
            else // Page available for acquisition
            {
                if (dwAcquisitionMethod & PAGE_ACQ_BASE_OUT_ONLY)
                {
                    if (uNoAcquired++ == 0)
                    {
                        *pOutIDs = PageID;
                    }
                }
                else
                {
                    pOutIDs[uNoAcquired++] = PageID;
                }
            }
        }
    }

    // See if we started our first iteration from a nonzero page, if so, wraparound so we search the area we skipped.
    if ((!bIsReroll && InitialSearchID != 0) && uNoAcquired < uToAcquire)
    {
        // Wraparound ruins DENSE streak.
        if (dwAcquisitionMethod & PAGE_ACQ_DENSE)
        {
            uNoAcquired = 0;
        }
        PageID = 0;
        bIsReroll = TRUE;
        goto Hunt;
    }

    if (!uNoAcquired)
    {
        return 0; // Hunting flies!
    }

    // Our function spec says:
    /* State of `pOutIDs` post-return of this function is (UNLESS BASE_OUT_ONLY WAS SPECIFIED):
     * Index `0` to Index `(NumAcquiredPages i.e. Return Value - 1)` are valid Page IDs to newly-acquired pages.
     * Index `NumAcquiredPages i.e. Return Value` to `dwToAcquire` are set to IVLDPGID. */
    if (!(dwAcquisitionMethod & PAGE_ACQ_BASE_OUT_ONLY))
    {
        for (UINT i = uNoAcquired; i < uToAcquire; i++)
        {
            pOutIDs[i] = IVLDPGID;
        }
    }

    const KrPhysicalPageMeta pMetaForAcquiredPages = {
        .RefCount = 1,
        .Flags = (dwAcquisitionMethod & PAGE_ACQ_FOR_USER) ? PAGE_FLAG_USER : 0,
        .Auxiliary = 0,
        .Type = PageType
    };

    if (dwAcquisitionMethod & PAGE_ACQ_DENSE)
    {
        KrSetPhysicalPageStatus(g_StatePMM.Private.pPrimaryBitmap, *pOutIDs, uNoAcquired, PAGE_ALLOCATED);
        if (g_StatePMM.InitStage >= PMM_INIT_STAGE_BOOKKEEPING)
        {
            KrBulkSetPagesMeta(*pOutIDs, uNoAcquired, &pMetaForAcquiredPages);
        }
    }
    else
    {
        for (UINT i = 0; i < uNoAcquired;)
        {
            UINT j;
            for (j = i + 1; j < uNoAcquired && pOutIDs[i] + 1 == pOutIDs[j]; j++);

            KrSetPhysicalPageStatus(g_StatePMM.Private.pPrimaryBitmap, pOutIDs[i], j - i, PAGE_ALLOCATED);
            if (g_StatePMM.InitStage >= PMM_INIT_STAGE_BOOKKEEPING)
            {
                KrBulkSetPagesMeta(pOutIDs[i], j - i, &pMetaForAcquiredPages);
            }
            i = j;
        }
    }

    g_StatePMM.AcquiredPages += uNoAcquired;
    g_StatePMM.AcquireHint = (dwAcquisitionMethod & PAGE_ACQ_BASE_OUT_ONLY) ? *pOutIDs + uNoAcquired : pOutIDs[uNoAcquired - 1] + 1;

    return uNoAcquired;
}

PAGEID KrAcquirePhysicalPage(BYTE PageType, PAGEID HintID)
{
    PAGEID PageID = IVLDPGID;
    DWORD  dwAcquired = KrAcquirePhysicalPages(&PageID, 1, PageType, PAGE_ACQ_SPARSE, HintID);
    return dwAcquired ? PageID : IVLDPGID;
}

BOOL KrRelinquishPhysicalPage(PAGEID PageID)
{
    // Cannot relinquish pages reserved during Physmemmgmt initialization or explicitly marked as reserved afterward.
    if (KrIsPhysicalPageReserved(PageID))
    {
        return FALSE;
    }

    KrPhysicalPageMeta* pMeta = NULLPTR;
    if (g_StatePMM.InitStage >= PMM_INIT_STAGE_BOOKKEEPING)
    {
        pMeta = KrGetPageMeta(PageID);

        if (pMeta->Flags & PAGE_FLAG_PINNED)
        {
            MDCODE MdCode = KR_MDCODE_PINNED_PAGE_RELINQUISHED;
            CSTR MdDesc = "Tried to relinquish a pinned physical page.";
            Krnlmeltdownimm(MdCode, MdDesc);
        }

        if (!pMeta->RefCount)
        {
            return FALSE; // Page has no references, cannot relinquish
        }
        if (--pMeta->RefCount)
        {
            return TRUE; // This reference was relinquished successfully.
        }
        // falls through if RefCount - 1 resulted in 0
    }

    SIZE Index = PageID / 8;
    if (Index >= g_StatePMM.Private.BitmapSize)
    {
        return FALSE;
    }
    BYTE BitOffset = PageID % 8;

    BYTE RegionData = g_StatePMM.Private.pPrimaryBitmap[Index];
    if (RegionData & (1 << BitOffset))
    {
        g_StatePMM.Private.pPrimaryBitmap[Index] &= ~(1 << BitOffset);
        g_StatePMM.AcquiredPages--;
        return TRUE;
    }

    return FALSE;
}

static BOOL KrSetPhysicalPageStatus(BYTE* pBitmap, PAGEID idStart, SIZE N, BYTE Status)
{
    if (idStart + N > g_StatePMM.Private.NrPages)
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

UINTPTR KrGetPhysicalPageAddress(PAGEID PageID)
{
    if (!KrIsValidPageID(PageID))
    {
        return (UINTPTR) NULLPTR;
    }
    return PageID * KR_PAGE_SIZE;
}

PAGEID KrGetPhysicalPageID(UINTPTR PhysAddr)
{
    if (PhysAddr > g_StatePMM.Private.PaddrHighest)
    {
        return IVLDPGID;
    }

    return PhysAddr / KR_PAGE_SIZE;
}

KrPhysicalPageMeta* KrGetPageMeta(PAGEID PageID)
{
    return ((KrPhysicalPageMeta*) g_StatePMM.Private.VaddrMetaArray) + PageID;
}

BOOL KrReservePhysicalPage(PAGEID PageID)
{
    if (!KrIsValidPageID(PageID))
    {
        return FALSE;
    }

    SIZE Index  = PageID / 8;
    SIZE Offset = PageID % 8;

    // First check if the page is acquired at all.
    if (!((g_StatePMM.Private.pPrimaryBitmap[Index]) & (1 << Offset)))
    {
        return FALSE;
    }

    g_StatePMM.Private.pAdvisoryBitmap[Index] |= (1 << Offset);
    g_StatePMM.UnusablePages++;

    if (g_StatePMM.InitStage >= PMM_INIT_STAGE_BOOKKEEPING)
    {
        KrPhysicalPageMeta* pMeta = KrGetPageMeta(PageID);
        pMeta->Flags |= PAGE_FLAG_PINNED;
    }

    return TRUE;
}

BOOL KrSetPhysicalPageAcquisitionHint(PAGEID PageID)
{
    if (!KrIsValidPageID(PageID))
    {
        return FALSE;
    }
    g_StatePMM.AcquireHint = PageID;
    return TRUE;
}

BOOL KrIsPhysicalPageReserved(PAGEID PageID)
{
    if (!KrIsValidPageID(PageID))
    {
        return FALSE;
    }
    return (g_StatePMM.Private.pAdvisoryBitmap[PageID / 8]) & (1 << (PageID % 8));
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

static VOID KrBulkSetPagesMeta(PAGEID StartID, UINT N, const KrPhysicalPageMeta* pMeta)
{
    for (PAGEID ID = 0; ID < StartID + N; ID++)
    {
        *KrGetPageMeta(ID) = *pMeta;
    }
}

static BOOL KrIsValidPageID(PAGEID PageID)
{
    return PageID != IVLDPGID && PageID < g_StatePMM.Private.NrPages;
}
