#ifndef YCH_KERNEL_CPU_CPUID_H
#define YCH_KERNEL_CPU_CPUID_H

#include <stdint.h>

/** EDX */
#define KR_CPUID_FEAT_EDX_APIC (1 << 9)

#define KrCPUID(function, EAX, EBX, ECX, EDX) \
    __asm__ __volatile__ ("cpuid\n\t" : "=a"(EAX), "=b"(EBX), "=c"(ECX), "=d"(EDX) : "0"(function));

#endif // !YCH_KERNEL_CPU_CPUID_H
