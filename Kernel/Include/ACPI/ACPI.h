#ifndef YCH_KERNEL_ACPI_ACPI_H
#define YCH_KERNEL_ACPI_ACPI_H

#define KR_ACPI_RSDP_OEMID_SIZE       6
#define KR_ACPI_RSDP_SIGNATURE_SIZE   8
#define KR_ACPI_RSDP_SIGNATURE       "RSD PTR "
#define KR_ACPI_LEGACY_SECTION_SIZE   20

#define KR_ACPI_SDT_SIGNATURE_SIZE    4
#define KR_ACPI_SDT_OEMID_SIZE        6
#define KR_ACPI_SDT_OEM_TABLE_ID_SIZE 8

#define KR_ACPI_XSDT_SIGNATURE       "XSDT" // eXtended System Description Table
#define KR_ACPI_MADT_SIGNATURE       "APIC" // Multiple APIC Description Table

#define KR_ACPI_MADT_RECORD_TYPE_PROCESSOR_LOCAL_APIC   0 // Processor Local APIC
#define KR_ACPI_MADT_RECORD_TYPE_IO_APIC                1 // I/O APIC
#define KR_ACPI_MADT_RECORD_TYPE_IO_APIC_INT_SRC_OVR    2 // I/O APIC Interrupt Source Override
#define KR_ACPI_MADT_RECORD_TYPE_IO_APIC_NMI_SRC        3 // I/O APIC Nonmaskable Interrupt Source
#define KR_ACPI_MADT_RECORD_TYPE_LOCAL_APIC_NMIS        4 // Local APIC Nonmaskable Interrupts
#define KR_ACPI_MADT_RECORD_TYPE_LOCAL_APIC_ADR_OVR     5 // Local APIC Address Override
#define KR_ACPI_MADT_RECORD_TYPE_PROCESSOR_LOCAL_X2APIC 9 // Processor Local x2APIC

#define KR_ACPI_LAPIC_FLAGS_PROCESSOR_ENABLED (1 << 0) // If this bit is set, this processor can be brought up online.
#define KR_ACPI_LAPIC_FLAGS_ONLINE_CAPABLE    (1 << 1) // Who cares about this loserous bit. I ain't no server OS.

#include "Krnlych.h"

typedef struct KR_PACKED
{
    /* VERSION 1, AKA LEGACY SECTION */
    CHAR  Signature[KR_ACPI_RSDP_SIGNATURE_SIZE];
    BYTE  Checksum;
    CHAR  OEMID[KR_ACPI_RSDP_OEMID_SIZE];
    BYTE  Revision;
    DWORD dwPhysAddrRsdt; // Deprecated

    /* VERSION 2 */
    DWORD dwLength;
    QWORD qwPhysAddrXsdt;
    BYTE  ExtdChecksum;
    BYTE  Reserved[3];
} KrAcpiRsdp;

typedef struct KR_PACKED
{
    CHAR  Signature[KR_ACPI_SDT_SIGNATURE_SIZE];
    DWORD dwLength;
    BYTE  Revision;
    BYTE  Checksum;
    CHAR  OEMID[KR_ACPI_SDT_OEMID_SIZE];
    CHAR  OEMTableID[KR_ACPI_SDT_OEM_TABLE_ID_SIZE];
    DWORD dwOEMRevision;
    DWORD dwCreatorID;
    DWORD dwCreatorRevision;
} KrAcpiSdtHeader;

typedef struct KR_PACKED
{
    KrAcpiSdtHeader AcpiHeader;
    UINT PhysAddrLocalApic;
    DWORD dwFlags;
} KrAcpiMadtHeader;

typedef struct KR_PACKED
{
    BYTE AcpiID;
    BYTE LocalApicID;
    DWORD dwFlags;
} KrAcpiMadtEntryLocalApic;

typedef struct KR_PACKED
{
    UCHAR Reserved[2];
    DWORD LocalApicID;
    DWORD dwFlags;
    DWORD AcpiID;
} KrAcpiMadtEntryLocalx2Apic;

UCHAR KrAcpiCalculateChecksum(const VOID* pAcpiStruct, DWORD dwStructLength);
BOOL  KrAcpiIsChecksumValid(const VOID* pAcpiStruct, DWORD dwStructLength);

KrAcpiSdtHeader* KrAcpiLocateTable(const KrAcpiSdtHeader* pSDT, CSTR strTableSignature);

#endif // !YCH_KERNEL_ACPI_ACPI_H
