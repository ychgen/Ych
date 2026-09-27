#include "Memory/Virtmemmgmt.h"

#include "Core/Krnlmeltdown.h"
#include "Core/KernelState.h"

#include "CPU/Interrupt.h"
#include "CPU/CPUID.h"
#include "CPU/MSR.h"
#include "CPU/PAT.h"
#include "CPU/CR.h"

#include "Memory/BootstrapArena.h"
#include "Memory/Physmemmgmt.h"
#include "Memory/PageFault.h"

#include "KRTL/Krnlmem.h"

// TODO: Remove later, these are for debugging currently
#include "Earlyvideo/DisplaywideTextProtocol.h"
#include "CPU/Halt.h"

KrVirtmemmgmtState g_StateVMM = {0};

// Include these here (they depend on stuff from above)
#include "PrivateVMM/BSmapInit.h" // for KrInitBootstrapStaticPages()
#include "PrivateVMM/DmapInit.h" // for KrInitDirectMap()
#include "PrivateVMM/RegionNodeAllocator.h"

KR_STATIC_ASSERT(sizeof(KrVirtualMemoryRegion) <= RNA_SLOT_SIZE, "VMR node struct size exceeds RNA slot size.");

static VOID KrVmmInitCpu(VOID);
static BOOL KrInitKrnlAddrSpace(VOID);

static UINT RegionPageSize(WORD Flags) { return Flags & VMR_FLAG_PAGE_SIZE ? 0x200000 : 0x1000; }

BOOL KrInitVirtmemmgmt(VOID)
{
    // See if we are already initialized or sumn
    if (g_StateVMM.bInitialized)
    {
        return FALSE;
    }

    // Feature checking, control registers, PAT MSR setup etc.
    KrVmmInitCpu();

    // This keeps our current mapping and unmapping identity-mapped lower 2 MiB.
    KrVmmInitKernelStaticPages();
    
    // Take control of paging.
    UINTPTR AddrPhysicalPML4 = KrReservedVirtToPhys(g_PML4);
    KrWriteCR3(AddrPhysicalPML4);

    // Our paging configuration must be active before calling this. It depends on using pages it maps like scaffolding once it falls out of the bootstrap arena.
    // It also directly works on g_PML4 and such.
    if (!KrInitDirectMap())
    {
        return FALSE;
    }

    // After direct map initialization we can finally do this
    if (!KrInitPhysMetaArray())
    {
        MDCODE MdCode  = KR_MDCODE_PMM_META_OOM;
        CSTR   pMdDesc = "Dense acquisition for PMM physical page metadata linear array failed! Either memory is too low or fragmentation is too high.";
        Krnlmeltdownimm(MdCode, pMdDesc);
    }

    if (!RnaInit())
    {
        MDCODE MdCode = KR_MDCODE_RNA_INIT_FAILURE;
        CSTR   MdDesc = "Failed to initialize the Region Node Allocator for Virtmemmgmt, initialization cannot proceed.";
        Krnlmeltdownimm(MdCode, MdDesc);
    }

    // Initialize g_StateVMM.KernelAddressSpace & the basic VMRs for the kernel.
    if (!KrInitKrnlAddrSpace())
    {
        MDCODE MdCode = KR_MDCODE_KERNEL_ADDRESS_SPACE_CREATION_FAILURE;
        CSTR MdDesc = "Failed to create the kernel address space and its initial VMRs.";
        Krnlmeltdownimm(MdCode, MdDesc);
    }

    // NOTE: Only and only after all initialization steps should we assign the Page Fault handler.
    // Parameter `bOverwrite`=TRUE for overwrite! You must provide it as TRUE to overwrite the basic KrCriticalProcessorInterrupt handler.
    if (!KrRegisterInterruptHandler(KR_INTERRUPT_VECTOR_PAGE_FAULT, KrGlobalPageFaultHandler, TRUE))
    {
        return FALSE;
    }

    g_StateVMM.bInitialized = TRUE;
    return TRUE;
}

KrVirtualMemoryRegion* VmLocateRegion(KrAddressSpace* pAddressSpace, UINTPTR Vaddr)
{
    if (!pAddressSpace)
    {
        return NULLPTR;
    }

    KrVirtualMemoryRegion* pNode = pAddressSpace->pRootVMR;
    while (pNode)
    {
        if (Vaddr >= pNode->VaddrStart && Vaddr < pNode->VaddrEnd)
        {
            break;
        }

        pNode = pNode->pNext;
    }

    return pNode;
}

