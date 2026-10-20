#ifndef YCH_KERNEL_INTMGMT_TIMER_H
#define YCH_KERNEL_INTMGMT_TIMER_H

#include "CPU/Interrupt.h"

#define KERNEL_TIMER_INTERRUPT_VECTOR 0x20

VOID TimerInterrupt(const KrInterruptFrame* pIntfr);

#endif // !YCH_KERNEL_INTMGMT_TIMER_H
