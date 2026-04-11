#ifndef YCK_KERNEL_INIT_KR_KERNEL_START_H
#define YCK_KERNEL_INIT_KR_KERNEL_START_H

// Contract between KRNL and BOOTLDR.
#include "BootContract/BootContract.h"

__attribute__((section(".text.KrKernelStart"), noreturn))
/** Entry point of the kernel. The bootloader will jump to this function upon control transfer. */
void KrKernelStart(const KrSystemInfoPack* pSystemInfoPack);

#endif // !YCK_KERNEL_INIT_KR_KERNEL_START_H
