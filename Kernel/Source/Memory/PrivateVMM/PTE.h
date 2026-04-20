// Private header for Virtmemmgmt.c

#ifndef YCH_KERNEL_MEMORY_PTE_H
#define YCH_KERNEL_MEMORY_PTE_H

#include "Memory/Virtmemmgmt.h"

#define KR_PAGE_STRUCTURE_ENTRY_COUNT 512
#define KR_PAGE_STRUCTURE_ENTRY_SIZE    8
#define KR_PAGE_STRUCTURE_SIZE       ( KR_PAGE_STRUCTURE_ENTRY_COUNT * KR_PAGE_STRUCTURE_ENTRY_SIZE ) // 512 * 8 = 4 KiB per structure like PML4, PDPT, PD and PT.

typedef struct KR_PACKED
{
    UINT bPresent    : 1;
    UINT bWritable   : 1;
    UINT bUserMode   : 1;
    UINT PWT         : 1;
    UINT PCD         : 1;
    UINT PS          : 1;
    UINT bGlobal     : 1;
    UINT bNoExecute  : 1;
} KrPageTableEntryFlags;

// 8 bytes
typedef QWORD KrPageTableEntry;

static KrPageTableEntry KrEncodePageTableEntry
(
    UINTPTR pPhysicalAddress,
    KrPageTableEntryFlags Flags
)
{
    return (KrPageTableEntry)
        ((QWORD)(GetVirtmemmgmtState()->bNoExecuteSupport ? Flags.bNoExecute : 0) << 63) | ((QWORD)(0) << 52) | ((QWORD)(pPhysicalAddress)) |
        ((QWORD)(0) << 9) | ((QWORD)(Flags.bGlobal) << 8) | ((QWORD)(Flags.PS) << 7) | ((QWORD)(0) << 5) |
        ((QWORD)(Flags.PCD) << 4) | ((QWORD)(Flags.PWT) << 3) | ((QWORD)(Flags.bUserMode) << 2) |
        ((QWORD)(Flags.bWritable) << 1) | (QWORD)(Flags.bPresent);
}

static KrPageTableEntry KrEncodeLargePageEntry
(
    UINTPTR pPhysicalAddress,
    KrPageTableEntryFlags Flags,
    BYTE PAT
)
{
    return (KrPageTableEntry)
        ((QWORD)(GetVirtmemmgmtState()->bNoExecuteSupport ? Flags.bNoExecute : 0) << 63) | ((QWORD)(0) << 52) | ((QWORD)(PAT) << 12) | ((QWORD)(pPhysicalAddress)) |
        ((QWORD)(0) << 9) | ((QWORD)(Flags.bGlobal) << 8) | ((QWORD)(Flags.PS) << 7) | ((QWORD)(0) << 5) |
        ((QWORD)(Flags.PCD) << 4) | ((QWORD)(Flags.PWT) << 3) | ((QWORD)(Flags.bUserMode) << 2) |
        ((QWORD)(Flags.bWritable) << 1) | (QWORD)(Flags.bPresent);
}

static KrPageTableEntry KrEncodeHugePageTableEntry
(
    UINTPTR pPhysicalAddress,
    KrPageTableEntryFlags Flags
)
{
    return (KrPageTableEntry)
        ((QWORD)(GetVirtmemmgmtState()->bNoExecuteSupport ? Flags.bNoExecute : 0) << 63) | ((QWORD)(pPhysicalAddress)) | ((QWORD)(Flags.bGlobal) << 8) |
        ((QWORD)(Flags.PS) << 7) | ((QWORD)(0) << 5) | ((QWORD)(Flags.PCD) << 4) | ((QWORD)(Flags.PWT) << 3) | ((QWORD)(Flags.bUserMode) << 2) |
        ((QWORD)(Flags.bWritable) << 1) | (QWORD)(Flags.bPresent);
}

typedef struct
{
    UINT PML4;
    UINT PDPT;
    UINT PD;
    UINT PT;
    UINT Offset;
} KrVirtualAddress;
typedef enum
{
    KR_VAMODE_SMALL,
    KR_VAMODE_LARGE,
    KR_VAMODE_HUGE,
} KrVirtualAddressMode;

static KrVirtualAddress KrVirtBreakdown(UINTPTR pAddrVirt, KrVirtualAddressMode eMode)
{
    KrVirtualAddress Result = {0};
    Result.PML4 = (pAddrVirt >> 39) & 0x1FF;
    Result.PDPT = (pAddrVirt >> 30) & 0x1FF;

    switch (eMode)
    {
    case KR_VAMODE_SMALL:
    {
        Result.PD     = (pAddrVirt >> 21) & 0x1FF;
        Result.PT     = (pAddrVirt >> 12) & 0x1FF;
        Result.Offset = (pAddrVirt >> 00) & 0xFFF;
        break;
    }
    case KR_VAMODE_LARGE:
    {
        Result.PD     = (pAddrVirt >> 21) & 0x1FF;
        Result.Offset = (pAddrVirt >> 12) & 0x1FFFFF;
        break;
    }
    case KR_VAMODE_HUGE:
    {
        Result.Offset = (pAddrVirt >> 21) & 0x3FFFFFFF;
        break;
    }
    }

    return Result;
}

typedef enum
{
    KR_PAGE_HIERARCHY_PML4,
    KR_PAGE_HIERARCHY_PDPT,
    KR_PAGE_HIERARCHY_PD,
    KR_PAGE_HIERARCHY_PT
} KrPageHierarchy;

KrPageTableEntry* KrGetAddrOfPTE(KrVirtualAddress VirtAddr, KrPageHierarchy eHierarchy)
{
    QWORD Idx = 0;
    switch (eHierarchy)
    {
    case KR_PAGE_HIERARCHY_PML4:
    {
        Idx = (((QWORD)(VirtAddr.PML4)));
        break;
    }
    case KR_PAGE_HIERARCHY_PDPT:
    {
        Idx = (((QWORD)(VirtAddr.PML4) << 9)) | (((QWORD)(VirtAddr.PDPT)));
        break;
    }
    case KR_PAGE_HIERARCHY_PD:
    {
        Idx = (((QWORD)(VirtAddr.PML4) << 18)) | (((QWORD)(VirtAddr.PDPT) << 9)) | (((QWORD)(VirtAddr.PD)));
        break;
    }
    case KR_PAGE_HIERARCHY_PT:
    {
        Idx = (((QWORD)(VirtAddr.PML4) << 27)) | (((QWORD)(VirtAddr.PDPT) << 18)) | (((QWORD)(VirtAddr.PD) << 9)) | (((QWORD)(VirtAddr.PT)));
        break;
    }
    default:
    {
        return NULLPTR;
    }
    }
    return ((KrPageTableEntry*) g_StateVMM.AddrRecursiveMapBase) + Idx;
}

#endif // !YCH_KERNEL_MEMORY_PTE_H
