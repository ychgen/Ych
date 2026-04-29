#include "Core/Krnlmeltdown.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

#include "Core/KernelState.h"
#include "CPU/Halt.h"

#define MDMSGPREFIX  "\n!!!KERNEL MELTDOWN!!!\nMDCODE=0x%RX(%s),MDDESC=`%s`\n"
#define MDMSGPOSTFIX "\n!!!KERNEL MELTDOWN!!!\nKernel bailing out, only God can resurrect the system as-is.\nHard reset your machine, the system is now halted.\n"

CSTR Krnlmddesc(MDCODE code)
{
    switch (code)
    {
    #define mkcase(x) case x: return #x
        /** PROCESSOR */
        mkcase(KR_MDCODE_CRITICAL_PROCESSOR_EXCEPTION);
        /** MEMORY */
        mkcase(KR_MDCODE_PHYSMEMMGMT_INIT_FAILURE);
        mkcase(KR_MDCODE_PHYSMEMMGMT_TEST_FAILURE);
        mkcase(KR_MDCODE_VIRTMEMMGMT_INIT_FAILURE);
        mkcase(KR_MDCODE_LOCAL_APIC_MAP_FAILURE);
        /** DEBUG */
        mkcase(KR_MDCODE_GENERAL_DEBUG);
        mkcase(KR_MDCODE_KERNEL_START_RETURNS);
    #undef mkcase
    default: break;
    }
    return "IVLDMDCODE"; // InVaLiD MeltDown CODE
}

KR_NORETURN VOID Krnlmeltdown(MDCODE code, CSTR pDesc, const KrProcessorSnapshot* pSnapshot)
{
    __asm__ __volatile__("cli\n\t");
    g_KernelState.bMeltdown = TRUE;

    uint64_t mdcode = (uint64_t) code;
    if (g_KernelState.VideoOutputProtocol == KR_VIDEO_OUTPUT_PROTOCOL_DISPLAYWIDE_TEXT)
    {
        // Clear fbuf and move cursor to start.
        KrdwtpResetState(KRDWTP_COLOR_RED);

        if (pSnapshot)
        {
            KrdwtpOutFormatText
            (
                 MDMSGPREFIX "\n"
                "R15=0x%RX\nR14=0x%RX\nR13=0x%RX\nR12=0x%RX\n"
                "R11=0x%RX\nR10=0x%RX\nR9=0x%RX\nR8=0x%RX\n"
                "RDI=0x%RX\nRSI=0x%RX\nRBP=0x%RX\nRDX=0x%RX\n"
                "RCX=0x%RX\nRBX=0x%RX\nRAX=0x%RX\nRSP=0x%RX\n"
                "RFLAGS=0x%RX\nRIP=0x%RX\n"
                 MDMSGPOSTFIX,

                mdcode, Krnlmddesc(code), pDesc,
                pSnapshot->R15, pSnapshot->R14, pSnapshot->R13, pSnapshot->R12,
                pSnapshot->R11, pSnapshot->R10, pSnapshot->R9,  pSnapshot->R8,
                pSnapshot->RDI, pSnapshot->RSI, pSnapshot->RBP, pSnapshot->RDX,
                pSnapshot->RCX, pSnapshot->RBX, pSnapshot->RAX, pSnapshot->RSP,
                pSnapshot->RFLAGS, pSnapshot->RIP
            );
        }
        else
        {
            KrdwtpOutFormatText(MDMSGPREFIX "\nIrregular call to Krnlmeltdown routine;\nNo processor snapshot was provided.\n" MDMSGPOSTFIX, mdcode, Krnlmddesc(code), pDesc);
        }
    }

    // Since frame buffer is mapped as WC (Write-Combining), let's wait until all that is flushed, and only then halt.
    __asm__ __volatile__ ("sfence\n\t" ::: "memory");
    // Halt indefinitely.
    while (1)
    {
        __asm__ __volatile__("cli\n\thlt\n\t");
    }
}
