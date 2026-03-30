#include "Core/Krnlmeltdown.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

#include "Core/KernelState.h"
#include "CPU/Halt.h"

const char* Krnlmddesc(meltdowncode_t code)
{
    switch (code)
    {
    #define mkcase(x) case x: return #x
        /** PROCESSOR */
        mkcase(KR_MELTDOWN_CODE_CRITICAL_PROCESSOR_INTERRUPT);
        /** DEBUG */
        mkcase(KR_MELTDOWN_CODE_GENERAL_DEBUG);
    #undef mkcase
    default: break;
    }
    return "IVLDMDCODE";
}

__attribute__((noreturn))
void Krnlmeltdown(meltdowncode_t code, const char* pDesc, const KrProcessorSnapshot* pSnapshot)
{
    if (g_KernelState.VideoOutputProtocol == KR_VIDEO_OUTPUT_PROTOCOL_DISPLAYWIDE_TEXT_PROTOCOL)
    {
        // Clear fbuf and move cursor to start.
        KrdwtpResetState(KRDWTP_COLOR_RED);
        
        KrdwtpOutFormatText
        (
            "\n!!!KERNEL MELTDOWN!!!\n"
            "MDCODE=0x%X,MDDESC=`%s`\n\nPROCESSOR SNAPSPHOT:\n"
            "R15=0x%RX\nR14=0x%RX\nR13=0x%RX\nR12=0x%RX\n"
            "R11=0x%RX\nR10=0x%RX\nR9=0x%RX\nR8=0x%RX\n"
            "RDI=0x%RX\nRSI=0x%RX\nRBP=0x%RX\nRDX=0x%RX\n"
            "RCX=0x%RX\nRBX=0x%RX\nRAX=0x%RX\nRSP=0x%RX\n"
            "RFLAGS=0x%RX\nRIP=0x%RX\n\n!!!KERNEL MELTDOWN!!!\nHard reset your machine.\n",

            (uint32_t) code, pDesc ? pDesc : Krnlmddesc(code),
            pSnapshot->R15, pSnapshot->R14, pSnapshot->R13, pSnapshot->R12,
            pSnapshot->R11, pSnapshot->R10, pSnapshot->R9,  pSnapshot->R8,
            pSnapshot->RDI, pSnapshot->RSI, pSnapshot->RBP, pSnapshot->RDX,
            pSnapshot->RCX, pSnapshot->RBX, pSnapshot->RAX, pSnapshot->RSP,
            pSnapshot->RFLAGS, pSnapshot->RIP
        );
    }

    KrProcessorHalt();
}
