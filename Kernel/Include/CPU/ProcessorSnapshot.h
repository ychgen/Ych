#ifndef YCH_KERNEL_CPU_PROCESSOR_SNAPSHOT_H
#define YCH_KERNEL_CPU_PROCESSOR_SNAPSHOT_H

#include <stdint.h>

typedef struct __attribute__((aligned(8)))
{
    uint64_t R15, R14, R13, R12, R11, R10, R9, R8;
    uint64_t RDI, RSI, RBP, RDX, RCX, RBX, RAX;

    uint64_t RFLAGS;
    uint64_t RSP;
    uint64_t RIP;
} KrProcessorSnapshot;

#endif // !YCH_KERNEL_CPU_PROCESSOR_SNAPSHOT_H