BOOL VmVrangeOverlapsRegion(KrVirtualMemoryRegion* pNode, UINTPTR VaddrStart, UINTPTR VaddrEnd)
{
    if (!pNode)
    {
        return FALSE;
    }
    if (VaddrStart >= VaddrEnd)
    {
        return FALSE;
    }
    if (VaddrStart < pNode->VaddrEnd && VaddrEnd > pNode->VaddrStart)
    {
        return TRUE;
    }
    return FALSE;
}

BOOL VmVrangeOverlapsAnyRegions(KrAddressSpace* pAddressSpace, UINTPTR VaddrStart, UINTPTR VaddrEnd)
{
    if (!pAddressSpace)
    {
        return FALSE;
    }
    if (VaddrStart >= VaddrEnd)
    {
        return FALSE;
    }

    KrVirtualMemoryRegion* pNode = pAddressSpace->pRootVMR;
    while (pNode)
    {
        if (VaddrEnd <= pNode->VaddrStart)
        {
            return FALSE; // does not overlap since range end is before the region even starts
        }
        if (VmVrangeOverlapsRegion(pNode, VaddrStart, VaddrEnd))
        {
            return TRUE;
        }
        pNode = pNode->pNext;
    }

    return FALSE;
}

BOOL VmFindInsertPoint(KrAddressSpace* pAddressSpace, UINTPTR VaddrStart, UINTPTR VaddrEnd, KrVirtualMemoryRegion** pBefore, KrVirtualMemoryRegion** pAfter)
{
    if (!pAddressSpace)
    {
        return FALSE;
    }
    if (VaddrStart >= VaddrEnd)
    {
        return FALSE;
    }

    KrVirtualMemoryRegion* pNode = pAddressSpace->pRootVMR;
    while (pNode)
    {
        if (VaddrEnd <= pNode->VaddrStart)
        {
            // Insert before me
            if (pBefore) *pBefore = pNode;
            if (pAfter)  *pAfter = pNode->pPrev;

            return TRUE;
        }

        // Vrange must be ahead of pNode but before pNode->pNext (if any, otherwise irrelevant)
        if (VaddrStart >= pNode->VaddrEnd)
        {
            if (pNode->pNext)
            {
                if (VaddrEnd >= pNode->pNext->VaddrStart)
                {
                    return FALSE; // Ahead of pNode, but overlaps pNext
                }
            }
            
            // insert after me, i.e. before my next
            if (pAfter)  *pAfter  = pNode;
            if (pBefore) *pBefore = pNode->pNext;

            return TRUE;
        }
        pNode = pNode->pNext;
    }

    return FALSE;
}

PTE VmEncodeEntryFor(const KrVirtualMemoryRegion* pNode, KrTypePTE Type, UINTPTR PaddrBase)
{
    KrPatSelect pslLeaf;
    switch (pNode->Flags & VMR_FLAG_CACHING_PROTOCOL)
    {
    case VMR_CACHE_PROTOCOL_WRITE_BACK:    pslLeaf = KrSelectPat(KR_PAT_WRITE_BACK);      break;
    case VMR_CACHE_PROTOCOL_UNCACHEABLE:   pslLeaf = KrSelectPat(KR_PAT_UNCACHEABLE);     break;
    case VMR_CACHE_PROTOCOL_WRITE_COMBINE: pslLeaf = KrSelectPat(KR_PAT_WRITE_COMBINING); break;
    case 0b11: return KR_PTE2_ENCODE_FAILURE_DUE_TO_ALIGNMENT; // invalid value, only these are valid: 00, 01, 10, as handled above
    }

    QWORD qwLeafFlags = PTE_PRESENT;
    if (pNode->Flags & VMR_FLAG_WRITABLE)
    {
        qwLeafFlags |= PTE_WRITABLE;
    }
    if (!(pNode->Flags & VMR_FLAG_ALLOW_CODE_EXEC))
    {
        qwLeafFlags |= PTE_NX;
    }

    if (pNode->pAddressSpace->PaddrRoot != g_StateVMM.KernelAddressSpace.PaddrRoot)
    {
        qwLeafFlags |= PTE_USER;
    }

    return PteEncodeEntry(Type, PaddrBase, qwLeafFlags, pslLeaf);
}

