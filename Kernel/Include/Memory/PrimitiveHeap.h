#ifndef YCH_KERNEL_MEMORY_PRIMITIVE_HEAP_H
#define YCH_KERNEL_MEMORY_PRIMITIVE_HEAP_H

#include "Krnlych.h"

#define KR_PRIMITIVE_HEAP_STEPPING 128 // Stepping bytes. Do not change without changing how regions work in PrimitiveHeap.c

BOOL  KrPrimitiveInit(VOID);
VOID* KrPrimitiveAcquire(SIZE szAcquisition);
BOOL  KrPrimitiveRelinquish(VOID* pAddr);

#endif // !YCH_KERNEL_MEMORY_PRIMITIVE_HEAP_H
