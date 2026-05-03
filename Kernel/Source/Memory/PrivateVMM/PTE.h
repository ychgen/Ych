// Private header for Virtmemmgmt.c

#ifndef YCH_KERNEL_MEMORY_PTE_H
#define YCH_KERNEL_MEMORY_PTE_H

#include "Memory/Virtmemmgmt.h"

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

        Result.PT = 0;
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

PTE* KrGetAddrOfPTE(KrVirtualAddress VirtAddr, KrPageHierarchy eHierarchy)
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
    return ((PTE*) g_StateVMM.VirtAddrRecursiveBase) + Idx;
}

#endif // !YCH_KERNEL_MEMORY_PTE_H
