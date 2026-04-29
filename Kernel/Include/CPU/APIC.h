#ifndef YCH_KERNEL_CPU_APIC_H
#define YCH_KERNEL_CPU_APIC_H

#include "Krnlych.h"

/**
 * @brief Checks whether or not the processor is capable of x2APIC.
 * 
 * @return TRUE if capable, FALSE otherwise.
 */
BOOL Krx2CheckSupport(VOID);

/**
 * @brief Enables APIC (x2APIC).
 * 
 * @return TRUE if enabled successfully, false if the processor is incapable of x2APIC.
 */
BOOL Krx2Enable(VOID);

/**
 * @brief Sends an EOI signal to the APIC.
 */
VOID Krx2SignalEndOfInterrupt(VOID);

#endif // !YCH_KERNEL_CPU_APIC_H
