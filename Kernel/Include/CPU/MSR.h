#ifndef YCH_KERNEL_CPU_MSR_H
#define YCH_KERNEL_CPU_MSR_H

#include "Krnlych.h"

// `RDMSR` but nicer.
QWORD KrReadModelSpecificRegister(DWORD RegisterID);
// `WRMSR` but nicer.
VOID  KrWriteModelSpecificRegister(DWORD RegisterID, QWORD qwData);

/** IA32_APIC_BASE */
#define KR_MSR_IA32_APIC_BASE         0x1B
#define KR_MSR_IA32_APIC_BASE_BSP  (1 <<  8)
#define KR_MSR_IA32_APIC_BASE_EXTD (1 << 10)
#define KR_MSR_IA32_APIC_BASE_EN   (1 << 11)

/** IA32_EFER */
#define KR_MSR_IA32_EFER           0xC0000080
#define KR_MSR_IA32_EFER_SCE       (1 <<  0)
#define KR_MSR_IA32_EFER_LME       (1 <<  8)
#define KR_MSR_IA32_EFER_LMA       (1 << 10)
#define KR_MSR_IA32_EFER_NXE       (1 << 11)
#define KR_MSR_IA32_EFER_SVME      (1 << 12)
#define KR_MSR_IA32_EFER_LMSLE     (1 << 13)
#define KR_MSR_IA32_EFER_FFXSR     (1 << 14)
#define KR_MSR_IA32_EFER_TCE       (1 << 15)

#endif // !YCH_KERNEL_CPU_MSR_H
