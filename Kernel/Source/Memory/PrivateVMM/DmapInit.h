// Private header for Virtmemmgmt.c

#ifndef YCH_KERNEL_MEMORY_PRIVATE_VMM_DMAP_INIT_H
#define YCH_KERNEL_MEMORY_PRIVATE_VMM_DMAP_INIT_H

#include "Core/Krnlmeltdown.h"
#include "Memory/PTEV2.h" // PTEv2 API, officially succeeds the PTE API

// For debugging
#define KR_VMM_DEBUG_FORCED_EVICTION       FALSE
#define KR_VMM_DEBUG_NO_PDPE1GB            FALSE
// 3 (one of each PDPT, PD and PT) is the lowest you can go. I don't recommend it though. 8 is a very very very safe point! Most configurations use 8 to 9 anyway.
// This value is irrelevant when KR_VMM_DEBUG_FORCED_EVICTION is FALSE. Bootstrap arena WILL be exhausted.
#define KR_VMM_DMAP_MINIMUM_VIABLE_PTCOUNT 8

// MappingContext has two modes: BarenaMode and EvictedMode
// Operates in BarenaMode until the bootstrap arena is exhausted.
// After that, operates in EvictedMode, where memory that was justly-mapped is used to store paging structures.
typedef struct
{
    // Physical address of the Bootstrap Arena pointer at the start of construction.
    UINTPTR PhysAddrBarenaBase;

    // Maximum number of paging structures that can be stored within the Bootstrap Arena.
    UINT    NumBarenaMaxStructs;

    // This is for tracking and used with KrdmiGetPhysicalOfLastPageStruct. Keeps track of how many paging structures have been acquired.
    UINT    NumTotalAcqPageStructs;

    // WorkingBase points to the start of the current working area (PhysAddrBarenaBase for BarenaMode, d-map virtual of the start of the physical page's base address for EvictedMode)
    BYTE*   WorkingBase;

    // WorkingCur acts as a stepping for BarenaMode (incremented by 4096 on each acquire request). For EvictedMode, it's always equal to WorkingBase.
    BYTE*   WorkingCur;

    // Page ID of the lastly acquired physical page. Before EvictedMode is entered, always irrelevant.
    PAGEID  LastAcqPageID;
} KrDirectMappingContext;

static PTE* KrdmiAcquirePageStruct(KrDirectMappingContext* pDmapContext);
static UINTPTR KrdmiGetPhysicalOfLastPageStruct(KrDirectMappingContext* pDmapContext);
static UINTPTR KrdmiGetVirtualOfPTE(UINTPTR AddrPhysical);
static PTE* KrdmiGetOrAcquirePageStruct(KrDirectMappingContext* pDmapContext, KrTypePTE eAcquireTypePTE, PTE* pMaster, UINT Index);

