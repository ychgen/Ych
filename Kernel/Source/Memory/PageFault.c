#include "Memory/PageFault.h"

#include "Core/Krnlmeltdown.h"

#include "CPU/CR.h"

#include "Memory/Physmemmgmt.h"
#include "Memory/Virtmemmgmt.h"

#include "KRTL/Krnlstring.h"

/** TODO: When we move to SMP, this has to be apart of Thread-Local data. Since we are single-core for now, it's okay. */
static BOOL g_bInprocFault = FALSE;

// Called by KrGlobalPageFaultHandler when it decides the kernel should meltdown, for example when a supervisor guard page is accessed by the kernel.
static VOID GiveUp(CSTR szMdDesc, const KrInterruptFrame* pInterruptFrame)
{
    MDCODE mdCode   = KR_MDCODE_PAGE_FAULT;
    KrProcessorSnapshot Snapshot = KrInterruptFrameToProcessorSnapshot(pInterruptFrame);
    Krnlmeltdown(mdCode, szMdDesc, &Snapshot);
}

VOID KrGlobalPageFaultHandler(const KrInterruptFrame* pInterruptFrame)
{
    if (g_bInprocFault)
    {
        GiveUp("Recursive page fault occurrence detected. This means the page fault itself has resulted in a page fault. Unsafe to continue.", pInterruptFrame);
    }
    // We set this to TRUE on function entry, we must set it to FALSE on return.
    g_bInprocFault = TRUE;

    /** The virtual address that caused the page fault. */
    UINTPTR VirtAddrFault;
    KrReadCR2(VirtAddrFault); // CR2 contains the faulting address upon #PF raise by processor.

    // If FALSE, fault caused by a NON-PRESENT page. If TRUE, fault caused by PAGE-LEVEL PROTECTION VIOLATION.
    const BOOL bWasPresent = pInterruptFrame->ErrorCode &  0x1;
    // If FALSE, fault caused by a READ operation. If TRUE, fault caused by a WRITE operation.
    const BOOL bWasWrite   = pInterruptFrame->ErrorCode &  0x2;
    // If FALSE, fault caused by KERNEL. If TRUE, fault caused by USER/PROCESS.
    // NOTE: Not about permissions themselves, this explains the origin, i.e. if kernel or user code caused the fault.
    const BOOL bWasUser    = pInterruptFrame->ErrorCode &  0x4;
    // If FALSE, no reserved bit violation detected. If TRUE, reserved bit violation detected.
    // This is set when CPU sees bits in PTE that are supposed to be reserved and supposed to be unset, but they were set.
    const BOOL bReserved   = pInterruptFrame->ErrorCode &  0x8;
    // If FALSE, fault NOT caused by instruction fetch. If TRUE, fault CAUSED by INSTRUCTION FETCH.
    const BOOL bInstrFetch = pInterruptFrame->ErrorCode & 0x10;

    g_bInprocFault = FALSE;
    // return here normally, but since we don't have the complete function implemented as of now, we'll just invoke meltdown

    CHAR BUF[64];
    KrtlUnsignedToString(BUF, pInterruptFrame->ErrorCode, KRTL_RADIX_HEXADECIMAL, KRTL_HEX_UPPERCASE);
    GiveUp(BUF, pInterruptFrame);
}
