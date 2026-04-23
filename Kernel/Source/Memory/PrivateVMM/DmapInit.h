// Private header for Virtmemmgmt.c

#ifndef YCH_KERNEL_MEMORY_PRIVATE_VMM_DMAP_INIT_H
#define YCH_KERNEL_MEMORY_PRIVATE_VMM_DMAP_INIT_H

#include "Earlyvideo/DisplaywideTextProtocol.h"

// For debugging
#define KR_VMM_DEBUG_FORCED_EVICTION FALSE
#define KR_VMM_DEBUG_NO_PDPE1GB      FALSE

static BYTE*   __KrAcquirePTE(PAGEID* pOutPageID, BYTE** WorkflowAreaHead, BYTE** WorkflowAreaCur, SIZE BootstrapAreaPageStructCount);
static UINTPTR __KrGetPhysicalOfLastPTE(PAGEID PageID, const VOID* pInitialBootstrapArena, SIZE BootstrapAreaPageStructCount);
static UINTPTR __KrGetVirtualOfPTE(UINTPTR AddrPhysical);

#define KrAcquirePTE()               __KrAcquirePTE(&PageID, &WorkflowAreaHead, &WorkflowAreaCur, BootstrapAreaPageStructCount)
#define KrGetPhysicalOfLastPTE(Addr) __KrGetPhysicalOfLastPTE(PageID, pBootstrapArenaInitial, BootstrapAreaPageStructCount)
#define KrGetVirtualOfPTE(Addr)      __KrGetVirtualOfPTE(Addr)

