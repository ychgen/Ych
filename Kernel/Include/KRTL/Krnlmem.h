#ifndef YCH_KERNEL_KRTL_KRNLMEMORY_H
#define YCH_KERNEL_KRTL_KRNLMEMORY_H

#include "Core/Fundtypes.h"

// memset and memzero

void* KrtlContiguousSetBuffer(void* pDest, BYTE uVal, USIZE N);
void* KrtlContiguousZeroBuffer(void* pDest, USIZE N);

// memcpy and memmove

void* KrtlContiguousCopyBuffer(void* restrict pDest, const void* restrict pSrc, USIZE N);
void* KrtlContiguousMoveBuffer(void* pDest, const void* pSrc, USIZE N);

#endif // !YCH_KERNEL_KRTL_KRNLMEMORY_H
