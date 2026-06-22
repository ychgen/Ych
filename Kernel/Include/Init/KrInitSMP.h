#ifndef YCH_KERNEL_INIT_KR_INIT_SMP_H
#define YCH_KERNEL_INIT_KR_INIT_SMP_H

#include "Krnlych.h"

// Innocent-looking function that potentially unleashes 32 instruction-lusting processors that treat memory and cache lines as a social constructs.
VOID KrInitSMP(VOID);

#endif // !YCH_KERNEL_INIT_KR_INIT_SMP_H
