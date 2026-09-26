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

KrVirtualMemoryRegion* KrLocateVMR(KrAddressSpace* pAddressSpace, UINTPTR Vaddr)
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

BOOL KrVrangeOverlapsVMR(KrVirtualMemoryRegion* pNode, UINTPTR VaddrStart, UINTPTR VaddrEnd)
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

BOOL KrVrangeOverlapsAnyVMRs(KrAddressSpace* pAddressSpace, UINTPTR VaddrStart, UINTPTR VaddrEnd)
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
        if (KrVrangeOverlapsVMR(pNode, VaddrStart, VaddrEnd))
        {
            return TRUE;
        }
        pNode = pNode->pNext;
    }

    return FALSE;
}

BOOL KrFindInsertPointVMR(KrAddressSpace* pAddressSpace, UINTPTR VaddrStart, UINTPTR VaddrEnd, KrVirtualMemoryRegion** pBefore, KrVirtualMemoryRegion** pAfter)
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

KrVirtualMemoryRegion* KrAcquireVMR(KrAddressSpace* pAddressSpace, UINTPTR VaddrStart, UINTPTR VaddrEnd, WORD Flags)
{
    if (!pAddressSpace)
    {
        return NULLPTR;
    }

    KrVirtualMemoryRegion* pInsertBefore = NULLPTR, *pInsertAfter = NULLPTR;
    if (!KrFindInsertPointVMR(pAddressSpace, VaddrStart, VaddrEnd, &pInsertBefore, &pInsertAfter))
    {
        return NULLPTR;
    }

    KrVirtualMemoryRegion* pNode = RnaAcquireNode();
    if (!pNode)
    {
        return NULLPTR;
    }

    KrdwtpOutFormatText("IB %p, IA = %p\n", pInsertBefore, pInsertAfter);

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
