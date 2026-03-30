#ifndef YCH_KERNEL_KERNEL_MEMORY_H
#define YCH_KERNEL_KERNEL_MEMORY_H

#include <stddef.h>

// memset and memzero

void* KrContiguousSetBuffer(void* pDest, int iVal, size_t N);
void* KrContiguousZeroBuffer(void* pDest, size_t N);

// memcpy and memmove

void* KrContiguousCopyBuffer(void* restrict pDest, const void* restrict pSrc, size_t N);

// strlen

size_t KrCountCharacters(const char* pString);

#endif // !YCH_KERNEL_KERNEL_MEMORY_H
