#include "ACPI/ACPI.h"

UCHAR KrAcpiCalculateChecksum(const VOID* pAcpiStruct, DWORD dwStructLength)
{
    UCHAR Sum = 0;
    while (dwStructLength--)
    {
        Sum += *(((UCHAR*) pAcpiStruct) + dwStructLength);
    }
    return Sum;
}

BOOL KrAcpiIsChecksumValid(const VOID* pAcpiStruct, DWORD dwStructLength)
{
    return KrAcpiCalculateChecksum(pAcpiStruct, dwStructLength) == 0;
}

