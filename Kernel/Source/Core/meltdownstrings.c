#include "Core/Private/meltdowncodes.h"

CSTR Krnlmddesc(MDCODE mdCode)
{
    switch (mdCode)
    {
    #define mkcase(x) case x: return #x
        /** PROCESSOR */
        mkcase(KR_MDCODE_CRITICAL_PROCESSOR_EXCEPTION);
        mkcase(KR_MDCODE_LOCAL_APIC_INIT_FAILURE);
        /** MEMORY */
        mkcase(KR_MDCODE_PHYSMEMMGMT_INIT_FAILURE);
        mkcase(KR_MDCODE_PHYSMEMMGMT_TEST_FAILURE);
        mkcase(KR_MDCODE_VIRTMEMMGMT_INIT_FAILURE);
        mkcase(KR_MDCODE_LOCAL_APIC_MAP_FAILURE);
        mkcase(KR_MDCODE_PAGE_FAULT);
        mkcase(KR_MDCODE_DMAP_SETUP_OOM);
        mkcase(KR_MDCODE_PMM_META_OOM);
        mkcase(KR_MDCODE_PINNED_PAGE_RELINQUISHED);
        mkcase(KR_MDCODE_RNA_INIT_FAILURE);
        mkcase(KR_MDCODE_KERNEL_ADDRESS_SPACE_CREATION_FAILURE);
        /** DEBUG */
        mkcase(KR_MDCODE_GENERAL_DEBUG);
        mkcase(KR_MDCODE_KERNEL_START_RETURNS);
        mkcase(KR_MDCODE_DMAP_SETUP_DEVCHECK);
        mkcase(KR_MDCODE_ISSUE_IPI_DEVCHECK);
        /** FIRMWARE */
        mkcase(KR_MDCODE_CORRUPT_ACPI_RSDP);
        mkcase(KR_MDCODE_ACPI_SDT_CHECKSUM_NV);
        mkcase(KR_MDCODE_ACPI_TABLE_NOT_FOUND);
        mkcase(KR_MDCODE_ACPI_TABLE_CORRUPT);
    #undef mkcase
    default: break;
    }
    return "IVLDMDCODE"; // InVaLiD MeltDown CODE
}
