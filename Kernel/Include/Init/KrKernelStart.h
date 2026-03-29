#ifndef YCK_KERNEL_INIT_KR_KERNEL_START_H
#define YCK_KERNEL_INIT_KR_KERNEL_START_H

#include "PactOfInit.h" // Contract between KRNL and BOOTLDR.

extern KrSystemInfoPack* g_pSystemInfoPack;

__attribute__((section(".text.KrKernelStart"), noreturn))
/** Entry point of the kernel. The bootloader will jump to this function upon control transfer. */
void KrKernelStart(const KrSystemInfoPack* pSystemInfoPack);

#endif // !YCK_KERNEL_INIT_KR_KERNEL_START_H
