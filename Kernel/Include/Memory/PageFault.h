#ifndef YCH_KERNEL_MEMORY_PAGE_FAULT_H
#define YCH_KERNEL_MEMORY_PAGE_FAULT_H

#include "CPU/Interrupt.h"

/**
 * @brief The global kernel page fault handler. It handles a lot of cases.
 * This is where demand-paging logic lives, also the logic to meltdown kernel if a supervisor GUARD page is touched.
 * 
 * @param pInterruptFrame The interrupt frame from the interrupt dispatcher.
 */
VOID KrGlobalPageFaultHandler(const KrInterruptFrame* pInterruptFrame);

#endif // !YCH_KERNEL_MEMORY_PAGE_FAULT_H
