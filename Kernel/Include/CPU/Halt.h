#ifndef YCH_KERNEL_CORE_KR_PROCESSOR_HALT_H
#define YCH_KERNEL_CORE_KR_PROCESSOR_HALT_H

#include "Krnlych.h"

// spin loop optimization and such
#define KrProcessorPause() __asm__ __volatile__ ("pause\n\t")
// Halt CPU for now, not indefinitely. IF remains untouched, so if set interrupts can awaken the processor
#define ProcessorHalt() __asm__ __volatile__ ("hlt\n\t")

KR_NORETURN VOID ProcessorIndefiniteHalt(VOID);

#endif // !YCH_KERNEL_CORE_KRHALT_H
