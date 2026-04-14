#include "CPU/Halt.h"

#include "Core/KernelState.h"
#include "Earlyvideo/DisplaywideTextProtocol.h"

__attribute__((noreturn))
VOID KrProcessorHalt(VOID)
{
    if (!g_KernelState.bMeltdown && KrdwtpGetProtocolState()->bIsActive)
    {
        KrdwtpOutColoredText("KrProcessorHalt(VOID) called, halting the processor indefinitely...", KRDWTP_COLOR_YELLOW, KRDWTP_BACKGROUND);
    }

    __asm__ __volatile__("cli"); // Clear maskable interrupts
    while (1) // In case CPU woke up, this loop puts it back to sleep
    {
        __asm__ __volatile__("hlt"); // Halt until NMI occurs
    }
}
