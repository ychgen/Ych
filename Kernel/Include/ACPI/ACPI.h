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
#define KR_ACPI_MCFG_SIGNATURE       "MCFG" // Enhanced Configuration Mechanism
#define KR_ACPI_FADT_SIGNATURE       "FACP" // Fixed ACPI Description Table

#define KR_ACPI_MADT_RECORD_TYPE_PROCESSOR_LOCAL_APIC   0 // Processor Local APIC
#define KR_ACPI_MADT_RECORD_TYPE_IO_APIC                1 // I/O APIC
#define KR_ACPI_MADT_RECORD_TYPE_IO_APIC_INT_SRC_OVR    2 // I/O APIC Interrupt Source Override
#define KR_ACPI_MADT_RECORD_TYPE_IO_APIC_NMI_SRC        3 // I/O APIC Nonmaskable Interrupt Source
#define KR_ACPI_MADT_RECORD_TYPE_LOCAL_APIC_NMIS        4 // Local APIC Nonmaskable Interrupts
#define KR_ACPI_MADT_RECORD_TYPE_LOCAL_APIC_ADR_OVR     5 // Local APIC Address Override
#define KR_ACPI_MADT_RECORD_TYPE_PROCESSOR_LOCAL_X2APIC 9 // Processor Local x2APIC

#define KR_ACPI_LAPIC_FLAGS_PROCESSOR_ENABLED (1 << 0) // If this bit is set, this processor can be brought up online.
#define KR_ACPI_LAPIC_FLAGS_ONLINE_CAPABLE    (1 << 1) // Who cares about this loserous bit. I ain't no server OS.

#define ACPI_FADT_FLAG_PM_TIMER_CNTR_IS_32BITS (1 << 8)

#define ACPI_ADDR_SPACE_MEMORY ((UCHAR) 0)
#define ACPI_ADDR_SPACE_PORT   ((UCHAR) 1)
#define ACPI_ADDR_ACCESS_BYTE  ((UCHAR) 1)
#define ACPI_ADDR_ACCESS_WORD  ((UCHAR) 2)
#define ACPI_ADDR_ACCESS_DWORD ((UCHAR) 3)
#define ACPI_ADDR_ACCESS_QWORD ((UCHAR) 4)

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
    UCHAR Reserved[8];
    // followed by AcpiMcfgEntry[]
} AcpiMcfgHeader;

typedef struct KR_PACKED
{
    QWORD PaddrBase;
     WORD PciSegmentGroupNumber;
     BYTE StartBusNo;
     BYTE EndBusNo;
    UCHAR Reserved[4];
} AcpiMcfgEntry;

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

typedef struct KR_PACKED
{
    BYTE AddrSpace;
    BYTE BitWidth;
    BYTE BitOffset;
    BYTE AccessSize;
    QWORD Address;
} AcpiAddrStruct;

typedef struct KR_PACKED
{
    KrAcpiSdtHeader AcpiHeader;
    DWORD FirmwareCtrl;
    DWORD Dsdt;
    BYTE Reserved0;
    BYTE PreferredPowermgmtProfile;
    WORD SCI;
    DWORD SMI_Cmdport;
    BYTE AcpiEnable;
    BYTE AcpiDisable;
    BYTE S4BIOS_REQ;
    BYTE PState_Ctrl;
    DWORD PM1aEventBlock;
    DWORD PM1bEventBlock;
    DWORD PM1aControlBlock;
    DWORD PM1bControlBlock;
    DWORD PM2ControlBlock;
    DWORD PMTimerBlock;
    DWORD GPE0Block;
    DWORD GPE1Block;
    BYTE PM1EventLength;
    BYTE PM1ControlLength;
    BYTE PM2ControlLength;
    BYTE PMTimerLength;
    BYTE GPE0Length;
    BYTE GPE1Length;
    BYTE GPE1Base;
    BYTE CStateControl;
    WORD WorstC2Latency;
    WORD WorstC3Latency;
    WORD FlushSize;
    WORD FlushStride;
    BYTE DutyOffset;
    BYTE DutyWidth;
    BYTE DayAlarm;
    BYTE MonthAlarm;
    BYTE Century;
    WORD BootArchFlags;
    BYTE Reserved1;
    DWORD Flags;
    AcpiAddrStruct ResetReg;
    BYTE ResetValue;
    BYTE Reserved2[3];
    QWORD X_FwCtrl;
    QWORD X_Dsdt;

    AcpiAddrStruct X_PM1aEventBlock;
    AcpiAddrStruct X_PM1bEventBlock;
    AcpiAddrStruct X_PM1aControlBlock;
    AcpiAddrStruct X_PM1bControlBlock;
    AcpiAddrStruct X_PM2ControlBlock;
    AcpiAddrStruct X_PMTimerBlock;
    AcpiAddrStruct X_GPE0Block;
    AcpiAddrStruct X_GPE1Block;
} AcpiFadtHeader;

UCHAR KrAcpiCalculateChecksum(const VOID* pAcpiStruct, DWORD dwStructLength);
BOOL  KrAcpiIsChecksumValid(const VOID* pAcpiStruct, DWORD dwStructLength);

KrAcpiSdtHeader* KrAcpiLocateTable(const KrAcpiSdtHeader* pSDT, CSTR strTableSignature);

#endif // !YCH_KERNEL_ACPI_ACPI_H
