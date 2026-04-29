#ifndef YCH_KERNEL_CPU_CR_H
#define YCH_KERNEL_CPU_CR_H

#include "Krnlych.h"

#define KR_CR0_PE         (1 <<  0) // Protection Enable
#define KR_CR0_MP         (1 <<  1) // Monitor Coprocessor
#define KR_CR0_EM         (1 <<  2) // Emulation
#define KR_CR0_TS         (1 <<  3) // Task Switched
#define KR_CR0_ET         (1 <<  4) // Extension Type
#define KR_CR0_NE         (1 <<  5) // Numeric Error
#define KR_CR0_WP         (1 << 16) // Write Protect
#define KR_CR0_AM         (1 << 18) // Alignment Mask
#define KR_CR0_NW         (1 << 29) // Not Write-through
#define KR_CR0_CD         (1 << 30) // Cache Disable
#define KR_CR0_PG         (1 << 31) // Paging

#define KR_CR3_PWT        (1 <<  3) // Page-level Write-Through
#define KR_CR3_PCD        (1 <<  4) // Page-level Cache Disable

#define KR_CR4_VME        (1 <<  0) // Virtual-8086 Mode Extensions
#define KR_CR4_PVI        (1 <<  1) // Protected-Mode Virtual Interrupts
#define KR_CR4_TSD        (1 <<  2) // Time Stamp Disable
#define KR_CR4_DE         (1 <<  3) // Debugging Extensions
#define KR_CR4_PSE        (1 <<  4) // Page Size Extensions
#define KR_CR4_PAE        (1 <<  5) // Physical Address Extension
#define KR_CR4_MCE        (1 <<  6) // Machine-Check Enable
#define KR_CR4_PGE        (1 <<  7) // Page-Global Enable
#define KR_CR4_PCE        (1 <<  8) // Performance-Monitoring Counter Enable
#define KR_CR4_OSFXSR     (1 <<  9) // Operating System Support For FXSAVE and FXRSTOR instructions
#define KR_CR4_OSXMMEXCPT (1 << 10) // Operating System Support For Unmasked SIMD Floating-Point Exceptions
#define KR_CR4_UMIP       (1 << 11) // User-Mode Instruction Prevention
#define KR_CR4_LA57       (1 << 12) // 57-bit linear addresses
#define KR_CR4_VMXE       (1 << 13) // VMX-Enable Bit
#define KR_CR4_SMXE       (1 << 14) // SMX-Enable Bit
#define KR_CR4_FSGSBASE   (1 << 16) // FSGSBASE-Enable Bit
#define KR_CR4_PCIDE      (1 << 17) // PCID-Enable Bit
#define KR_CR4_OSXSAVE    (1 << 18) // XSAVE and Processor Extended States-Enable Bit
#define KR_CR4_KL         (1 << 19) // Key-Locker-Enable Bit
#define KR_CR4_SMEP       (1 << 20) // SMEP-Enable Bit
#define KR_CR4_SMAP       (1 << 21) // SMAP-Enable Bit
#define KR_CR4_PKE        (1 << 22) // Enable protection keys for user-mode pages
#define KR_CR4_CET        (1 << 23) // Control-flow Enforcement Technology
#define KR_CR4_PKS        (1 << 24) // Enable protection keys for supervisor-mode pages
#define KR_CR4_UINTR      (1 << 25) // User Interrupts Enable Bit

#define __KR_READ_CRX(CRX, OutCRX) { \
    (VOID)(&OutCRX); \
    KR_STATIC_ASSERT(sizeof(OutCRX) == 8, "OutCRX must be a QWORD."); \
    __asm__ __volatile__ ("mov %%cr" KR_STRINGIZE(CRX) ", %0\n\t" : "=r"(OutCRX)); \
}
#define __KR_WRITE_CRX(CRX, InCRX) { \
    (VOID)(&InCRX); \
    KR_STATIC_ASSERT(sizeof(InCRX) == 8, "InCRX must be a QWORD."); \
    __asm__ __volatile__ ("mov %0, %%cr" KR_STRINGIZE(CRX) "\n\t" : : "r"(InCRX)); \
}

#define KrReadCR0(OutCR0) __KR_READ_CRX(0, OutCR0)
#define KrReadCR2(OutCR2) __KR_READ_CRX(2, OutCR2)
#define KrReadCR3(OutCR3) __KR_READ_CRX(3, OutCR3)
#define KrReadCR4(OutCR4) __KR_READ_CRX(4, OutCR4)

#define KrWriteCR0(InCR0) __KR_WRITE_CRX(0, InCR0)
#define KrWriteCR3(InCR3) __KR_WRITE_CRX(3, InCR3)
#define KrWriteCR4(InCR4) __KR_WRITE_CRX(4, InCR4)

#endif // !YCH_KERNEL_CPU_CR_H
