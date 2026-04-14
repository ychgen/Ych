#ifndef YCH_KERNEL_CPU_MSR_H
#define YCH_KERNEL_CPU_MSR_H

#include "Krnlych.h"

// `RDMSR`
QWORD KrReadModelSpecificRegister(DWORD RegisterID);
// `WRMSR`
VOID  KrWriteModelSpecificRegister(DWORD RegisterID, QWORD qwData);

#endif // !YCH_KERNEL_CPU_MSR_H
