#include "Intmgmt/PageFault.h"

#include "Core/Krnlmeltdown.h"
#include "Core/KernelState.h"
#include "Core/Assert.h"

#include "CPU/PerCpu.h"
#include "CPU/CR.h"

#include "Memory/Physmemmgmt.h"
#include "Memory/Virtmemmgmt.h"

#include "KRTL/Krnlstring.h"
#include "KRTL/Krnlmem.h"

#include "Memory/Virtmemmgmt.h"
#include "Memory/PTE.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

#define PFEC_PRESENT     (1 << 0)
#define PFEC_WRITE       (1 << 1)
#define PFEC_USER        (1 << 2)
#define PFEC_INSTRUCTION (1 << 4)

// Called by KrGlobalPageFaultHandler when it decides the kernel should meltdown, for example when a supervisor guard page is accessed by the kernel.
STATIC VOID GiveUp(CSTR szMdDesc, const InterruptFrame* pInterruptFrame)
{
    MDCODE mdCode   = KR_MDCODE_PAGE_FAULT;
    KrProcessorSnapshot Snapshot = InterruptFrameToProcessorSnapshot(pInterruptFrame);
    Krnlmeltdown(mdCode, szMdDesc, &Snapshot);
}

VOID IntmgmtPageFault(const InterruptFrame* pInterruptFrame)
{
    BOOL bFaultHandled = FALSE;
    UINTPTR VaddrFault; // The virtual address that caused the page fault
    KrReadCR2(VaddrFault);

    PerCpu* pThisCpu = GetThisCpu();
    if (pThisCpu->bInPageFault)
    {
        GiveUp("Recursive page fault occurrence detected. This means the page fault itself has resulted in a page fault. Unsafe to continue.", pInterruptFrame);
    }
    if (!KrIsVirtmemmgmtInitialized())
    {
        goto AbortMission;
    }
    pThisCpu->bInPageFault = TRUE; // We set this to TRUE on function entry, we must set it to FALSE on return.

    // For now we only handle supervisor, data-related demand paging page faults
    // This mandates the exception to be caused from nonpresent entry, occur in supervisor (aka not user) and to be data-related (aka not instr.)
    if (!(pInterruptFrame->ErrorCode & PFEC_PRESENT) && !(pInterruptFrame->ErrorCode & PFEC_USER) && !(pInterruptFrame->ErrorCode & PFEC_INSTRUCTION))
    {
        // TODO: (Overall TODO)
        // Currently VmMapStatic and this use distinct code blocks to do relatively the same thing. Unify that some time into a Page Walk system 


        KrVirtualMemoryRegion* pRegion = VmLocateRegion(KrGetKernelAddressSpace(), VaddrFault);
        if (!pRegion)
        {
            goto AbortMission;
        }
        if (pRegion->Flags & (VMR_FLAG_STATIC | VMR_FLAG_GUARD))
        {
            goto AbortMission;
        }

        KrVirtualAddressMode VaddrMode = VADDR_SMALL;
        if (pRegion->Flags & VMR_FLAG_PAGE_SIZE)
        {
            goto AbortMission; // Demand paging 2 MiB pages is forbidden
            VaddrMode = VADDR_LARGE;
        }
        const KrVirtualAddress Vidx = KrUnmakeVirtual(VaddrMode, VaddrFault);

        const PAGEID PhysPageID = PmAcquirePage(PAGE_TYPE_GENERAL, IVLDPGID);
        if (PhysPageID == IVLDPGID)
        {
            goto AbortMission;
        }
        const UINTPTR PaddrPage = KrGetPhysicalPageAddress(PhysPageID);

        PAGESTRUCT PML4 = (PAGESTRUCT) KrPhysToVirt(KrGetKernelAddressSpace()->PaddrRoot);
        PAGESTRUCT PDPT = PteGetOrAcquirePageStruct(PML4, PML4_ENTRY, Vidx.PML4, PTE_PRESENT | PTE_WRITABLE, g_pslDefault);

        if (!PDPT)
        {
            PmRelinquishPage(PhysPageID);
            goto AbortMission;
        }

        PAGESTRUCT PD = PteGetOrAcquirePageStruct(PDPT, PDPT_ENTRY, Vidx.PDPT, PTE_PRESENT | PTE_WRITABLE, g_pslDefault);
        if (!PD)
        {
            PmRelinquishPage(PhysPageID);
            goto AbortMission;
        }

        PAGESTRUCT PT = PteGetOrAcquirePageStruct(PD, PD_ENTRY, Vidx.PD, PTE_PRESENT | PTE_WRITABLE, g_pslDefault);
        if (!PT)
        {
            PmRelinquishPage(PhysPageID);
            goto AbortMission;
        }

        PT[Vidx.PT] = VmEncodeEntryFor(pRegion, PT_ENTRY, PaddrPage);
        if (PT[Vidx.PT] == KR_PTE2_ENCODE_FAILURE_DUE_TO_ALIGNMENT)
        {
            PmRelinquishPage(PhysPageID);
            goto AbortMission;
        }
        
        // Make sure to zero out the allocated page immediately!
        KrtlContiguousZeroBuffer((VOID*) KrPhysToVirt(PaddrPage), KR_PAGE_SIZE);

        bFaultHandled = TRUE;
    }

AbortMission:
    pThisCpu->bInPageFault = FALSE;
    
    RUNTIME_ENSURE(bFaultHandled, KR_MDCODE_PAGE_FAULT,
        "Unrecoverable page fault occurred with error code: `0x%X`, faulting address: %p",
        pInterruptFrame->ErrorCode, VaddrFault);
}
