#include "CPU/Halt.h"

KR_NORETURN VOID KrProcessorHalt(VOID)
{
    __asm__ __volatile__("cli\n\t"); // Clear maskable interrupts
    while (1) // In case CPU woke up, this loop puts it back to sleep
    {
        __asm__ __volatile__("hlt\n\t"); // Halt until NMI occurs
    }
}