// I am genuinely going to kill myself,
// when you look at this code keep in mind it was written with 45 minutes of sleep and a dream.
// all the associated cursed helpers included.
static BOOL KrInitDirectMap(VOID)
{
    const UINT ONEGIB   = 1024 * 1024 * 1024;
    const UINT TWOMIB   =    2 * 1024 * 1024; // Tragic note: originally, I defined this as `1 * 1024 * 1024`.
    const UINT FOURKIB  =    4 * 1024;
    const UINT PageSize = g_KernelState.MemoryMapInfo.PageSize;

    // THIS MUST BE SET IN STONE EARLY!
    if (!KrBootstrapArenaAlign(KR_PAGE_STRUCTURE_SIZE)) // Align to 4KiB
    {
        return FALSE;
    }

    // Calculate how many paging structures can fit in the bootstrap arena.
    SIZE BootstrapAreaPageStructCount = KrBootstrapArenaGetSpaceLeft() / KR_PAGE_STRUCTURE_SIZE; // Floor divide
    // If no juice is left, system initialization cannot proceed.
    if (!BootstrapAreaPageStructCount)
    {
        return FALSE;
    }
    // Forced eviction is a debug mode to utilize bootstrap area bare minimum so page structures get evicted to normal memory...
    // This is done so we can be sure our main logic works with acquisiton and accessing of just mapped areas.
    if (KR_VMM_DEBUG_FORCED_EVICTION && BootstrapAreaPageStructCount > 16)
    {
        // `16` is enough to bootstrap us and very little to very quickly evict us out of the bootstrap arena.
        BootstrapAreaPageStructCount = 16;
    }
    // MUST STORE AFTER ALIGNMENT OPERATION!
    const VOID* pBootstrapArenaInitial = g_KernelState.LoadInfo.AddrPhysicalBase + (KrBootstrapArenaGetPtr() - g_KernelState.LoadInfo.AddrVirtualBase);
    BYTE* WorkflowAreaHead = KrBootstrapArenaAcquire(BootstrapAreaPageStructCount * KR_PAGE_STRUCTURE_SIZE); // * 4096
    KrtlContiguousZeroBuffer(WorkflowAreaHead, BootstrapAreaPageStructCount * KR_PAGE_STRUCTURE_SIZE);
    BYTE* WorkflowAreaCur  = WorkflowAreaHead;
    PAGEID PageID = KR_INVALID_PAGEID;

    /* Pointers to the start of PDPT, PD and PT structures. No PML4 one because you use the global g_PML4. */
    /* NOTE: These are virtual addresses so we work with these. */
    QWORD* pStructPDPT = NULLPTR;
    QWORD* pStructPD   = NULLPTR;
    QWORD* pStructPT   = NULLPTR;

    KrVirtualAddressMode ModeVA = KR_VAMODE_SMALL;
    KrPageTableEntryFlags TopLevelFlags = {0};
    TopLevelFlags.bPresent  = TRUE;
    TopLevelFlags.bWritable = TRUE;

    for (UINT i = 0; i < g_KernelState.NumCanonicalMapEntries; i++)
    {
        KrMemoryDescriptor* pRegion = g_KernelState.CanonicalMemoryMap + i;

        // Not needed since we use CanonicalMemoryMap now which already contains only super regions of CONVENTIONAL_MEMORY.
        // if (!KrIsUsableMemoryRegionType(pRegion->Type))
        // {
        //     continue;
        // }

        UINTPTR PhysAddrRegionBase = pRegion->PhysicalBase;
        ULONG   szRegionSize       = pRegion->PageCount * PageSize;

        KrVirtualAddress VirtIndices;
        while (szRegionSize) // No infinite loop because regions are guaranteed to be 4KiB aligned so we will exhaust the region eventually.
        {
            /* Decide *what* sort of table we have to use for this shit */
            // 1GiB aligned page & 1GiB span? HUGE.
            // Not so fast, processor must support huge pages (and our debug flag must be the correct value ofcourse)
            if ((!(KR_VMM_DEBUG_NO_PDPE1GB) && g_StateVMM.bHugePageSupport) && !(PhysAddrRegionBase & (ONEGIB - 1)) && szRegionSize >= ONEGIB)
            {
                ModeVA = KR_VAMODE_HUGE;
            }
            // 2MiB aligned page and 2MiB span? LARGE.
            else if (!(PhysAddrRegionBase & (TWOMIB - 1)) && szRegionSize >= TWOMIB)
            {
                ModeVA = KR_VAMODE_LARGE;
            }
            // The dreaded 4KiB pages.
            else
            {
                ModeVA = KR_VAMODE_SMALL;
            }
            VirtIndices = KrVirtBreakdown(g_StateVMM.AddrDirectMapBase + PhysAddrRegionBase, ModeVA);

            // Now comes to fun part... we have to see what structures we potentially need to map this.
            switch (ModeVA)
            {
            // We need the PML4 (always exists) and a PDPT entry.
            case KR_VAMODE_HUGE:
            {
                KrPageTableEntryFlags Flags = {0};
                Flags.bPresent  = TRUE;
                Flags.bWritable = TRUE;
                Flags.PS        = TRUE; // HUGE page, 1GiB

                // Acquire new PDPT if needed.
                if (!(g_PML4[VirtIndices.PML4]))
                {
                    pStructPDPT = (QWORD*) KrAcquirePTE();
                    g_PML4[VirtIndices.PML4] = KrEncodePageTableEntry(KrGetPhysicalOfLastPTE(), TopLevelFlags);
                    g_StateVMM.TotalPTEs++;
                }
                else
                {
                    UINTPTR VirtAddr = g_PML4[VirtIndices.PML4] & 0xFFFFFFFFFF000;
                    pStructPDPT = (QWORD*) KrGetVirtualOfPTE(VirtAddr);
                }
                *((KrPageTableEntry*) (((BYTE*) pStructPDPT) + VirtIndices.PDPT * KR_PAGE_STRUCTURE_ENTRY_SIZE)) = KrEncodeHugePageTableEntry(PhysAddrRegionBase, Flags);

                // Update tracking (note: these are not just stats. page struct space allocation logic uses them!!!)
                g_StateVMM.HugePages++;
                g_StateVMM.TotalPages++;

                PhysAddrRegionBase += ONEGIB;
                szRegionSize -= ONEGIB;

                break;
            }
            // We will need a PDPT and a PD. I'm contemplating ending it as of now.
            case KR_VAMODE_LARGE:
            {
                KrPageTableEntryFlags Flags = {0};
                Flags.bPresent  = TRUE;
                Flags.bWritable = TRUE;
                Flags.PS        = TRUE; // LARGE page, 2MiB

                // Acquire new PDPT if needed
                if (!(g_PML4[VirtIndices.PML4]))
                {
                    pStructPDPT = (QWORD*) KrAcquirePTE();
                    g_PML4[VirtIndices.PML4] = KrEncodePageTableEntry(KrGetPhysicalOfLastPTE(), TopLevelFlags);
                    g_StateVMM.TotalPTEs++;
                }
                else
                {
                    // g_PML4 contains physical. We need virtual
                    UINTPTR VirtAddr = g_PML4[VirtIndices.PML4] & 0xFFFFFFFFFF000;
                    pStructPDPT = (QWORD*) KrGetVirtualOfPTE(VirtAddr);
                }

                // Acquire new PD if needed
                if (!(pStructPDPT[VirtIndices.PDPT]))
                {
                    pStructPD = (QWORD*) KrAcquirePTE();
                    pStructPDPT[VirtIndices.PDPT] = KrEncodePageTableEntry(KrGetPhysicalOfLastPTE(), TopLevelFlags);
                    g_StateVMM.TotalPTEs++;
                }
                else
                {
                    UINTPTR VirtAddr = pStructPDPT[VirtIndices.PDPT] & 0xFFFFFFFFFF000;
                    pStructPD = (QWORD*) KrGetVirtualOfPTE(VirtAddr);
                }
                *((KrPageTableEntry*) (((BYTE*) pStructPD) + VirtIndices.PD * KR_PAGE_STRUCTURE_ENTRY_SIZE)) = KrEncodeLargePageEntry(PhysAddrRegionBase, Flags, 0);

                g_StateVMM.LargePages++;
                g_StateVMM.TotalPages++;

                PhysAddrRegionBase += TWOMIB;
                szRegionSize -= TWOMIB;

                break;
            }
            // We will need a PDPT, a PD and a PT. Why am I still alive?
            case KR_VAMODE_SMALL:
            {
                KrPageTableEntryFlags Flags = {0};
                Flags.bPresent  = TRUE;
                Flags.bWritable = TRUE;

                // Acquire new PDPT if needed
                if (!(g_PML4[VirtIndices.PML4]))
                {
                    pStructPDPT = (QWORD*) KrAcquirePTE();
                    g_PML4[VirtIndices.PML4] = KrEncodePageTableEntry(KrGetPhysicalOfLastPTE(), TopLevelFlags);
                    g_StateVMM.TotalPTEs++;
                }
                else
                {
                    // g_PML4 contains physical. We need virtual
                    UINTPTR VirtAddr = g_PML4[VirtIndices.PML4] & 0xFFFFFFFFFF000;
                    pStructPDPT = (QWORD*) KrGetVirtualOfPTE(VirtAddr);
                }

                // Acquire new PD if needed
                if (!(pStructPDPT[VirtIndices.PDPT]))
                {
                    pStructPD = (QWORD*) KrAcquirePTE();
                    pStructPDPT[VirtIndices.PDPT] = KrEncodePageTableEntry(KrGetPhysicalOfLastPTE(), TopLevelFlags);
                    g_StateVMM.TotalPTEs++;
                }
                else
                {
                    UINTPTR VirtAddr = pStructPDPT[VirtIndices.PDPT] & 0xFFFFFFFFFF000;
                    pStructPD = (QWORD*) KrGetVirtualOfPTE(VirtAddr);
                }

                // Acquire new PT if needed
                if (!(pStructPD[VirtIndices.PD]))
                {
                    pStructPT = (QWORD*) KrAcquirePTE();
                    pStructPD[VirtIndices.PD] = KrEncodePageTableEntry(KrGetPhysicalOfLastPTE(), TopLevelFlags);
                    g_StateVMM.TotalPTEs++;
                }
                else
                {
                    UINTPTR VirtAddr = pStructPD[VirtIndices.PD] & 0xFFFFFFFFFF000;
                    pStructPT = (QWORD*) KrGetVirtualOfPTE(VirtAddr);
                }
                *((KrPageTableEntry*) (((BYTE*) pStructPT) + VirtIndices.PT * KR_PAGE_STRUCTURE_ENTRY_SIZE)) = KrEncodePageTableEntry(PhysAddrRegionBase, Flags);

                g_StateVMM.SmallPages++;
                g_StateVMM.TotalPages++;

                PhysAddrRegionBase += FOURKIB;
                szRegionSize -= FOURKIB;

                break;
            }
            }
        }
    }

    return TRUE;
}

