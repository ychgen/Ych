#include "Core/Krnlmeltdown.h"

#include "Core/KernelState.h"

#include "CPU/Interrupt.h"
#include "CPU/Spinlock.h"
#include "CPU/Halt.h"
#include "CPU/CR.h"

#include "Intmgmt/NMI.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

#define MDMSGWARNPREFIX "\n!!!KERNEL MELTDOWN!!!\n"
#define MDMSGCATASTROPHICPREFIX "\n!!!CATASTRPHIC KERNEL MELTDOWN!!!\n"

#define MDMSGPREFIX  "MDCODE=0x%RX(%s),MDDESC=`%s`\n"
#define MDMSGPOSTFIX "\n!!!KERNEL MELTDOWN!!!\nKernel bailing out, only God can resurrect the system as-is.\nHard reset your machine, the system is now halted.\n"

static Spinlock sg_MeltdownSpinlock = {0};

KR_NORETURN VOID Krnlmeltdown(MDCODE MdCode, CSTR MdDesc, const KrProcessorSnapshot* pSnapshot)
{
    AcquireOrWaitSpinlock(&sg_MeltdownSpinlock); // We won't let go of this

    KrDisableInterrupts();
    g_KernelState.bMeltdown = TRUE;

    UINT NrCpusComplied;
    if (g_KernelState.SmpInfo.ActiveProcessorCount > 1)
    {
        // Make sure all other CPUs stop
        NmiMeltdownData NmiData = {
            .MdCode = MdCode
        };
        RaiseGlobalNonmaskableInterrupt(FALSE, NMI_REASON_MELTDOWN, &NmiData, &NrCpusComplied);

        // -1 because we, the meltdown CPU, are excluded as we called RaiseGlobalNonmaskableInterrupt with bInterruptSelf=FALSE
        g_KernelState.bMeltdownCatastrophic = NrCpusComplied != g_KernelState.SmpInfo.ActiveProcessorCount - 1;
    }
    else
    {
        NrCpusComplied = 1;
    }

    if (g_KernelState.VideoOutputProtocol == KR_VIDEO_OUTPUT_PROTOCOL_DISPLAYWIDE_TEXT)
    {
        // Clear fbuf and move cursor to start.
        KrdwtpResetState(KRDWTP_MAKE_COLOR(0x2E, 0x00, 0x45, 0xFF));

        KrdwtpOutText(g_KernelState.bMeltdownCatastrophic ? MDMSGCATASTROPHICPREFIX : MDMSGWARNPREFIX);
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

                MdCode, Krnlmddesc(MdCode), MdDesc,
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
            KrdwtpOutFormatText(MDMSGPREFIX "\nIrregular call to Krnlmeltdown routine;\nNo processor snapshot was provided.\n" MDMSGPOSTFIX, MdCode, Krnlmddesc(MdCode), MdDesc);
        }
    }

    if (g_KernelState.bMeltdownCatastrophic)
    {
        KrdwtpOutFormatText("\nA total of %u logical processor(s) did not respond to nor acknowledged the meltdown nonmaskable interrupt!"
        " It is advised that you reset the machine immediately in case that/those logical processor(s) try/tries to do things!\n"
        "This is why this meltdown is classified as catastrophic!", g_KernelState.SmpInfo.ActiveProcessorCount - NrCpusComplied);
    }

    // Halt indefinitely.
    ProcessorIndefiniteHalt();
}
