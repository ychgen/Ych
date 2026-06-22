#include "Init/KrInitACPI.h"

#include "Core/Krnlmeltdown.h"
#include "Core/KernelState.h"

#include "CPU/MSR.h"

#include "Memory/Virtmemmgmt.h"
#include "KRTL/Krnlmem.h"

#include "ACPI/ACPI.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

static KR_NORETURN VOID FailInit(MDCODE mdCode, CSTR strMdDesc)
{
    // invoke kernel meltdown as default behavior
    // critical acpi tables must be acquired otherwise continuing on is meaningless
    // for example we need multiple apic descriptor sdt in order to properly wake APs
    Krnlmeltdownimm(mdCode, strMdDesc);
}

static VOID ProcessMADT(KrAcpiMadtHeader* pMadt);

VOID KrInitACPI(UINTPTR PhysAddrRSDP)
{
    if (!PhysAddrRSDP)
    {
        FailInit(KR_MDCODE_CORRUPT_ACPI_RSDP, "ACPI RSDP pointer is NULL!");
    }

    KrAcpiRsdp* pRsdp = g_KernelState.AcpiInfo.pRsdp = (KrAcpiRsdp*) KrPhysToVirt(PhysAddrRSDP);
    if (!KrtlBufferEqual(pRsdp->Signature, KR_ACPI_RSDP_SIGNATURE, KR_ACPI_RSDP_SIGNATURE_SIZE))
    {
        FailInit(KR_MDCODE_CORRUPT_ACPI_RSDP, "Invalid ACPI RSDP pointer provided by firmware.");
    }
    if (pRsdp->Revision < 2 || pRsdp->dwLength < sizeof(KrAcpiRsdp))
    {
        FailInit(KR_MDCODE_CORRUPT_ACPI_RSDP, "Bootloader provided an invalid ACPI RSDP or the one the bootloader picked is corrupt thanks to the firmware side (we is innocent). Shoulduva been at least revision 2 and matched minimum size for said revision.");
    }

    if (!KrAcpiIsChecksumValid(pRsdp, KR_ACPI_LEGACY_SECTION_SIZE))
    {
        FailInit(KR_MDCODE_CORRUPT_ACPI_RSDP, "ACPI RSDP legacy section checksum failure.");
    }
    if (!KrAcpiIsChecksumValid(pRsdp, pRsdp->dwLength))
    {
        FailInit(KR_MDCODE_CORRUPT_ACPI_RSDP, "ACPI RSDP whole section checksum failure.");
    }

    if (!pRsdp->qwPhysAddrXsdt)
    {
        FailInit(KR_MDCODE_CORRUPT_ACPI_RSDP, "ACPI RSDP pointer to the XSDT is NULL!");
    }
    KrAcpiSdtHeader* pXsdt = g_KernelState.AcpiInfo.pXsdt = (KrAcpiSdtHeader*) KrPhysToVirt(pRsdp->qwPhysAddrXsdt);
    if (!KrtlBufferEqual(pXsdt->Signature, KR_ACPI_XSDT_SIGNATURE, KR_ACPI_SDT_SIGNATURE_SIZE))
    {
        FailInit(KR_MDCODE_CORRUPT_ACPI_RSDP, "ACPI RSDP pointer to the XSDT is invalid as pointed address contains invalid signature.");
    }
    if (!KrAcpiIsChecksumValid(pXsdt, pXsdt->dwLength))
    {
        FailInit(KR_MDCODE_ACPI_SDT_CHECKSUM_NV, "ACPI XSDT structure has invalid checksum.");
    }

    KrAcpiMadtHeader* pMadt = (KrAcpiMadtHeader*) KrAcpiLocateTable(pXsdt, KR_ACPI_MADT_SIGNATURE);
    if (!pMadt)
    {
        FailInit(KR_MDCODE_ACPI_TABLE_NOT_FOUND, "System has no MADT (Multiple APIC DT) ACPI table present present (or exists in a malformed form)! Issue on firmware side.");
    }
    if (pMadt->AcpiHeader.dwLength < sizeof(KrAcpiMadtHeader))
    {
        FailInit(KR_MDCODE_ACPI_TABLE_CORRUPT, "ACPI MADT table has its dwField lower than the size of the KrAcpiMadtHeader struct.");
    }
    ProcessMADT(pMadt);

    if (!g_KernelState.SmpInfo.IdealProcessorCount)
    {
        FailInit(KR_MDCODE_ACPI_TABLE_CORRUPT, "ACPI MADT table described 0 logical processors. Such occurrence is impossible as you are observing this error.");
    }

    QWORD BspApicId = KrReadModelSpecificRegister(KR_MSR_IA32_X2APIC_APICID);
    g_KernelState.SmpInfo.ProcInfo[(BYTE) BspApicId].BSP = 1;
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
        default:
        {
            break;
        }
        }

        // Minus two because Record Length is inclusive of the little table it makes with Entry Type.
        // We consumed two bytes originally with the pointer
        pRecordsCur += RecordSize - 2;
    }
}
