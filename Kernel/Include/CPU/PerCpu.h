#ifndef YCH_KERNEL_CPU_PER_CPU_H
#define YCH_KERNEL_CPU_PER_CPU_H

#include "Krnlych.h"

typedef struct KrPerCpu
{
    struct KrPerCpu* pSelf; // Pointer to self structure
    DWORD ID; // Processor Local APIC ID
} KrPerCpu;

VOID KrGrabThisCpuStruct(VOID);

KrPerCpu* KrThisCpu(VOID);
DWORD KrThisCpuID(VOID);

#endif // !YCH_KERNEL_CPU_PER_CPU_H
