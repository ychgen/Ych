#include "Init/KrInitMem.h"

#include "Core/KernelState.h"
#include "Core/Krnlmeltdown.h"

#include "Memory/Physmemmgmt.h"
#include "Memory/Virtmemmgmt.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

VOID KrInitMem(VOID)
{
    // Initialize Physical Memory Management
    if (!KrInitPhysmemmgmt())
    {
        MDCODE code = KR_MDCODE_PHYSMEMMGMT_INIT_FAILURE;
        CSTR pDesc = "Failed to initialize Physmemmgmt.";
        Krnlmeltdownimm(code, pDesc);
    }
    KrdwtpOutColoredText("Initialized Physmemmgmt (Physical Memory Management).\n", KRDWTP_COLOR_GREEN, KRDWTP_BACKGROUND);
    {
        const KrPhysmemmgmtState* pStatePMM = GetPhysmemmgmtState();
        KrdwtpOutFormatText
        (
            "Physical Memory Information:\n"
            " -> Page Size: %Ru (%Ru KiB)\n"
            " -> Total Pages: %Ru (%Ru MiB)\n"
            " -> Unusable Pages: %Ru (%Ru MiB)\n"
            " -> Total Usable Pages: %Ru (%Ru MiB)\n",
            pStatePMM->PageSize, pStatePMM->PageSize / 1024,
            pStatePMM->TotalPages, pStatePMM->TotalPages * pStatePMM->PageSize / 1024 / 1024,
            pStatePMM->UnusablePages, pStatePMM->UnusablePages * pStatePMM->PageSize / 1024 / 1024,
            (pStatePMM->TotalPages - pStatePMM->UnusablePages), (pStatePMM->TotalPages - pStatePMM->UnusablePages) * pStatePMM->PageSize / 1024 / 1024
        );
    }

    // Test acquisition/relinquishment
    {
        PAGEID TestPageID = KrAcquirePhysicalPage(KR_INVALID_PAGEID);
        if (TestPageID == KR_INVALID_PAGEID)
        {
            MDCODE code = KR_MDCODE_PHYSMEMMGMT_TEST_FAILURE;
            CSTR pDesc = "Test page acquisition from Physmemmgmt failed!";
            Krnlmeltdownimm(code, pDesc);
        }
        KrdwtpOutFormatText("Acquired test page from Physmemmgmt: ID = %Ru, Address = %p\n", TestPageID, KrGetPhysicalPageAddress(TestPageID));
        if (!KrRelinquishPhysicalPage(TestPageID))
        {
            MDCODE code = KR_MDCODE_PHYSMEMMGMT_TEST_FAILURE;
            CSTR pDesc = "Test page relinquishment to Physmemmgmt failed!";
            Krnlmeltdownimm(code, pDesc);
        }
        KrdwtpOutColoredText("Relinquished test page to Physmemmgmt.\n", KRDWTP_COLOR_CYAN, KRDWTP_BACKGROUND);
    }

    // Init VMM
    if (!KrInitVirtmemmgmt())
    {
        MDCODE code = KR_MDCODE_VIRTMEMMGMT_INIT_FAILURE;
        CSTR pDesc = "Failed to initialize Virtmemmgmt.";
        Krnlmeltdownimm(code, pDesc);
    }
    KrdwtpOutColoredText("Initialized Virtmemmgmt (Virtual Memory Management).\n", KRDWTP_COLOR_GREEN, KRDWTP_BACKGROUND);
    {
        const KrVirtmemmgmtState* pStateVMM = GetVirtmemmgmtState();
        KrdwtpOutFormatText
        (
            "Virtual Memory DMAP Information:\n"
            " -> Huge  Pages (1GiB) : %Ru\n"
            " -> Large Pages (2MiB) : %Ru\n"
            " -> Small Pages (4KiB) : %Ru\n"
            " -> Total Pages (H+L+S): %Ru\n",
            pStateVMM->HugePages, pStateVMM->LargePages, pStateVMM->SmallPages, pStateVMM->TotalPages
        );
    }
}
