#ifndef YCH_KERNEL_CORE_PRIVATE_MELTDOWNCODES_H
#define YCH_KERNEL_CORE_PRIVATE_MELTDOWNCODES_H

#include "Krnlych.h"

// `meltdown codes are unsigned 64-bit integers. this is standard.`
typedef ULONG MDCODE;

// MDCODE descriptor
CSTR Krnlmddesc(MDCODE code);

#define KR_CATEGORY_MDCODE_PROCESSOR ((MDCODE)0x1000)
#define KR_CATEGORY_MDCODE_MEMORY    ((MDCODE)0x2000)
#define KR_CATEGORY_MDCODE_IO        ((MDCODE)0x3000)
#define KR_CATEGORY_MDCODE_DEBUG     ((MDCODE)0x4000)

/** KR_CATEGORY_MDCODE_PROCESSOR */

#define KR_MDCODE_CRITICAL_PROCESSOR_EXCEPTION (KR_CATEGORY_MDCODE_PROCESSOR +    1) // CPU Exception
#define KR_MDCODE_PROCESSOR_X2APIC_INCAPABLE   (KR_CATEGORY_MDCODE_PROCESSOR +    2) // Processor incapable of x2APIC.

/** KR_CATEGORY_MDCODE_MEMORY */

#define KR_MDCODE_PHYSMEMMGMT_INIT_FAILURE     (KR_CATEGORY_MDCODE_MEMORY +    1) // KrInitPhysmemmgmt() failure
#define KR_MDCODE_PHYSMEMMGMT_TEST_FAILURE     (KR_CATEGORY_MDCODE_MEMORY +    2) // PMM Acquire/Relinquishment sanity check failure
#define KR_MDCODE_VIRTMEMMGMT_INIT_FAILURE     (KR_CATEGORY_MDCODE_MEMORY +    3) // KrInitVirtmemmgmt() failure
#define KR_MDCODE_LOCAL_APIC_MAP_FAILURE       (KR_CATEGORY_MDCODE_MEMORY +    4) // Couldn't map LAPIC
#define KR_MDCODE_PAGE_FAULT                   (KR_CATEGORY_MDCODE_MEMORY +    5) // No-nonsense page fault (like kernel accessing a kernel-level guard page.)
#define KR_MDCODE_DMAP_SETUP_OOM               (KR_CATEGORY_MDCODE_MEMORY +    6)

/** KR_CATEGORY_MDCODE_DEBUG */

#define KR_MDCODE_GENERAL_DEBUG                (KR_CATEGORY_MDCODE_DEBUG  +    1) // General Debug (use for debug meltdowns)
#define KR_MDCODE_KERNEL_START_RETURNS         (KR_CATEGORY_MDCODE_DEBUG  +    2) // KrKernelStart returned
#define KR_MDCODE_DMAP_SETUP_DEVCHECK          (KR_CATEGORY_MDCODE_DEBUG  +    3)

#endif // !YCH_KERNEL_CORE_PRIVATE_MELTDOWNCODES_H
