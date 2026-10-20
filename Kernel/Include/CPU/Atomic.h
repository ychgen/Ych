#ifndef YCH_KERNEL_CPU_ATOMIC_H
#define YCH_KERNEL_CPU_ATOMIC_H

#include "Krnlych.h"

BYTE  AtomicExchangeOr8(BYTE* pByte, BYTE OrValue);
DWORD AtomicExchangeAdd32(DWORD* pDoubleword, DWORD ValueToAdd);

VOID  AtomicOr8(BYTE* pByte, BYTE OrValue);

#endif // !YCH_KERNEL_CPU_ATOMIC_H
