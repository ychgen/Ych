#ifndef YCH_KERNEL_CORE_PRIVATE_MELTDOWNCODES_H
#define YCH_KERNEL_CORE_PRIVATE_MELTDOWNCODES_H

#include "Krnlych.h"

// `meltdown codes are unsigned 64-bit integers. this is standard.`
typedef ULONG MDCODE;

#define KR_CATEGORY_MDCODE_PROCESSOR ((MDCODE)0x1000)
#define KR_CATEGORY_MDCODE_MEMORY    ((MDCODE)0x2000)
#define KR_CATEGORY_MDCODE_IO        ((MDCODE)0x3000)
#define KR_CATEGORY_MDCODE_DEBUG     ((MDCODE)0x4000)

/** PROCESSOR */

#define KR_MDCODE_CRITICAL_PROCESSOR_EXCEPTION (KR_CATEGORY_MDCODE_PROCESSOR     +    1) // CPU Exception

/** MEMORY */

#define KR_MDCODE_PHYSMEMMGMT_INIT_FAILURE     (KR_CATEGORY_MDCODE_MEMORY +    1) // KrInitPhysmemmgmt() failure
#define KR_MDCODE_PHYSMEMMGMT_TEST_FAILURE     (KR_CATEGORY_MDCODE_MEMORY +    2) // PMM Acquire/Relinquishment sanity check failure

/** DEBUG */

#define KR_MDCODE_GENERAL_DEBUG                (KR_CATEGORY_MDCODE_DEBUG  +    1) // General Debug (use for debug meltdowns)
#define KR_MDCODE_KERNEL_START_RETURNS         (KR_CATEGORY_MDCODE_DEBUG  +    2) // KrKernelStart returned

#endif // !YCH_KERNEL_CORE_PRIVATE_MELTDOWNCODES_H
