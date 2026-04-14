#ifndef YCH_KERNEL_KRTL_KRNLMEMORY_H
#define YCH_KERNEL_KRTL_KRNLMEMORY_H

#include "Krnlych.h"

// memset and memzero

VOID* KrtlContiguousSetBuffer (VOID* pDest, BYTE Value, SIZE N);
VOID* KrtlContiguousZeroBuffer(VOID* pDest, SIZE N);

// memcpy and memmove

VOID* KrtlContiguousCopyBuffer(VOID* restrict pDest, const VOID* restrict pSrc, SIZE N);
VOID* KrtlContiguousMoveBuffer(VOID*          pDest, const VOID*          pSrc, SIZE N);

#endif // !YCH_KERNEL_KRTL_KRNLMEMORY_H
