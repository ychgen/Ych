#include "Memory/Virtmemmgmt.h"

#include "Core/Krnlmeltdown.h"
#include "Core/KernelState.h"

#include "CPU/Interrupt.h"
#include "CPU/CPUID.h"
#include "CPU/MSR.h"
#include "CPU/PAT.h"
#include "CPU/CR.h"

#include "Memory/BootstrapArena.h"
#include "Memory/PrimitiveHeap.h"
#include "Memory/Physmemmgmt.h"
#include "Memory/PageFault.h"

#include "KRTL/Krnlmem.h"

// TODO: Remove later, these are for debugging currently
#include "Earlyvideo/DisplaywideTextProtocol.h"
#include "CPU/Halt.h"

// Important stuff check (dead serious)
KR_STATIC_ASSERT(sizeof(KrVirtualMemoryRegion) <= KR_PRIMITIVE_HEAP_STEPPING, "struct `KrVirtualMemoryRegion` does not fit within a Primitive Heap 'Stepping'.");

KrVirtmemmgmtState     g_StateVMM = {0};
KrVirtualMemoryRegion  g_RootVMR  = {0};
KrVirtualMemoryRegion  g_KrnlVMR  = {0};
KrVirtualMemoryRegion  g_FrBufVMR = {0};
KrVirtualMemoryRegion* g_pTailVMR = NULLPTR;

// Include these here (they depend on stuff from above)
#include "PrivateVMM/Smapinit.h" // for KrInitStaticPages()
#include "PrivateVMM/DmapInit.h" // for KrInitDirectMap()

inline UINT KrGetRegionPageGranularity(WORD wRegionFlags)
{
    // 2MiB if LARGE_PAGE, 4KiB otherwise.
    return wRegionFlags & KR_PAGE_FLAG_LARGE ? 0x200000 : 0x1000;
}

