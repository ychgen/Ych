#ifndef YCH_KERNEL_CPU_PROCESSOR_SNAPSHOT_H
#define YCH_KERNEL_CPU_PROCESSOR_SNAPSHOT_H

#include "Core/Fundtypes.h"

typedef struct __attribute__((aligned(8)))
{
    QWORD R15, R14, R13, R12, R11, R10, R9, R8;
    QWORD RDI, RSI, RBP, RDX, RCX, RBX, RAX;

    QWORD RFLAGS;
    QWORD RSP;
    QWORD RIP;
} KrProcessorSnapshot;

#endif // !YCH_KERNEL_CPU_PROCESSOR_SNAPSHOT_H