KrVirtualMemoryRegion* VmAcquireRegion(KrAddressSpace* pAddressSpace, UINTPTR VaddrStart, UINTPTR VaddrEnd, WORD Flags)
{
    if (!pAddressSpace)
    {
        return NULLPTR;
    }
    if (VaddrStart >= VaddrEnd || VaddrStart == VaddrEnd)
    {
        return NULLPTR;
    }

    if (pAddressSpace->PaddrRoot == g_StateVMM.KernelAddressSpace.PaddrRoot)
    {
        if (VaddrStart < KR_MAKE_VIRTUAL(KRNL_PML4_IDX, 0, 0, 0, 0))
        {
            return NULLPTR;
        }
    }
    else if (VaddrStart >= KR_MAKE_VIRTUAL(KRNL_PML4_IDX, 0, 0, 0, 0))
    {
        return NULLPTR;
    }

    const UINT PageSize = RegionPageSize(Flags);
    if (!(KrtlIsPowerOfTwoAligned(VaddrStart, PageSize) && KrtlIsPowerOfTwoAligned(VaddrEnd, PageSize)))
    {
        return NULLPTR;
    }

    KrVirtualMemoryRegion* pInsertBefore = NULLPTR, *pInsertAfter = NULLPTR;
    if (!VmFindInsertPoint(pAddressSpace, VaddrStart, VaddrEnd, &pInsertBefore, &pInsertAfter))
    {
        return NULLPTR;
    }

    KrVirtualMemoryRegion* pNode = RnaAcquireNode();
    if (!pNode)
    {
        return NULLPTR;
    }

    pNode->pAddressSpace = pAddressSpace;
    pNode->VaddrStart = VaddrStart;
    pNode->VaddrEnd = VaddrEnd;
    pNode->Flags = Flags;

    if (pInsertBefore)
    {
        pInsertBefore->pPrev = pNode;
    }
    if (pInsertAfter)
    {
        pInsertAfter->pNext = pNode;
    }
    pNode->pPrev = pInsertAfter;
    pNode->pNext = pInsertBefore;

    if (pInsertBefore == pAddressSpace->pRootVMR)
    {
        pAddressSpace->pRootVMR = pNode;
    }
    if (pInsertAfter == pAddressSpace->pTailVMR)
    {
        pAddressSpace->pTailVMR = pNode;
    }

    pAddressSpace->NrVMRs++;
    return pNode;
}

BOOL VmRelinquishRegion(KrVirtualMemoryRegion* pNode)
{
    // TODO: Implement
    return FALSE;
}

BOOL VmMapStatic(KrVirtualMemoryRegion* pNode, UINTPTR PaddrStart)
{
    if (!(pNode && pNode->pAddressSpace && KrtlIsPowerOfTwoAligned(PaddrStart, RegionPageSize(pNode->Flags))))
    {
        return FALSE;
    }
    if (!(pNode->Flags & VMR_FLAG_STATIC))
    {
        return FALSE;
    }

    const SIZE MappingSize = (SIZE)(pNode->VaddrEnd - pNode->VaddrStart);
    const UINTPTR PaddrEnd = PaddrStart + (pNode->VaddrEnd - pNode->VaddrStart);

    KrVirtualAddressMode VaddrMode = VADDR_SMALL;
    if (pNode->Flags & KR_PAGE_SIZE)
    {
        VaddrMode = VADDR_LARGE;
    }
    
    KrVirtualAddress Vidx = {0};
    PAGESTRUCT PML4 = (PAGESTRUCT) KrPhysToVirt(pNode->pAddressSpace->PaddrRoot), PDPT = NULLPTR, PD = NULLPTR, PT = NULLPTR;
    
    for (UINTPTR MappingOffset = 0; MappingOffset < MappingSize; MappingOffset += RegionPageSize(pNode->Flags))
    {
        // TODO: Handle partial failures
        Vidx = KrUnmakeVirtual(VaddrMode, pNode->VaddrStart + MappingOffset);

        PDPT = PteGetOrAcquirePageStruct(PML4, PML4_ENTRY, Vidx.PML4, PTE_PRESENT | PTE_WRITABLE, g_pslDefault);
        if (!PDPT)
        {
            return FALSE;
        }

        PD = PteGetOrAcquirePageStruct(PDPT, PDPT_ENTRY, Vidx.PDPT, PTE_PRESENT | PTE_WRITABLE, g_pslDefault);
        if (!PD)
        {
            return FALSE;
        }

        if (VaddrMode == VADDR_LARGE)
        {
            PD[Vidx.PD] = VmEncodeEntryFor(pNode, PD2MB_ENTRY, PaddrStart + MappingOffset);
            continue;
        }

        PT = PteGetOrAcquirePageStruct(PD, PD_ENTRY, Vidx.PD, PTE_PRESENT | PTE_WRITABLE, g_pslDefault);
        if (!PT)
        {
            return FALSE;
        }

        PT[Vidx.PT] = VmEncodeEntryFor(pNode, PT_ENTRY, PaddrStart + MappingOffset);
    }

    return TRUE;
}

