#ifndef YCH_KERNEL_KERNEL_STATE_H
#define YCH_KERNEL_KERNEL_STATE_H

#include "Core/Fundtypes.h"
#include "BootContract/BootContract.h"

typedef enum
{
    KR_VIDEO_OUTPUT_PROTOCOL_NULL,
    KR_VIDEO_OUTPUT_PROTOCOL_DISPLAYWIDE_TEXT,
    KR_VIDEO_OUTPUT_PROTOCOL_VDDI,
    KR_VIDEO_OUTPUT_PROTOCOL_TEXT_STREAM
} KrVideoOutputProtocol;

typedef struct
{
    USIZE   BinarySize;
    USIZE   ReserveSize;

    UINTPTR AddrPhysicalBase;
    UINTPTR AddrVirtualBase;
} KrLoadInfo;

typedef struct
{
    UINTPTR AddrDescriptorTable;       // Address to GDT entries array
    WORD    NumberOfDescriptorEntries; // NULL DESCRIPTOR included
    WORD    KernelCodeSegmentSelector;
    WORD    KernelDataSegmentSelector;
} KrGlobalDescriptorTableState;

typedef struct
{
    uintptr_t BaseAddrPhysical;
    uintptr_t BaseAddrVirtual;
    uintptr_t BaseAddr; // Use this to address local APIC!
} KrLocalAPICState;

typedef struct
{
    USIZE PageSize;       // Size in bytes per physical page.
    USIZE TotalPages;     // Total amount of physical pages.
    USIZE UnusablePages;  // Total amount of physical pages that cannot be used for reasons like reserved by the platform, MMIO, kernel reserved etc.
    USIZE AcquiredPages;  // Total amount of physical pages currently acquired and managed by Physmemmgmt.
} KrStatePMM;

typedef struct
{
    /** If TRUE, kernel has entered Kernel Meltdown. Only God can help us. */
    BOOL       bMeltdown;

    /** Kernel Load Information */
    KrLoadInfo LoadInfo;

    /** Memory Map */
    KrMemoryMapInfo MemoryMapInfo;
    KrMemoryDescriptor* MemoryMap;

    /** Physmemmgmt */
    KrStatePMM StatePMM;

    /** Global Descriptor Table Information */
    KrGlobalDescriptorTableState StateGDT;

    /** Interrupt Descriptor Table Information */
    UINTPTR AddrIDT; // Address to IDT entries array.

    /** Local APIC Information */
    KrLocalAPICState StateLocalAPIC;

    /** Video Output Information */
    KrVideoOutputProtocol VideoOutputProtocol;
    void* VideoOutputContext;
} KrKernelState;

extern KrKernelState g_KernelState;

#endif // !YCH_KERNEL_KERNEL_STATE_H