static BOOL KrInitDirectMap(VOID)
{
    const UINT ONEGIB  = 0x40000000;
    const UINT TWOMIB  = 0x200000; // Tragic note: originally, I defined this as `1 * 1024 * 1024`.
    const UINT FOURKIB = 0x1000;

    // THIS MUST BE SET IN STONE EARLY!
    if (!KrBootstrapArenaAlign(KR_PAGE_STRUCTURE_SIZE)) // Align to 4KiB
    {
        return FALSE;
    }

    KrDirectMappingContext DmapContext = {0};
    DmapContext.LastAcqPageID = KR_INVALID_PAGEID;

    // Calculate how many paging structures can fit in the bootstrap arena.
    DmapContext.NumBarenaMaxStructs = KrBootstrapArenaGetSpaceLeft() / KR_PAGE_STRUCTURE_SIZE; // Floor divide

    // If no juice is left, system initialization cannot proceed.
    if (!DmapContext.NumBarenaMaxStructs)
    {
        return FALSE;
    }

    // Forced eviction is a debug mode to utilize bootstrap area bare minimum so page structures get evicted to normal memory...
    // This is done so we can be sure our main logic works with acquisiton and accessing of justly mapped areas.
    if (KR_VMM_DEBUG_FORCED_EVICTION && DmapContext.NumBarenaMaxStructs > KR_VMM_DMAP_MINIMUM_VIABLE_PTCOUNT)
    {
        // This is enough to bootstrap us and very little to very quickly evict us out of the bootstrap arena.
        DmapContext.NumBarenaMaxStructs = KR_VMM_DMAP_MINIMUM_VIABLE_PTCOUNT;
    }

    // MUST STORE AFTER ALIGNMENT OPERATION!
    DmapContext.PhysAddrBarenaBase = g_KernelState.LoadInfo.AddrPhysicalBase + (((UINTPTR) KrBootstrapArenaGetPtr()) - g_KernelState.LoadInfo.AddrVirtualBase);

    DmapContext.WorkingBase = KrBootstrapArenaAcquire(DmapContext.NumBarenaMaxStructs * KR_PAGE_STRUCTURE_SIZE); // * 4096
    // Important: Zero our shit otherwise we'll get garbage and Present bits will be whatever RAM felt like that day.
    KrtlContiguousZeroBuffer(DmapContext.WorkingBase, DmapContext.NumBarenaMaxStructs * KR_PAGE_STRUCTURE_SIZE);
    DmapContext.WorkingCur = DmapContext.WorkingBase;

    /* Pointers to the start of PDPT, PD and PT structures. No PML4 one because you use the global `g_PML4` like a sane person. */
    PTE* PDPT = NULLPTR;
    PTE* PD   = NULLPTR;
    PTE* PT   = NULLPTR;

    KrVirtualAddressMode ModeVA = KR_VAMODE_SMALL; // default value to shush the compiler... actual mode chosen in loop.
    KrVirtualAddress Vidx;
    const QWORD qwLeafFlags = KR_PTE_PRESENT | KR_PTE_WRITABLE | KR_PTE_NX;
    const KrPatSelect pslDefault = KrSelectPat(KR_PAT_WRITE_BACK);
    
    for (UINT i = 0; i < g_KernelState.NumCanonicalMapEntries; i++)
    {
        KrMemoryDescriptor* pRegion = g_KernelState.CanonicalMemoryMap + i;

        UINTPTR PhysAddrRegion = pRegion->PhysicalBase;
        ULONG   szRegionSize   = pRegion->PageCount * KR_PAGE_SIZE;

        while (szRegionSize) // No infinite loop because regions are guaranteed to be 4KiB aligned so we will exhaust the region eventually.
        {
            /* Decide *what* sort of table we have to use for this shit */
            // 1GiB aligned page & 1GiB span? HUGE.
            // Not so fast, processor must support huge pages (and our debug flag must be the correct value of course)
            if ((!(KR_VMM_DEBUG_NO_PDPE1GB) && g_StateVMM.bHugePageSupport) && !(PhysAddrRegion & (ONEGIB - 1)) && szRegionSize >= ONEGIB)
            {
                ModeVA = KR_VAMODE_HUGE;
            }
            // 2MiB aligned page and 2MiB span? LARGE.
            else if (!(PhysAddrRegion & (TWOMIB - 1)) && szRegionSize >= TWOMIB)
            {
                ModeVA = KR_VAMODE_LARGE;
            }
            // The dreaded 4KiB pages.
            else
            {
                ModeVA = KR_VAMODE_SMALL;
            }
            Vidx = KrVirtBreakdown(g_StateVMM.DmapInfo.VirtAddrBase + PhysAddrRegion, ModeVA);

            switch (ModeVA)
            {
            case KR_VAMODE_HUGE: // We need : PDPT(1GiB)
            {
                PDPT = KrdmiGetOrAcquirePageStruct(&DmapContext, PDPT1GB_ENTRY, g_PML4, Vidx.PML4);
                PDPT[Vidx.PDPT] = KrpteEncodeEntry(PDPT1GB_ENTRY, PhysAddrRegion, qwLeafFlags, pslDefault);

                g_StateVMM.DmapInfo.HugePages++;
                g_StateVMM.DmapInfo.TotalPages++;

                PhysAddrRegion += ONEGIB;
                szRegionSize   -= ONEGIB;

                break;
            }
            case KR_VAMODE_LARGE: // We need : PDPT, PD(2MiB)
            {
                PDPT = KrdmiGetOrAcquirePageStruct(&DmapContext, PDPT_ENTRY ,  g_PML4, Vidx.PML4);
                PD   = KrdmiGetOrAcquirePageStruct(&DmapContext, PD2MB_ENTRY,  PDPT  , Vidx.PDPT);
                PD[Vidx.PD] = KrpteEncodeEntry(PD2MB_ENTRY, PhysAddrRegion, qwLeafFlags, pslDefault);

                g_StateVMM.DmapInfo.LargePages++;
                g_StateVMM.DmapInfo.TotalPages++;

                PhysAddrRegion += TWOMIB;
                szRegionSize   -= TWOMIB;

                break;
            }
            case KR_VAMODE_SMALL: // We need : PDPT, PD, PT
            {
                PDPT = KrdmiGetOrAcquirePageStruct(&DmapContext, PDPT_ENTRY, g_PML4, Vidx.PML4);
                PD   = KrdmiGetOrAcquirePageStruct(&DmapContext, PD_ENTRY  , PDPT  , Vidx.PDPT);
                PT   = KrdmiGetOrAcquirePageStruct(&DmapContext, PT_ENTRY  , PD    , Vidx.PD);
                PT[Vidx.PT] = KrpteEncodeEntry(PT_ENTRY, PhysAddrRegion, qwLeafFlags, pslDefault);

                g_StateVMM.DmapInfo.SmallPages++;
                g_StateVMM.DmapInfo.TotalPages++;

                PhysAddrRegion += FOURKIB;
                szRegionSize   -= FOURKIB;

                break;
            }
            }
        }
    }

    g_StateVMM.DmapInfo.TotalPageStructs = DmapContext.NumTotalAcqPageStructs; // set statistic of VMM for potential future inspection when debugging.
    return TRUE;
}

