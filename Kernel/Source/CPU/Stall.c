#include "CPU/Stall.h"

#include "CPU/Halt.h"

static QWORD ReadTSC(VOID);

VOID KrProcessorStall(ULONG ClockCycles)
{
    QWORD InitialTSC = ReadTSC();
    while (ReadTSC() < InitialTSC + ClockCycles)
    {
        KrProcessorPause();
    }
}

static QWORD ReadTSC(VOID)
{
    DWORD Lodword;
    DWORD Hidword;
    
    __asm__ __volatile__
    (
        "lfence\n\t"
        "rdtsc\n\t"
        : "=a"(Lodword), "=d"(Hidword)
        :
        : "memory"
    );

    return (((QWORD) Hidword) << 32) | Lodword;
}
