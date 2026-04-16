#ifndef YCK_KERNEL_INIT_KR_KERNEL_START_H
#define YCK_KERNEL_INIT_KR_KERNEL_START_H

// Contract between KRNL and BOOTLDR.
#include "BootContract/BootContract.h"
#include "Krnlych.h"

/** Entry point of the kernel. The bootloader will jump to this function upon control transfer. */
KR_SECTION(".text.KrKernelStart")
KR_NORETURN VOID KrKernelStart(const KrSystemInfoPack* pSystemInfoPack);

#endif // !YCK_KERNEL_INIT_KR_KERNEL_START_H