static PTE* KrdmiGetOrAcquirePageStruct(KrDirectMappingContext* pDmapContext, KrTypePTE eAcquireTypePTE, PTE* pMaster, UINT Index)
{
    // These are developer-level code mistakes.
    if (Index > 511 || eAcquireTypePTE == INVALID_PTE_TYPE || eAcquireTypePTE == PML4_ENTRY) // Why the hell are you trying to acquire a PML4, always use g_PML4.
    {
        MDCODE  mdCode = KR_MDCODE_DMAP_SETUP_DEVCHECK;
        CSTR   pMdDesc = "DevCheck: KrdmiGetOrAcquirePageStruct function supplied with nonsensical parameters. Investigation is highly recommended if you care even a little bit.";
        Krnlmeltdownimm(mdCode, pMdDesc);
    }

    // If present, return it! This function is called GetOrAcquire for a reason.
    if (pMaster[Index] & KR_PTE_PRESENT)
    {
        return (PTE*) KrdmiGetVirtualOfPTE(pMaster[Index] & KR_PTE_PHYSADDR_MASK);
    }

    PTE* pPTE = KrdmiAcquirePageStruct(pDmapContext);    
    if (!pPTE)
    {
        MDCODE  mdCode = KR_MDCODE_VIRTMEMMGMT_INIT_FAILURE;
        CSTR   pMdDesc = "Out-of-Memory during Direct-Map construction, cannot keep on acquiring physical pages for paging structures.";
        Krnlmeltdownimm(mdCode, pMdDesc);
    }
    
    KrTypePTE eTypeOfMaster = KrpteGetUpperType(eAcquireTypePTE);
    pMaster[Index] = KrpteEncodeEntry(eTypeOfMaster, KrdmiGetPhysicalOfLastPageStruct(pDmapContext), KR_PTE_PRESENT | KR_PTE_WRITABLE | KR_PTE_NX, KrSelectPat(KR_PAT_WRITE_BACK));
    
    return pPTE;
}

