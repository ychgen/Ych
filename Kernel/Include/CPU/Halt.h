#ifndef YCH_KERNEL_CORE_KR_PROCESSOR_HALT_H
#define YCH_KERNEL_CORE_KR_PROCESSOR_HALT_H

#include "Krnlych.h"

// spin loop optimization and such
#define KrProcessorPause() __asm__ __volatile__ ("pause\n\t")

KR_NORETURN VOID KrProcessorHalt(VOID);

#endif // !YCH_KERNEL_CORE_KRHALT_H
