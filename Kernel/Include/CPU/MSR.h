#ifndef YCH_KERNEL_CPU_MSR_H
#define YCH_KERNEL_CPU_MSR_H

#include "Core/Fundtypes.h"

// `RDMSR`
QWORD KrReadModelSpecificRegister(DWORD idRegister);
// `WRMSR`
void  KrWriteModelSpecificRegister(DWORD idRegister, DWORD data);

#endif // !YCH_KERNEL_CPU_MSR_H
