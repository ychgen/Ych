#ifndef YCH_KERNEL_CPU_IDENTIFY_H
#define YCH_KERNEL_CPU_IDENTIFY_H

#include "Krnlych.h"

#define KR_PROCESSOR_MANUFACTURER_ID_SIZE 12
#define KR_PROCESSOR_BRAND_STRING_SIZE    48

/**
 * @brief Acquires the `Processor Manufacturer ID` of the CPU by using the relevant CPUID function.
 * This function only writes KR_PROCESSOR_MANUFACTURER_ID_SIZE characters, caller must ensure null terminator themselves if needed.
 * 
 * @param pOut Output destination buffer to write the manufacturer ID string to. Its size must be at least KR_PROCESSOR_MANUFACTURER_ID_SIZE.
 */
VOID KrGetProcessorManufacturerID(CHAR* pOut);

/**
 * @brief Acquires the `Processor Brand String` of the CPU by using relevant CPUID functions.
 * This function only writes KR_PROCESSOR_BRAND_STRING_SIZE characters, caller must ensure null terminator themselves if needed.
 * 
 * @param pOut Output destination buffer to write the brand string to. Its size must be at least KR_PROCESSOR_BRAND_STRING_SIZE.
 */
VOID KrGetProcessorBrandString(CHAR* pOut);

#endif // !YCH_KERNEL_CPU_IDENTIFY_H
