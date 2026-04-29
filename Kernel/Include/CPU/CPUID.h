#ifndef YCH_KERNEL_CPU_CPUID_H
#define YCH_KERNEL_CPU_CPUID_H

#define KrCPUID(Leaf, EAX, EBX, ECX, EDX) __asm__ __volatile__ ("cpuid\n\t" : "=a"(EAX), "=b"(EBX), "=c"(ECX), "=d"(EDX) : "0"(Leaf))

// Highest Function Parameter and Manufacturer ID
#define KR_CPUID_LEAF_HFP_MANUFACID     0x00000000
// Processor Info and Feature Bits
#define KR_CPUID_LEAF_PRC_INF_FEAT_BITS 0x00000001
// Extended Feature Information / Extended Processor Info and Features
#define KR_CPUID_LEAF_EXT_FEAT_INFO     0x80000001

// Brand String
#define KR_CPUID_LEAF_BRAND_STR_1ST     0x80000002
#define KR_CPUID_LEAF_BRAND_STR_2ND     0x80000003
#define KR_CPUID_LEAF_BRAND_STR_3RD     0x80000004

/** ECX */
#define KR_CPUID_FEAT_ECX_X2APIC  (1 << 21)

/** EDX */
#define KR_CPUID_FEAT_EDX_APIC    (1 <<  9)
#define KR_CPUID_FEAT_EDX_NX_BIT  (1 << 20)
#define KR_CPUID_FEAT_EDX_PDPE1GB (1 << 26)

#endif // !YCH_KERNEL_CPU_CPUID_H
