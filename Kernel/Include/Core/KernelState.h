#ifndef YCH_KERNEL_KERNEL_STATE_H
#define YCH_KERNEL_KERNEL_STATE_H

#include "Krnlych.h"
#include "ACPI/ACPI.h"
#include "BootContract/BootContract.h"

#define MAX_SMP_PROCESSORS 256

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

typedef struct
{
    BYTE PA_UC  : 3; // PA entry that contains UC (Uncacheable)
    BYTE PA_WC  : 3; // PA entry that contains WC (Write-Combining)
    BYTE PA_WT  : 3; // PA entry that contains WT (Write-Through)
    BYTE PA_WP  : 3; // PA entry that contains WP (Write-Protected)
    BYTE PA_WB  : 3; // PA entry that contains WB (Write-Back)
    BYTE PA_UCM : 3; // PA entry that contains UC- (Uncached)
} KrPatMsrState;

typedef struct
{
    KrAcpiRsdp* pRsdp;
    KrAcpiSdtHeader* pXsdt;
} KrAcpiInfo;

typedef struct
{
    UINT IdealProcessorCount;  // Ideal amount of processor that should be active after AP bringup.
    UINT ActiveProcessorCount; // How many logical processors currently active.

    struct
    {
        BOOL  E_V : 1; // Entry_Valid
        BOOL  BSP : 1; // 0 if AP, 1 if BSP
        BOOL  RFD : 1; // Reported for Duty (set once upon AP reaches common kernel entry point alongside his accomplices)
        DWORD AcpiID;
        DWORD LocalApicID;
    } ProcInfo[MAX_SMP_PROCESSORS];
} KrSmpInfo;

typedef struct
{
    UINT NumIvldEncodeOfPTEs; // Number of times KrpteEncodeEntry failed.
} KrDevCheckStats;

/**
 * @brief Global kernel state that applies everywhere
 */
typedef struct
{
    /** If TRUE, kernel has entered Kernel Meltdown. Only God can help us. */
    BOOL bMeltdown;

    /** Kernel Load Information, containing fields like load address and certain area sizes. */
    KrLoadInfo LoadInfo;

    /** Developer-Check Statistics */
    KrDevCheckStats DevCheckStats;

    /** PAT MSR state. */
    KrPatMsrState PatMsrState; 

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

    /** ACPI stuff */
    KrAcpiInfo AcpiInfo;

    /** Symmetric Multi-Processing */
    KrSmpInfo SmpInfo;

    /** GOP/VBE Frame Buffer Information (Non-Driver Related) */
    KrFrameBufferInfo FrameBufferInfo;

    /** Video Output Information */
    KrVideoOutputProtocol VideoOutputProtocol;
    const VOID* VideoOutputContext; // Pointer to relevant video information data. Content depends on KrKernelState.VideoOutputProtocol.
} KrKernelState;

extern KrKernelState g_KernelState;

#endif // !YCH_KERNEL_KERNEL_STATE_H
