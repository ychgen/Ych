#ifndef YCH_KERNEL_KRTL_KRNLMEMORY_H
#define YCH_KERNEL_KRTL_KRNLMEMORY_H

#include <stddef.h>
#include <stdint.h>

// memset and memzero

void* KrtlContiguousSetBuffer(void* pDest, uint8_t uVal, size_t N);
void* KrtlContiguousZeroBuffer(void* pDest, size_t N);

// memcpy and memmove

void* KrtlContiguousCopyBuffer(void* restrict pDest, const void* restrict pSrc, size_t N);
void* KrtlContiguousMoveBuffer(void* pDest, const void* pSrc, size_t N);

#endif // !YCH_KERNEL_KRTL_KRNLMEMORY_H
