#include "Init/KrInitACPI.h"

#include "Core/Krnlmeltdown.h"
#include "Core/KernelState.h"
#include "Core/Assert.h"

#include "CPU/PerCpu.h"
#include "CPU/Halt.h"
#include "CPU/MSR.h"

#include "Memory/Virtmemmgmt.h"
#include "Memory/MMIO.h"

#include "KRTL/Krnlmem.h"

#include "ACPI/ACPI.h"

#include "Intmgmt/IOAPIC.h"

static VOID ProcessMADT(KrAcpiMadtHeader* pMadt);
static VOID ProcessMCFG(AcpiMcfgHeader* pMcfg);

static KR_NORETURN VOID FailInit(MDCODE mdCode, CSTR strMdDesc);

VOID KrInitACPI(UINTPTR PaddrRsdp)
{
    if (!PaddrRsdp) FailInit(KR_MDCODE_CORRUPT_ACPI_RSDP, "ACPI RSDP pointer is NULL!");

    // Get RSDP
    KrAcpiRsdp* pRsdp = g_KernelState.AcpiInfo.pRsdp = (KrAcpiRsdp*) KrPhysToVirt(PaddrRsdp);
    
    if (!KrtlBufferEqual(pRsdp->Signature, KR_ACPI_RSDP_SIGNATURE, KR_ACPI_RSDP_SIGNATURE_SIZE)) FailInit(KR_MDCODE_CORRUPT_ACPI_RSDP, "Invalid ACPI RSDP pointer provided by firmware.");
    if (pRsdp->Revision < 2 || pRsdp->dwLength < sizeof(KrAcpiRsdp)) FailInit(KR_MDCODE_CORRUPT_ACPI_RSDP, "Bootloader provided an invalid ACPI RSDP or the one the bootloader picked is corrupt thanks to the firmware side (we is innocent). Shoulduva been at least revision 2 and matched minimum size for said revision.");

    if (!KrAcpiIsChecksumValid(pRsdp, KR_ACPI_LEGACY_SECTION_SIZE))FailInit(KR_MDCODE_CORRUPT_ACPI_RSDP, "ACPI RSDP legacy section checksum failure.");
    if (!KrAcpiIsChecksumValid(pRsdp, pRsdp->dwLength)) FailInit(KR_MDCODE_CORRUPT_ACPI_RSDP, "ACPI RSDP whole section checksum failure.");
    if (!pRsdp->qwPhysAddrXsdt) FailInit(KR_MDCODE_CORRUPT_ACPI_RSDP, "ACPI RSDP pointer to the XSDT is NULL!");

    KrAcpiSdtHeader* pXsdt = g_KernelState.AcpiInfo.pXsdt = (KrAcpiSdtHeader*) KrPhysToVirt(pRsdp->qwPhysAddrXsdt);
    if (!KrtlBufferEqual(pXsdt->Signature, KR_ACPI_XSDT_SIGNATURE, KR_ACPI_SDT_SIGNATURE_SIZE)) FailInit(KR_MDCODE_CORRUPT_ACPI_RSDP, "ACPI RSDP pointer to the XSDT is invalid as pointed address contains invalid signature.");
    if (!KrAcpiIsChecksumValid(pXsdt, pXsdt->dwLength)) FailInit(KR_MDCODE_ACPI_SDT_CHECKSUM_NV, "ACPI XSDT structure has invalid checksum.");

    KrAcpiMadtHeader* pMadt = (KrAcpiMadtHeader*) KrAcpiLocateTable(pXsdt, KR_ACPI_MADT_SIGNATURE);
    if (!pMadt) FailInit(KR_MDCODE_ACPI_TABLE_NOT_FOUND, "System has no MADT (Multiple APIC DT) ACPI table present present (or exists in a malformed form)! Issue on firmware side.");
    if (pMadt->AcpiHeader.dwLength < sizeof(KrAcpiMadtHeader)) FailInit(KR_MDCODE_ACPI_TABLE_CORRUPT, "ACPI MADT table has its dwField lower than the size of the KrAcpiMadtHeader struct.");
    
    // Multiple APIC Descriptor Table
    ProcessMADT(pMadt);

    if (!g_KernelState.SmpInfo.IdealProcessorCount)
    {
        FailInit(KR_MDCODE_ACPI_TABLE_CORRUPT, "ACPI MADT table described 0 logical processors. Such occurrence is impossible as you are observing this error.");
    }

    AcpiMcfgHeader* pMcfg = (AcpiMcfgHeader*) KrAcpiLocateTable(pXsdt, KR_ACPI_MCFG_SIGNATURE);
    if (!pMcfg) FailInit(KR_MDCODE_ACPI_TABLE_NOT_FOUND, "System has no MCFG (Enhanced Configuration Mechanism) ACPI table present present (or exists in a malformed form)! Issue on firmware side");
    if (pMcfg->AcpiHeader.dwLength < sizeof(AcpiMcfgHeader) + sizeof(AcpiMcfgEntry)) FailInit(KR_MDCODE_ACPI_TABLE_CORRUPT, "ACPI MCFG table has its dwField lower than the size of the AcpiMcfgHeader struct.");
    
    // PCI Enhanced Configuration Mechanism
    ProcessMCFG(pMcfg);

    AcpiFadtHeader* pFadt = (AcpiFadtHeader*) KrAcpiLocateTable(pXsdt, KR_ACPI_FADT_SIGNATURE);
    RUNTIME_ASSERT(pFadt != NULLPTR, KR_MDCODE_ACPI_TABLE_NOT_FOUND, "Firmware has provided no FADT table.\n");
    //RUNTIME_ASSERT(pFadt->AcpiHeader.dwLength == sizeof(AcpiFadtHeader), KR_MDCODE_ACPI_TABLE_CORRUPT, "FADT table invalid dwLength.");
    RUNTIME_ASSERT(KrAcpiIsChecksumValid(pFadt, pFadt->AcpiHeader.dwLength), KR_MDCODE_ACPI_SDT_CHECKSUM_NV, "FADT table checksum is invalid.");

    g_KernelState.AcpiInfo.pFadt = pFadt;
}

