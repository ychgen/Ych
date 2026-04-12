#ifndef YCH_KERNEL_CPU_APIC_H
#define YCH_KERNEL_CPU_APIC_H

#include "Core/Fundtypes.h"

#define KR_APIC_BASE_MSR           0x1B
#define KR_APIC_BASE_MSR_ENABLE ( 1 << 3 )

// End of Interupt
#define KR_LOCAL_APIC_REGISTER_EOI 0x0B0

// Redundant, Local APIC is a prerequisite for Long Mode, which Kernelych is 64-bit, so always `true`.
BOOL  KrProcessorSupportsAPIC(void);
void  KrEnableAPIC(void);
QWORD KrGetAPICPhysicalBase(void);

#endif // !YCH_KERNEL_CPU_APIC_H