UINTPTR KrPhysToVirt(UINTPTR AddrPhys)
{
    return g_StateVMM.DmapInfo.VirtAddrBase + AddrPhys;
}

UINTPTR KrVirtToPhys(UINTPTR AddrVirt)
{
    return AddrVirt - g_StateVMM.DmapInfo.VirtAddrBase;
}

const KrVirtmemmgmtState* KrGetVirtmemmgmtState(VOID)
{
    return &g_StateVMM;
}

BOOL KrIsVirtmemmgmtInitialized(VOID)
{
    return g_StateVMM.bInitialized;
}

KrAddressSpace* KrGetKernelAddressSpace(VOID)
{
    return &g_StateVMM.KernelAddressSpace;
}

static VOID KrVmmInitCpu(VOID)
{
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
            QWORD msrEFER = KrReadMSR(KR_MSR_IA32_EFER);
            msrEFER |= KR_MSR_IA32_EFER_NXE;
            KrWriteMSR(KR_MSR_IA32_EFER, msrEFER);
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
        g_pslDefault = KrSelectPat(KR_PAT_WRITE_BACK);
    }
}

static BOOL KrInitKrnlAddrSpace(VOID)
{
    g_StateVMM.KernelAddressSpace.PaddrRoot = KrReservedVirtToPhys(g_PML4);
    
    KrVirtualMemoryRegion* pBinaryNode = RnaAcquireNode();
    KrVirtualMemoryRegion* pFrameBufferNode = RnaAcquireNode();
    
    if (!(pBinaryNode && pFrameBufferNode))
    {
        return FALSE;
    }
    
    pBinaryNode->pAddressSpace = &g_StateVMM.KernelAddressSpace;
    pBinaryNode->VaddrStart = g_KernelState.LoadInfo.AddrVirtualBase;
    pBinaryNode->VaddrEnd = pBinaryNode->VaddrStart + g_KernelState.LoadInfo.ReserveSize;
    pBinaryNode->Flags = VMR_FLAG_STATIC | VMR_FLAG_READABLE | VMR_FLAG_ALLOW_CODE_EXEC | VMR_FLAG_PAGE_SIZE;

    pFrameBufferNode->pAddressSpace = &g_StateVMM.KernelAddressSpace;
    pFrameBufferNode->VaddrStart = g_KernelState.FrameBufferInfo.VirtualAddress;
    pFrameBufferNode->VaddrEnd = pBinaryNode->VaddrStart + g_KernelState.FrameBufferInfo.Size;
    pFrameBufferNode->Flags = VMR_FLAG_STATIC | VMR_FLAG_WRITABLE | VMR_FLAG_PAGE_SIZE;

    if (pBinaryNode->VaddrStart < pFrameBufferNode->VaddrStart)
    {
        g_StateVMM.KernelAddressSpace.pRootVMR = pBinaryNode;
        g_StateVMM.KernelAddressSpace.pTailVMR = pFrameBufferNode;
    }
    else
    {
        g_StateVMM.KernelAddressSpace.pRootVMR = pFrameBufferNode;
        g_StateVMM.KernelAddressSpace.pTailVMR = pBinaryNode;
    }

    g_StateVMM.KernelAddressSpace.pRootVMR->pNext = g_StateVMM.KernelAddressSpace.pTailVMR;
    g_StateVMM.KernelAddressSpace.pTailVMR->pPrev = g_StateVMM.KernelAddressSpace.pRootVMR;

    g_StateVMM.KernelAddressSpace.NrVMRs = 2;
    return TRUE;
}
