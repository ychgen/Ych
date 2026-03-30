#ifndef YCH_KERNEL_INIT_KR_INIT_IDT_H
#define YCH_KERNEL_INIT_KR_INIT_IDT_H

#include "CPU/IDT.h"

extern __attribute__((aligned(16))) KrInterruptDescriptor g_krInterruptDescriptorTable[KR_NUMBER_OF_INTERRUPT_DESCRIPTOR_ENTRIES];

void KrInitIDT(void);

#endif // !YCH_KERNEL_INIT_KR_INIT_IDT_H
