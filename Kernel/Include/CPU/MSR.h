#ifndef YCH_KERNEL_CPU_MSR_H
#define YCH_KERNEL_CPU_MSR_H

#include "Krnlych.h"

// `RDMSR` but nicer.
QWORD KrReadMSR(DWORD RegisterID);
// `WRMSR` but nicer.
VOID  KrWriteMSR(DWORD RegisterID, QWORD qwData);

/** IA32_APIC_BASE */
#define IA32_APIC_BASE      0x1B
#define IA32_APIC_BASE_BSP  (1 <<  8)
#define IA32_APIC_BASE_EXTD (1 << 10)
#define IA32_APIC_BASE_EN   (1 << 11)
#define IA32_APIC_BASE_ADDR_MASK 0x0000FFFFFFFFF000ULL

/** IA32_X2APIC_APICID */
#define KR_MSR_IA32_X2APIC_APICID  0x802

/** IA32_X2APIC_EOI */
#define KR_MSR_IA32_X2APIC_EOI     0x80B

/** IA32_X2APIC_ICR */
#define KR_MSR_IA32_X2APIC_ICR     0x830

/** IA32_PAT_MSR */
#define KR_PAT_IA32_PAT_MSR        0x277

/** IA32_EFER */
#define KR_MSR_IA32_EFER           0xC0000080
#define KR_MSR_IA32_EFER_SCE       (1 <<  0) // SYSCALL Enable
#define KR_MSR_IA32_EFER_LME       (1 <<  8) // IA-32e Mode Enable
#define KR_MSR_IA32_EFER_LMA       (1 << 10) // IA-32e Mode Active
#define KR_MSR_IA32_EFER_NXE       (1 << 11) // Execute Disable Bit Enable

#define IA32_GS_BASE        0xC0000101 // Holds active GS base
#define IA32_KERNEL_GS_BASE 0xC0000102 // Holds inactive GS base

#endif // !YCH_KERNEL_CPU_MSR_H
