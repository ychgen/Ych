void KrHalt(void)
{
    __asm__ __volatile__("cli"); // Clear maskable interrupts
    while (1) // In case CPU woke up, this loop puts it back to sleep
    {
        __asm__ __volatile__("hlt"); // Halt until NMI occurs
    }
}