static PTE* KrdmiAcquirePageStruct(KrDirectMappingContext* pDmapContext)
{
    PTE* pResult = NULLPTR;
    if (pDmapContext->NumTotalAcqPageStructs < pDmapContext->NumBarenaMaxStructs)
    {
        pResult = (PTE*) pDmapContext->WorkingCur;
        pDmapContext->WorkingCur += KR_PAGE_STRUCTURE_SIZE;
    }
    // There is no Base-Cur separation with pages because a physical page is 4KiB, the same size as a paging structure.
    // So for each new paging structure to acquire... we just return a new page.
    else
    {
        pDmapContext->LastAcqPageID = KrAcquirePhysicalPage(KR_INVALID_PAGEID);
        if (pDmapContext->LastAcqPageID == KR_INVALID_PAGEID)
        {
            return NULLPTR;
        }

        // We need to ensure people cannot just go around and relinquish d-map PTEs. These are static.
        KrReservePhysicalPage(pDmapContext->LastAcqPageID);

        pDmapContext->WorkingBase = (BYTE*)(g_StateVMM.DmapInfo.VirtAddrBase + KrGetPhysicalPageAddress(pDmapContext->LastAcqPageID));
        pDmapContext->WorkingCur = pDmapContext->WorkingBase;

        // Important! Zero the page otherwise entires are sure to contain garbage values and the entirety of our logic breaks.
        KrtlContiguousZeroBuffer(pDmapContext->WorkingCur, KR_PAGE_SIZE);
        
        pResult = (PTE*) pDmapContext->WorkingCur; 
    }

    pDmapContext->NumTotalAcqPageStructs++;
    return pResult;
}

static UINTPTR KrdmiGetPhysicalOfLastPageStruct(KrDirectMappingContext* pDmapContext)
{
    // If we didn't exceed Barena limit we are still doing acquisitions in Barena space.
    // So we need to calculate it based on that...
    if (pDmapContext->NumTotalAcqPageStructs <= pDmapContext->NumBarenaMaxStructs)
    {
        return pDmapContext->PhysAddrBarenaBase + (pDmapContext->NumTotalAcqPageStructs - 1) * KR_PAGE_STRUCTURE_SIZE;
    }

    // If the condition above is FALSE this page structure lives on memory acquired from Physmemmgmt.
    // So we can just asks the PMM to give us the address of the page.
    return KrGetPhysicalPageAddress(pDmapContext->LastAcqPageID);
}

static UINTPTR KrdmiGetVirtualOfPTE(UINTPTR AddrPhysical)
{
    /* Physical Boundaries of the Bootstrap Arena */
    UINTPTR PhysAddrBarenaBase = KrReservedVirtToPhys(KrBootstrapArenaGetBase());
    UINTPTR PhysAddrBarenaEnd  = KrReservedVirtToPhys(KrBootstrapArenaGetEnd());

    if (AddrPhysical >= PhysAddrBarenaBase && AddrPhysical < PhysAddrBarenaEnd)
    {
        // Barena resides within the Kernel Reserved Area space, so we can do this.
        return KrReservedPhysToVirt(AddrPhysical);
    }

    // If the condition above is FALSE this page structure lives on memory acquired from Physmemmgmt.
    // We can use the very direct-map we are constructing to refer to it.
    return g_StateVMM.DmapInfo.VirtAddrBase + AddrPhysical;
}

#endif // !YCH_KERNEL_MEMORY_PRIVATE_VMM_DMAP_INIT_H
