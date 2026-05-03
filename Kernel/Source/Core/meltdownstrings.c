#include "Core/Private/meltdowncodes.h"

CSTR Krnlmddesc(MDCODE mdCode)
{
    switch (mdCode)
    {
    #define mkcase(x) case x: return #x
        /** PROCESSOR */
        mkcase(KR_MDCODE_CRITICAL_PROCESSOR_EXCEPTION);
        mkcase(KR_MDCODE_PROCESSOR_X2APIC_INCAPABLE);
        /** MEMORY */
        mkcase(KR_MDCODE_PHYSMEMMGMT_INIT_FAILURE);
        mkcase(KR_MDCODE_PHYSMEMMGMT_TEST_FAILURE);
        mkcase(KR_MDCODE_VIRTMEMMGMT_INIT_FAILURE);
        mkcase(KR_MDCODE_LOCAL_APIC_MAP_FAILURE);
        mkcase(KR_MDCODE_PAGE_FAULT);
        mkcase(KR_MDCODE_DMAP_SETUP_OOM);
        /** DEBUG */
        mkcase(KR_MDCODE_GENERAL_DEBUG);
        mkcase(KR_MDCODE_KERNEL_START_RETURNS);
        mkcase(KR_MDCODE_DMAP_SETUP_DEVCHECK);
    #undef mkcase
    default: break;
    }
    return "IVLDMDCODE"; // InVaLiD MeltDown CODE
}
