#ifndef YCH_KERNEL_CPU_PAT_H
#define YCH_KERNEL_CPU_PAT_H

#include "Krnlych.h"

#define KR_PAT_UNCACHEABLE     0 // UC
#define KR_PAT_WRITE_COMBINING 1 // WC
#define KR_PAT_WRITE_THROUGH   4 // WT
#define KR_PAT_WRITE_PROTECTED 5 // WP
#define KR_PAT_WRITE_BACK      6 // WB
#define KR_PAT_UNCACHED        7 // UC-

typedef struct
{
    BYTE PWT : 1;
    BYTE PCD : 1;
    BYTE PAT : 1;
} KrPatSelect;

VOID        KrLoadPatMsr(QWORD qwPatMsr);
KrPatSelect KrSelectPat(BYTE MemType);

#endif // !YCH_KERNEL_CPU_PAT_H