static BYTE* __KrAcquirePTE(PAGEID* pOutPageID, BYTE** WorkflowAreaHead, BYTE** WorkflowAreaCur, SIZE BootstrapAreaPageStructCount)
{
    if (g_StateVMM.TotalPTEs < BootstrapAreaPageStructCount)
    {
        BYTE* pResult = *WorkflowAreaCur;
        *WorkflowAreaCur += KR_PAGE_STRUCTURE_SIZE;
        return pResult;
    }
    else
    {
        *pOutPageID = KrAcquirePhysicalPage(KR_INVALID_PAGEID);
        *WorkflowAreaHead = (BYTE*)(g_StateVMM.AddrDirectMapBase + KrGetPhysicalPageAddress(*pOutPageID));
        *WorkflowAreaCur = *WorkflowAreaHead;
        KrtlContiguousZeroBuffer(*WorkflowAreaHead, GetPhysmemmgmtState()->PageSize);
        return *WorkflowAreaCur; 
    }
}

static UINTPTR __KrGetPhysicalOfLastPTE(PAGEID PageID, const VOID* pInitialBootstrapArena, SIZE BootstrapAreaPageStructCount)
{
    if (g_StateVMM.TotalPTEs < BootstrapAreaPageStructCount)
    {
        // Conversion ceremony...
        return (UINTPTR)(pInitialBootstrapArena + g_StateVMM.TotalPTEs * KR_PAGE_STRUCTURE_SIZE);
    }
    // If the condition above is FALSE this page structure lives on memory acquired from Physmemmgmt.
    return (UINTPTR) KrGetPhysicalPageAddress(PageID);
}

static UINTPTR __KrGetVirtualOfPTE(UINTPTR AddrPhysical)
{
    UINTPTR PhysAddrBootstrapArenaBase = KrReservedVirtToPhys(KrBootstrapArenaGetBase());
    UINTPTR PhysAddrBootstrapArenaEnd  = KrReservedVirtToPhys(KrBootstrapArenaGetEnd());

    if (AddrPhysical >= PhysAddrBootstrapArenaBase && AddrPhysical <= PhysAddrBootstrapArenaEnd)
    {
        return KrReservedPhysToVirt(AddrPhysical);
    }
    // If the condition above is FALSE this page structure lives on memory acquired from Physmemmgmt.
    return g_StateVMM.AddrDirectMapBase + AddrPhysical;
}

#endif // !YCH_KERNEL_MEMORY_PRIVATE_VMM_DMAP_INIT_H
