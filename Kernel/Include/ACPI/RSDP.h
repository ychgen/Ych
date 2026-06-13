#ifndef YCH_KERNEL_ACPI_RSDP_H
#define YCH_KERNEL_ACPI_RSDP_H

#define KR_ACPI_RSDP_OEMID_SIZE     6
#define KR_ACPI_RSDP_SIGNATURE_SIZE 8
#define KR_ACPI_RSDP_SIGNATURE      "RSD PTR "

#include "Krnlych.h"

typedef struct KR_PACKED
{
    CHAR  Signature[KR_ACPI_RSDP_SIGNATURE_SIZE];
    BYTE  Checksum;
    CHAR  OEMID[KR_ACPI_RSDP_OEMID_SIZE];
    BYTE  Revision;
    DWORD dwPhysAddrRsdt; // Deprecated

    DWORD Length;
    QWORD PhysAddrXsdt;
    BYTE  ExtdChecksum;
    BYTE  Reserved[3];
} KrAcpiRsdp;

#endif // !YCH_KERNEL_ACPI_H