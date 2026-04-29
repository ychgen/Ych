#include "Core/Krnlmeltdown.h"

#include "Core/KernelState.h"

#include "CPU/Interrupt.h"
#include "CPU/Halt.h"
#include "CPU/CR.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"\

#define MDMSGPREFIX  "\n!!!KERNEL MELTDOWN!!!\nMDCODE=0x%RX(%s),MDDESC=`%s`\n"
#define MDMSGPOSTFIX "\n!!!KERNEL MELTDOWN!!!\nKernel bailing out, only God can resurrect the system as-is.\nHard reset your machine, the system is now halted.\n"

CSTR Krnlmddesc(MDCODE mdCode);

KR_NORETURN VOID Krnlmeltdown(MDCODE mdCode, CSTR szDesc, const KrProcessorSnapshot* pSnapshot)
{
    KrDisableInterrupts();
    g_KernelState.bMeltdown = TRUE;

    if (g_KernelState.VideoOutputProtocol == KR_VIDEO_OUTPUT_PROTOCOL_DISPLAYWIDE_TEXT)
    {
        // Clear fbuf and move cursor to start.
        KrdwtpResetState(KRDWTP_COLOR_RED);

        if (pSnapshot)
        {
            QWORD CR0; KrReadCR0(CR0);
            QWORD CR2; KrReadCR2(CR2);
            QWORD CR3; KrReadCR3(CR3);
            QWORD CR4; KrReadCR4(CR4);

            KrdwtpOutFormatText
            (
                 MDMSGPREFIX "\n"
                "R15=0x%RX\nR14=0x%RX\nR13=0x%RX\nR12=0x%RX\n"
                "R11=0x%RX\nR10=0x%RX\nR9=0x%RX\nR8=0x%RX\n"
                "RDI=0x%RX\nRSI=0x%RX\nRBP=0x%RX\nRDX=0x%RX\n"
                "RCX=0x%RX\nRBX=0x%RX\nRAX=0x%RX\nRSP=0x%RX\n"
                "RFLAGS=0x%RX\nRIP=0x%RX\n"
                "CR0=0x%RX\nCR2=0x%RX\nCR3=0x%RX\nCR4=0x%RX\n"
                 MDMSGPOSTFIX,

                mdCode, Krnlmddesc(mdCode), szDesc,
                pSnapshot->R15, pSnapshot->R14, pSnapshot->R13, pSnapshot->R12,
                pSnapshot->R11, pSnapshot->R10, pSnapshot->R9,  pSnapshot->R8,
                pSnapshot->RDI, pSnapshot->RSI, pSnapshot->RBP, pSnapshot->RDX,
                pSnapshot->RCX, pSnapshot->RBX, pSnapshot->RAX, pSnapshot->RSP,
                pSnapshot->RFLAGS, pSnapshot->RIP,
                CR0, CR2, CR3, CR4
            );
        }
        else
        {
            KrdwtpOutFormatText(MDMSGPREFIX "\nIrregular call to Krnlmeltdown routine;\nNo processor snapshot was provided.\n" MDMSGPOSTFIX, mdCode, Krnlmddesc(mdCode), szDesc);
        }
    }

    // Halt indefinitely.
    KrProcessorHalt();
}

CSTR Krnlmddesc(MDCODE mdCode)
{
    switch (mdCode)
    {
    #define mkcase(x) case x: return #x
        /** PROCESSOR */
        mkcase(KR_MDCODE_CRITICAL_PROCESSOR_EXCEPTION);
        mkcase(KR_MDCODE_PROCESSOR_X2APIC_INCAPABLE);
        /** MEMORY */
        mkcase(KR_MDCODE_PHYSMEMMGMT_INIT_FAILURE);
        mkcase(KR_MDCODE_PHYSMEMMGMT_TEST_FAILURE);
        mkcase(KR_MDCODE_VIRTMEMMGMT_INIT_FAILURE);
        mkcase(KR_MDCODE_LOCAL_APIC_MAP_FAILURE);
        mkcase(KR_MDCODE_PAGE_FAULT);
        /** DEBUG */
        mkcase(KR_MDCODE_GENERAL_DEBUG);
        mkcase(KR_MDCODE_KERNEL_START_RETURNS);
    #undef mkcase
    default: break;
    }
    return "IVLDMDCODE"; // InVaLiD MeltDown CODE
}
