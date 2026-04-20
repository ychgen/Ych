#ifndef YCH_KERNEL_CPU_IDENTIFY_H
#define YCH_KERNEL_CPU_IDENTIFY_H

#include "Krnlych.h"

#define KR_PROCESSOR_MANUFACTURER_ID_SIZE 12
#define KR_PROCESSOR_BRAND_STRING_SIZE    48

VOID KrGetProcessorManufacturerID(CHAR* pOut);

/**
 * @brief Acquires the `Processor Brand String` of the CPU by using relevant CPUID functions.
 * 
 * @param pOut Output destination buffer to write the brand string to. Its size must be at least KR_PROCESSOR_BRAND_STRING_SIZE.
 */
VOID KrGetProcessorBrandString(CHAR* pOut);

#endif // !YCH_KERNEL_CPU_IDENTIFY_H
