// Private header for Virtmemmgmt.c

#ifndef YCH_KERNEL_MEMORY_PTE_H
#define YCH_KERNEL_MEMORY_PTE_H

#include "Krnlych.h"

#define KR_PAGE_STRUCTURE_ENTRY_COUNT     512
#define KR_PAGE_STRUCTURE_ENTRY_CONT_SIZE 8
#define KR_PAGE_STRUCTURE_SIZE            ( KR_PAGE_STRUCTURE_ENTRY_COUNT * KR_PAGE_STRUCTURE_ENTRY_CONT_SIZE ) // 512 * 8 = 4 KiB per structure like PML4, PDPT, PD and PT.

typedef struct KR_PACKED
{
    UINT bPresent    : 1;
    UINT bWritable   : 1;
    UINT bSupervisor : 1;
    UINT PWT         : 1;
    UINT PCD         : 1;
    UINT PS          : 1;
    UINT bGlobal     : 1;
    UINT bNoExecute  : 1;
} KrPageTableEntryFlags;

// 8 bytes
typedef QWORD KrPageTableEntry;

KrPageTableEntry KrEncodePageTableEntry
(
    UINTPTR pPhysicalAddress,
    KrPageTableEntryFlags Flags
)
{
    return (KrPageTableEntry)
        ((QWORD)(Flags.bNoExecute) << 63) | ((QWORD)(0) << 52) | ((QWORD)(pPhysicalAddress)) |
        ((QWORD)(0) << 9) | ((QWORD)(Flags.bGlobal) << 8) | ((QWORD)(Flags.PS) << 7) | ((QWORD)(0) << 5) |
        ((QWORD)(Flags.PCD) << 4) | ((QWORD)(Flags.PWT) << 3) | ((QWORD)(Flags.bSupervisor) << 2) |
        ((QWORD)(Flags.bWritable) << 1) | (QWORD)(Flags.bPresent);
}

KrPageTableEntry KrEncodeLargePageEntry
(
    UINTPTR pPhysicalAddress,
    KrPageTableEntryFlags Flags,
    BYTE PAT
)
{
    return (KrPageTableEntry)
        ((QWORD)(Flags.bNoExecute) << 63) | ((QWORD)(0) << 52) | ((QWORD)(PAT) << 12) | ((QWORD)(pPhysicalAddress)) |
        ((QWORD)(0) << 9) | ((QWORD)(Flags.bGlobal) << 8) | ((QWORD)(Flags.PS) << 7) | ((QWORD)(0) << 5) |
        ((QWORD)(Flags.PCD) << 4) | ((QWORD)(Flags.PWT) << 3) | ((QWORD)(Flags.bSupervisor) << 2) |
        ((QWORD)(Flags.bWritable) << 1) | (QWORD)(Flags.bPresent);
}

#endif // !YCH_KERNEL_MEMORY_PTE_H
