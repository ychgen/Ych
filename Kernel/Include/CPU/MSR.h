#ifndef YCH_KERNEL_CPU_MSR_H
#define YCH_KERNEL_CPU_MSR_H

#include <stdint.h>

// `RDMSR`
uint64_t KrReadModelSpecificRegister(uint32_t idRegister);
// `WRMSR`
void     KrWriteModelSpecificRegister(uint32_t idRegister, uint64_t data);

#endif // !YCH_KERNEL_CPU_MSR_H
