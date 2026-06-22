#include "ACPI/ACPI.h"

#include "Memory/Virtmemmgmt.h"

#include "KRTL/Krnlmem.h"

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

KrAcpiSdtHeader* KrAcpiLocateTable(const KrAcpiSdtHeader* pSDT, CSTR strTableSignature)
{
    UINT EntryCount = (pSDT->dwLength - sizeof(KrAcpiSdtHeader)) / 8;
    QWORD* pAddresses = (QWORD*)(((UINTPTR) pSDT) + sizeof(KrAcpiSdtHeader));

    for (UINT i = 0; i < EntryCount; i++)
    {
        KrAcpiSdtHeader* pEntry = (KrAcpiSdtHeader*) KrPhysToVirt(pAddresses[i]);
        if (KrtlBufferEqual(pEntry->Signature, strTableSignature, KR_ACPI_SDT_SIGNATURE_SIZE))
        {
            return pEntry;
        }
    }

    return NULLPTR;
}
