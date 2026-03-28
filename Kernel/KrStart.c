#include <stdint.h>

void KrHalt(void)
{
    __asm__ __volatile__("cli"); // Clear maskable interrupts
    while (1) // In case CPU woke up, this loop puts it back to sleep
    {
        __asm__ __volatile__("hlt"); // Halt until NMI occurs
    }
}

void KrStart(void* pFramebuffer)
{
    volatile uint8_t* framebuffer = (volatile uint8_t*) pFramebuffer;
    // Goofy, but writing 64 FFs should certainly change some stuff on screen. We just need proof that kernel works.
    for (int i = 0; i < 64; ++i)
    {
        framebuffer[i] = 0xFF;
    }

    KrHalt();
}