BOOL KrInitVirtmemmgmt(VOID)
{
    // See if we are already initialized or sumn
    if (g_PML4[KR_KERNEL_RESERVED_PML4_INDEX])
    {
        return FALSE;
    }

    // Huge Page Support Check & NX Support Check + Activation via MSR.
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

    // Control Register Stuff
    {
        QWORD CR0; KrReadCR0(CR0);
        QWORD CR4; KrReadCR4(CR4);

        CR0 |=   KR_CR0_WP;
        CR4 |=   KR_CR4_PSE | KR_CR4_PAE | KR_CR4_PGE;
        
        KrWriteCR0(CR0);
        KrWriteCR4(CR4);
    }

    // PAT MSR setup
    {
        QWORD qwPatMsr =
        // HIDWORD
        (((QWORD) KR_PAT_UNCACHEABLE) << 56) | (((QWORD) KR_PAT_UNCACHED) << 48) | (((QWORD) KR_PAT_WRITE_PROTECTED) << 40) | (((QWORD) KR_PAT_WRITE_COMBINING) << 32)
        |
        // LODWORD (keep exactly as is as reset state for maximum compatibility, modify HIDWORD!)
        (((QWORD) KR_PAT_UNCACHEABLE) << 24) | (((QWORD) KR_PAT_UNCACHED) << 16) | (((QWORD) KR_PAT_WRITE_THROUGH) << 8) | (((QWORD) KR_PAT_WRITE_BACK) << 0);

        // Just look at the PA stuff above you'll see the pattern cousin...
        g_KernelState.PatMsrState.PA_UC  = 3; // Uncacheable
        g_KernelState.PatMsrState.PA_WC  = 4; // Write-Combining
        g_KernelState.PatMsrState.PA_WT  = 1; // Write-Through
        g_KernelState.PatMsrState.PA_WP  = 5; // Write-Protect
        g_KernelState.PatMsrState.PA_WB  = 0; // Write-Back
        g_KernelState.PatMsrState.PA_UCM = 2; // Uncached

        KrLoadPatMsr(qwPatMsr);
    }

    // This keeps our current mapping and unmapping identity-mapped lower 2MiB (L bozo).
    KrInitStaticPages();

    // Take control of paging.
    UINTPTR AddrPhysicalPML4 = KrReservedVirtToPhys(g_PML4);
    KrWriteCR3(AddrPhysicalPML4);

    // Our paging configuration must be active before calling this. It depends on using pages it maps like scaffolding once it falls out of the bootstrap arena.
    // It also directly works on g_PML4 and such.
    if (!KrInitDirectMap())
    {
        return FALSE;
    }

    // Initialize `Primitive Heap`, needed to allocate VirtualMemoryRegion nodes.
    if (!KrPrimitiveInit())
    {
        return FALSE;
    }

    // Create root node by hand (cus KrMapVirt needs at least a proper root node to exist to function.)
    // Root node will cover address 0 through 2MiB.
    // We create this reservation, so no fool can claim this space.
    // Reason is: No one should map NULLPTR otherwise I am coming after them.
    // Reason for anything other than the NULLPTR reason: my dick wanted it that way so it is the way it is.
    g_RootVMR.VirtAddrBase = 0; // Virtual address 0x0000000000000000, so like, NULLPTR and shit.
    g_RootVMR.szPageCount  = 1; // 1 page.
    g_RootVMR.wAcquisitionType = KR_ACQUIRE_STATIC; // Do not commit, duh.
    g_RootVMR.wFlags = KR_PAGE_FLAG_GUARD | KR_PAGE_FLAG_LARGE; // One `2MiB` page. Also hard reserve so page is inaccessible.
    g_RootVMR.uProcID = KR_VMM_PROCID_INVALID; // KrMapVirt() would reject this but we are not asking him right now.
    g_RootVMR.pPrev = NULLPTR; // We are the root node, duh.

    // Kernel and Frame Buffer were already physically mapped by KrInitStaticPages() so they have PTEs and stuff,
    // so we just need to create nodes for them. No need to use KrMapVirt.
    g_KrnlVMR.VirtAddrBase = g_KernelState.LoadInfo.AddrVirtualBase;
    g_KrnlVMR.szPageCount  = KR_CEILDIV(g_KernelState.LoadInfo.ReserveSize, 0x200000); // NOTE: Like this currently because kernel is mapped using all LARGE pages. Update for future where various ones are used for different sections.
    g_KrnlVMR.wAcquisitionType = KR_ACQUIRE_STATIC;
    g_KrnlVMR.wFlags = KR_PAGE_FLAG_READ | KR_PAGE_FLAG_WRITE | KR_PAGE_FLAG_EXECUTE | KR_PAGE_FLAG_LARGE;
    g_KrnlVMR.uProcID = KR_VMM_PROCID_KERNEL;

    g_FrBufVMR.VirtAddrBase = g_KernelState.FrameBufferInfo.VirtualAddress;
    g_FrBufVMR.szPageCount  = KR_CEILDIV(g_KernelState.FrameBufferInfo.Size, 0x200000); // NOTE: Because FRBUF is mapped using LARGE pages.
    g_FrBufVMR.wAcquisitionType = KR_ACQUIRE_STATIC;
    g_FrBufVMR.wFlags = KR_PAGE_FLAG_WRITE | KR_PAGE_FLAG_EXECUTE | KR_PAGE_FLAG_LARGE | KR_PAGE_FLAG_WRITE_COMBINE; // Mapped as WC by KrInitStaticPages already. NOTE: No READ flag! Please do not read from frbuf.
    g_FrBufVMR.uProcID = KR_VMM_PROCID_KERNEL;

    // Because the VMR linked list has to be sorted, see which one to link 1st and 2nd.
    KrVirtualMemoryRegion* pRegion1st, *pRegion2nd;
    if (g_KernelState.LoadInfo.AddrVirtualBase < g_KernelState.FrameBufferInfo.VirtualAddress)
    {
        pRegion1st = &g_KrnlVMR;
        pRegion2nd = &g_FrBufVMR;
    }
    else
    {
        pRegion1st = &g_FrBufVMR;
        pRegion2nd = &g_KrnlVMR;
    }
    g_RootVMR.pNext   =  pRegion1st;
    pRegion1st->pPrev = &g_RootVMR;
    pRegion1st->pNext =  pRegion2nd;
    pRegion2nd->pPrev =  pRegion1st;
    pRegion2nd->pNext =  NULLPTR;
    g_pTailVMR        =  pRegion2nd;

    // NOTE: Only and only after all initialization steps should we assign the Page Fault handler.
    // Parameter `bOverwrite`=TRUE for overwrite! You must provide it as TRUE to overwrite the basic KrCriticalProcessorInterrupt handler.
    if (!KrRegisterInterruptHandler(KR_INTERRUPTNO_PAGE_FAULT, KrGlobalPageFaultHandler, TRUE))
    {
        return FALSE;
    }

    return TRUE;
}

