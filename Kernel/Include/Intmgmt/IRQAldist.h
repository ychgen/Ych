// IRQ Allocation & Distribution
#ifndef YCH_KERNEL_INTMGMT_IRQ_ALDIST_H
#define YCH_KERNEL_INTMGMT_IRQ_ALDIST_H

#include "Krnlych.h"

typedef struct
{
    DWORD CpuID;
    UCHAR IntVector;
} PairOfProcessorIntVector;

PairOfProcessorIntVector AldistOfferSetupForIRQ(DWORD IRQ);

#endif // !YCH_KERNEL_INTMGMT_IRQ_ALDIST_H
