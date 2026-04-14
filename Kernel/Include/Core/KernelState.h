#ifndef YCH_KERNEL_KERNEL_STATE_H
#define YCH_KERNEL_KERNEL_STATE_H

#include "Krnlych.h"
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
    SIZE    BinarySize;
    SIZE    ReserveSize;

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
    UINTPTR BaseAddrPhysical;
    UINTPTR BaseAddrVirtual;
    UINTPTR BaseAddr; // Use this to address local APIC!
} KrLocalAPICState;

/**
 * @brief Global kernel state that applies everywhere
 */
typedef struct
{
    /** If TRUE, kernel has entered Kernel Meltdown. Only God can help us. */
    BOOL bMeltdown;

    /** Kernel Load Information, containing fields like load address and certain area sizes. */
    KrLoadInfo LoadInfo;

    /** Memory Map */
    KrMemoryMapInfo MemoryMapInfo;
    KrMemoryDescriptor* MemoryMap;

    /** Global Descriptor Table Information */
    KrGlobalDescriptorTableState StateGDT;

    /** Interrupt Descriptor Table Information */
    UINTPTR AddrIDT; // Address to IDT entries array.

    /** Local APIC Information */
    KrLocalAPICState StateLocalAPIC;

    /** Video Output Information */
    KrVideoOutputProtocol VideoOutputProtocol;
    void* VideoOutputContext; // Pointer to relevant video information data. Content depends on KrKernelState.VideoOutputProtocol.
} KrKernelState;

extern KrKernelState g_KernelState;

#endif // !YCH_KERNEL_KERNEL_STATE_H