static VOID ProcessMADT(KrAcpiMadtHeader* pMadt)
{
    const BYTE* pRecordsCur = (const BYTE*)(((UINTPTR) pMadt) + sizeof(KrAcpiMadtHeader));
    const BYTE* pRecordsEnd = (const BYTE*)(((UINTPTR) pMadt) + pMadt->AcpiHeader.dwLength);

    while (pRecordsCur < pRecordsEnd)
    {
        UCHAR eType = *pRecordsCur++;
        UCHAR RecordSize = *pRecordsCur++;

        switch (eType)
        {
        case KR_ACPI_MADT_RECORD_TYPE_PROCESSOR_LOCAL_APIC:
        {
            const KrAcpiMadtEntryLocalApic* pLocalApic = (const KrAcpiMadtEntryLocalApic*) pRecordsCur;
            if (!(pLocalApic->dwFlags & KR_ACPI_LAPIC_FLAGS_PROCESSOR_ENABLED))
            {
                break;
            }

            g_KernelState.SmpInfo.IdealProcessorCount++;
            g_KernelState.SmpInfo.ProcInfo[pLocalApic->LocalApicID].E_V = 1;
            g_KernelState.SmpInfo.ProcInfo[pLocalApic->LocalApicID].BSP = 0;
            g_KernelState.SmpInfo.ProcInfo[pLocalApic->LocalApicID].AcpiID = pLocalApic->AcpiID;
            g_KernelState.SmpInfo.ProcInfo[pLocalApic->LocalApicID].LocalApicID = pLocalApic->LocalApicID;

            break;
        }
        case KR_ACPI_MADT_RECORD_TYPE_IO_APIC:
        {
            CONST AcpiMadtEntryIoApic* CONST FwIoApicData = (CONST AcpiMadtEntryIoApic* CONST) pRecordsCur;

            IoApicInfo* IoApicInfo = GetIoApics() + FwIoApicData->ID;
            IoApicInfo->E_V = TRUE;
            IoApicInfo->ID = FwIoApicData->ID;
            IoApicInfo->FirstIRQ = FwIoApicData->FirstIRQ;
            IoApicInfo->PaddrBase = FwIoApicData->BaseAddress;

            GetIoApicIDs()[GetIoApicCount()] = IoApicInfo->ID;
            _SetIoApicCount(GetIoApicCount() + 1);

            break;
        }
        default:
        {
            break;
        }
        }

        // Minus two because Record Length is inclusive of the little table it makes with Entry Type.
        // We consumed two bytes originally with the pointer
        pRecordsCur += RecordSize - 2;
    }

    g_KernelState.SmpInfo.ActiveProcessorCount = 1; // BSP is active
    g_KernelState.SmpInfo.ProcInfo[GetThisCpuID()].BSP = 1; // Mark BSP flag
    g_KernelState.SmpInfo.ProcInfo[GetThisCpuID()].RFD = 1; // Mark reported for duty because well it's running as of now
}

static VOID ProcessMCFG(AcpiMcfgHeader* pMcfg)
{
    AcpiMcfgEntry* pEntry = (AcpiMcfgEntry*)(((UINTPTR) pMcfg) + sizeof(AcpiMcfgHeader));

    SIZE MapSize = (pEntry->EndBusNo - pEntry->StartBusNo + 1) * 0x100000;
    g_KernelState.MmioInfo.VaddrPciEcamBase = MmioMapDevice("PCIe Enhanced Configuration Access Mechanism", pEntry->PaddrBase, MapSize, 0);
    if (!g_KernelState.MmioInfo.VaddrPciEcamBase)
    {
        FailInit(KR_MDCODE_MMIO_DEVICE_MAP_FAILURE, "Failed to map ECAM into the virtual address space.");
    }
}

static KR_NORETURN VOID FailInit(MDCODE mdCode, CSTR strMdDesc)
{
    // invoke kernel meltdown as default behavior
    // critical acpi tables must be acquired otherwise continuing on is meaningless
    // for example we need multiple apic descriptor sdt in order to properly wake APs
    Krnlmeltdownimm(mdCode, strMdDesc);
}
