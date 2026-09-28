#ifndef YCH_MEMORY_KRNLHEAP_H
#define YCH_MEMORY_KRNLHEAP_H

#include "Krnlych.h"

#define HEAP_DEFAULT_ALIGNMENT 0x10

BOOL  HeapInit(VOID);
VOID* HeapAlignedAcquire(SIZE N, UINT Alignment);
VOID* HeapAcquire(SIZE N);
BOOL  HeapRelinquish(VOID* Ptr);

#endif // !YCH_MEMORY_KRNLHEAP_H
