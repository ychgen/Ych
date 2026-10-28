#ifndef YCH_KERNEL_INTMGMT_SPURIOUS_H
#define YCH_KERNEL_INTMGMT_SPURIOUS_H

#include "CPU/Interrupt.h"

#define KERNEL_SPURIOUS_INTERRUPT_VECTOR 0xFF

VOID IntmgmtSpuriousInterrupt(const KrInterruptFrame* pIntfr);

#endif // !YCH_KERNEL_INTMGMT_SPURIOUS_H
