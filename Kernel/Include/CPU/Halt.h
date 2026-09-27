#ifndef YCH_KERNEL_CORE_KR_PROCESSOR_HALT_H
#define YCH_KERNEL_CORE_KR_PROCESSOR_HALT_H

#include "Krnlych.h"

// spin loop optimization and such
VOID KrProcessorPause(VOID); 

KR_NORETURN VOID KrProcessorHalt(VOID);

#endif // !YCH_KERNEL_CORE_KRHALT_H
