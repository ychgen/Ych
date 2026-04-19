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
    UINTPTR PhysicalAddress;
    UINTPTR VirtualAddress;
    UINT    Size;
} KrFrameBufferInfo;

typedef struct
{
    UINTPTR AddrDescriptorTable;       // Address to GDT entries array
    WORD    NumberOfDescriptorEntries; // NULL DESCRIPTOR included!
    WORD    KernelCodeSegmentSelector;
    WORD    KernelDataSegmentSelector;
} KrGlobalDescriptorTableState;

/**
 * @brief Global kernel state that applies everywhere
 */
typedef struct
{
    /** If TRUE, kernel has entered Kernel Meltdown. Only God can help us. */
    BOOL bMeltdown;

    /** Kernel Load Information, containing fields like load address and certain area sizes. */
    KrLoadInfo LoadInfo;

    /** Firmware Provided Memory Map */
    KrMemoryMapInfo MemoryMapInfo;
    KrMemoryDescriptor* MemoryMap;
    /* Canonical Memory Map */
    UINT NumCanonicalMapEntries;
    KrMemoryDescriptor* CanonicalMemoryMap;

    /** Global Descriptor Table Information */
    KrGlobalDescriptorTableState StateGDT;

    /** Interrupt Descriptor Table Information */
    UINTPTR AddrIDT; // Address to IDT entries array.

    /** GOP/VBE Frame Buffer Information (Non-Driver Related) */
    KrFrameBufferInfo FrameBufferInfo;

    /** Video Output Information */
    KrVideoOutputProtocol VideoOutputProtocol;
    const VOID* VideoOutputContext; // Pointer to relevant video information data. Content depends on KrKernelState.VideoOutputProtocol.
} KrKernelState;

extern KrKernelState g_KernelState;

#endif // !YCH_KERNEL_KERNEL_STATE_H
