#ifndef YCH_KERNEL_CPU_APIC_H
#define YCH_KERNEL_CPU_APIC_H

#include "Krnlych.h"

// End of Interupt
#define KR_LAPIC_REGISTER_EOI 0x0B0

// Redundant, Local APIC is a prerequisite for Long Mode, which Kernelych is 64-bit, so always `true`.
BOOL  KrProcessorSupportsAPIC(VOID);
VOID  KrEnableAPIC(VOID);
QWORD KrGetAPICPhysicalBase(VOID);

#endif // !YCH_KERNEL_CPU_APIC_H
