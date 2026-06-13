#ifndef YCH_KERNEL_ACPI_ACPI_H
#define YCH_KERNEL_ACPI_ACPI_H

#include "Krnlych.h"

UCHAR KrAcpiCalculateChecksum(const VOID* pAcpiStruct, DWORD dwStructLength);\
BOOL  KrAcpiIsChecksumValid(const VOID* pAcpiStruct, DWORD dwStructLength);

#endif // !YCH_KERNEL_ACPI_ACPI_H