KrPageTableEntry* KrSmapGetOrAcquireStruct(BOOL* bWasAcquire, KrPageTableEntry* pMaster, UINT Idx)
{
    KR_STATIC_ASSERT(KR_PAGE_SIZE == 4096, "This function assumes a physical page is 4KiB, exactly the size of a page structure. Verily, the problem is on another scale, KR_PAGE_SIZE is not 4096. Congratulations, you just broke modern computing.");

    if (pMaster[Idx] & 1) // P bit
    {
        if (bWasAcquire)
        {
            *bWasAcquire = FALSE;
        }
        return (KrPageTableEntry*) KrPhysToVirt(pMaster[Idx] & 0xFFFFFFFFFF000);
    }
    
    PAGEID PageID = KrAcquirePhysicalPage(KR_INVALID_PAGEID);
    if (PageID == KR_INVALID_PAGEID)
    {
        return NULLPTR;
    }
    if (bWasAcquire)
    {
        *bWasAcquire = TRUE;
    }
    KrPageTableEntry* pResult = (KrPageTableEntry*) KrPhysToVirt(KrGetPhysicalPageAddress(PageID));
    KrtlContiguousZeroBuffer(pResult, KR_PAGE_SIZE);
    return pResult;
}

KrMapResult KrMapVirt(UINT uProcID, UINTPTR pAddrVirt, UINTPTR pAddrPhys, SIZE szRegionSize, WORD wAcquisitionType, WORD wFlags)
{
    if (szRegionSize == 0)
    {
        return KR_MAP_RESULT_UNPAGED;
    }
    if (uProcID == KR_VMM_PROCID_INVALID) // the fuck are you even tryna do bruh???
    {
        return KR_MAP_RESULT_UNIMPLEMENTED;
    }

    // Check contradictory flags
    if ((wFlags & (KR_PAGE_FLAG_WRITE_COMBINE | KR_PAGE_FLAG_UNCACHEABLE)) == (KR_PAGE_FLAG_WRITE_COMBINE | KR_PAGE_FLAG_UNCACHEABLE))
    {
        return KR_MAP_RESULT_CONTRADICTION;
    }
    // Acquisition cannot be COMMIT/STATIC while flags specificies GUARD (page with access = illegal)
    if (wAcquisitionType != KR_ACQUIRE_RESERVE && (wFlags & KR_PAGE_FLAG_GUARD))
    {
        return KR_MAP_RESULT_CONTRADICTION;
    }

    const UINT PageGranularity = KrGetRegionPageGranularity(wFlags);
    if ((pAddrVirt & (PageGranularity - 1)) || (pAddrPhys & (PageGranularity - 1))) // Must be aligned otherwise I'll be under your bed
    {
        return KR_MAP_RESULT_UNALIGNED;
    }

    const SIZE    PageCount    = KR_CEILDIV(szRegionSize, PageGranularity);
    const SIZE    ByteCount    = PageCount * PageGranularity; 
    const UINTPTR pAddrVirtEnd = pAddrVirt + ByteCount;

    KrVirtualMemoryRegion* pRegion      = &g_RootVMR; // Start from root
    KrVirtualMemoryRegion* pInsertAfter =  NULLPTR;

    // First check this to potentially avoid iteration.
    // If requested virtual address is ahead of the tail, we know we have to insert after the tail.
    // If this is the case the if block won't be entered and the NULLPTR check of pInsertAfter will set it to tail.
    if (pAddrVirt <= g_pTailVMR->VirtAddrBase + g_pTailVMR->szPageCount * KrGetRegionPageGranularity(g_pTailVMR->wFlags))
    {
        while (pRegion)
        {
            const UINT    RegionPageGranularity = KrGetRegionPageGranularity(pRegion->wFlags);
            const UINTPTR pRegionEnd = pRegion->VirtAddrBase + pRegion->szPageCount * RegionPageGranularity;

            if (pAddrVirt == pRegion->VirtAddrBase || (pAddrVirt > pRegion->VirtAddrBase && pAddrVirtEnd < pRegionEnd))
            {
                break;
            }

            KrVirtualMemoryRegion* const pNextRegion = pRegion->pNext;
            if (pNextRegion)
            {
                // Currently, we know this virtual address range is ahead of pRegion.
                // So we will check if this virtual address range is behind pNextRegion.
                // If so, we'll directly insert our new node after pRegion and update linked list accordingly.
                // This way, it is sorted by virtual address on insertion directly.
                // We want to keep our linked list sorted by virtual address, because I said so.

                if (pAddrVirtEnd < pNextRegion->VirtAddrBase)
                {
                    // Found our boy I'm so happy boy wherever you're taking me, I got 24 hours away from ma wife!
                    pInsertAfter = pRegion;
                    pRegion = NULLPTR;
                    break;
                }
            }
            pRegion = pNextRegion;
        }

        if (pRegion) // not exhausted? or not broken out?
        {
            return KR_MAP_RESULT_SPACE_OCCUPIED;
        }
    }
    
    if (!pInsertAfter)
    {
        pInsertAfter = g_pTailVMR;
    }

    KrVirtualMemoryRegion* pThisNode = KrPrimitiveAcquire(sizeof(KrVirtualMemoryRegion));
    pThisNode->VirtAddrBase = pAddrVirt;
    pThisNode->szPageCount  = PageCount;
    pThisNode->wAcquisitionType = wAcquisitionType;
    pThisNode->wFlags = wFlags;
    pThisNode->uProcID = uProcID;
    pThisNode->pPrev = pInsertAfter;
    pThisNode->pNext = pInsertAfter->pNext;

    // Basically update the node structures so they don't act like uncs and get a twisted sense of reality.
    if (pInsertAfter->pNext)
    {
        pInsertAfter->pNext->pPrev = pThisNode;
    }
    pInsertAfter->pNext = pThisNode;
    if (pInsertAfter == g_pTailVMR)
    {
        g_pTailVMR = pThisNode; // we are the tuff boy now
    }

    // This is our command to map NOW, IMMEDIATELY.
    // Static mapping is for things like: Kernel, Frame Buffer, MMIO etc.
    // No memory management involved, we map given physical to given virtual, effective immediately!
    // Static mapping regions can never be unmapped or modified in any way through VMM API.
    // Once registered, the mapping remains effective as-is until a system reset.
    if (wAcquisitionType == KR_ACQUIRE_STATIC)
    {
        // NOTE: We won't keep track of these PTEs because it's a static mapping.
        // So we literally just ask the PMM for pages to use as page structures when needed.
        KrVirtualAddressMode eVirtAddrMode = (wFlags & KR_PAGE_FLAG_LARGE) ? KR_VAMODE_LARGE : KR_VAMODE_SMALL;
        KrVirtualAddress VirtIndices;

        KrPageTableEntry* pPDPT = NULLPTR,
                        * pPD   = NULLPTR,
                        * pPT   = NULLPTR;

        KrPageTableEntryFlags TopLevelFlags = {0};
        TopLevelFlags.bPresent  = TRUE;
        TopLevelFlags.bWritable = TRUE;
        TopLevelFlags.bUserMode = (uProcID > KR_VMM_PROCID_KERNEL ) ? TRUE  : FALSE;

        // NOTE: The reason we explicitly convert to FALSE/TRUE is because Flags fields are bit fields and compiler likes to mess them up if you give direct exprs.
        KrPageTableEntryFlags Flags = {0};
        Flags.bPresent   = TRUE;
        Flags.bWritable  = (wFlags  & KR_PAGE_FLAG_WRITE   ) ? TRUE  : FALSE;
        Flags.bUserMode  = (uProcID > KR_VMM_PROCID_KERNEL ) ? TRUE  : FALSE;
        Flags.PS         = (wFlags  & KR_PAGE_FLAG_LARGE   ) ? TRUE  : FALSE;
        Flags.bNoExecute = (wFlags  & KR_PAGE_FLAG_EXECUTE ) ? FALSE : TRUE ;

        // Cache behavior
        KrPatSelect PatSel;
        if (wFlags & KR_PAGE_FLAG_UNCACHEABLE)
        {
            PatSel = KrSelectPat(KR_PAT_UNCACHEABLE);
        }
        else if (wFlags & KR_PAGE_FLAG_WRITE_COMBINE)
        {
            PatSel = KrSelectPat(KR_PAT_WRITE_COMBINING);
        }
        else
        {
            PatSel = KrSelectPat(KR_PAT_WRITE_BACK); // default cache behavior
        }
        Flags.PCD = PatSel.PCD;
        Flags.PWT = PatSel.PWT;
        if (!(wFlags & KR_PAGE_FLAG_LARGE))
        {
            Flags.PS = PatSel.PAT; // In a PT, bit 7 is the PAT bit whereas large and huge pages use bit 12, so their functions take in an extra PAT param.
        }

        BOOL bWasAcquire = FALSE; // OUT info param of KrSmapGetOrAcquireStruct()
        for (SIZE i = 0; i < pThisNode->szPageCount; i++)
        {
            VirtIndices = KrVirtBreakdown(pAddrVirt + i * PageGranularity, eVirtAddrMode);

            pPDPT = KrSmapGetOrAcquireStruct(&bWasAcquire, g_PML4, VirtIndices.PML4);
            if (bWasAcquire)
            {
                g_PML4[VirtIndices.PML4] = KrEncodePageTableEntry(KrVirtToPhys((UINTPTR) pPDPT), TopLevelFlags);
            }

            pPD = KrSmapGetOrAcquireStruct(&bWasAcquire, pPDPT, VirtIndices.PDPT);
            if (bWasAcquire)
            {
                pPDPT[VirtIndices.PDPT] = KrEncodePageTableEntry(KrVirtToPhys((UINTPTR) pPD), TopLevelFlags);
            }
            
            if (eVirtAddrMode == KR_PAGE_FLAG_LARGE)
            {
                pPD[VirtIndices.PD] = KrEncodeLargePageEntry(pAddrPhys + i * PageGranularity, Flags, PatSel.PAT);
                continue; // Do NOT execute lower VAMODE logic with PT.
            }

            pPT = KrSmapGetOrAcquireStruct(&bWasAcquire, pPD, VirtIndices.PD);
            if (bWasAcquire)
            {
                pPD[VirtIndices.PD] = KrEncodePageTableEntry(KrVirtToPhys((UINTPTR) pPT), TopLevelFlags);
            }
            pPT[VirtIndices.PT] = KrEncodePageTableEntry(pAddrPhys + i * PageGranularity, Flags);
        }

        // yeah... should be mapped now. i can't say the same about my will to live.
    }

    g_StateVMM.NumVMRs++; // oh, wow.

    return KR_MAP_RESULT_SUCCESS;
}

UINTPTR KrPhysToVirt(UINTPTR AddrPhys)
{
    return g_StateVMM.VirtAddrDmapBase + AddrPhys;
}

UINTPTR KrVirtToPhys(UINTPTR AddrVirt)
{
    return AddrVirt - g_StateVMM.VirtAddrDmapBase;
}

const KrVirtmemmgmtState* KrGetVirtmemmgmtState(VOID)
{
    return &g_StateVMM;
}

const KrVirtualMemoryRegion* KrGetRootVMR(VOID)
{
    return &g_RootVMR;
}
