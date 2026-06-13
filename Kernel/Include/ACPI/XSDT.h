#ifndef YCH_KERNEL_ACPI_XSDT_H
#define YCH_KERNEL_ACPI_XSDT_H

#define KR_ACPI_XSDT_SIGNATURE_SIZE    4
#define KR_ACPI_XSDT_SIGNATURE        "XSDT"
#define KR_ACPI_XSDT_OEMID_SIZE        6
#define KR_ACPI_XSDT_OEM_TABLE_ID_SIZE 8

#include "Krnlych.h"

typedef struct KR_PACKED
{
    CHAR  Signature[KR_ACPI_XSDT_SIGNATURE_SIZE];
    DWORD Length;
    BYTE  Revision;
    BYTE  Checksum;
    CHAR  OEMID[KR_ACPI_XSDT_OEMID_SIZE];
    CHAR  OEMTableID[KR_ACPI_XSDT_OEM_TABLE_ID_SIZE];
    DWORD OEMRevision;
    DWORD CreatorID;
    DWORD CreatorRevision;
} KrAcpiXsdt;

#endif // !YCH_KERNEL_ACPI_XSDT_H
