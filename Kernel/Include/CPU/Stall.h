/**
 * Primitive CPU timing-stalling. Not precise at all. Useful when you need something that works "well enough".
 * Really, not recommended usually. This is only useful if you need a "relatively good enough" buffer between like attempting something.
 */

#ifndef YCH_KERNEL_CPU_STALL_H
#define YCH_KERNEL_CPU_STALL_H

#include "Krnlych.h"

VOID KrProcessorStall(ULONG ClockCycles);

#endif // !YCH_KERNEL_CPU_STALL_H
