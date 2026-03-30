#ifndef YCH_KERNEL_KERNEL_STATE_H
#define YCH_KERNEL_KERNEL_STATE_H

#include "YchBootContract.h"

#include <stdint.h>

typedef struct
{
    uintptr_t AddrDescriptorTable;       // Address to GDT entries array
    uint16_t  NumberOfDescriptorEntries; // NULL included
    uint16_t  KernelCodeSegmentSelector;
    uint16_t  KernelDataSegmentSelector;
} KrGlobalDescriptorTableState;

typedef struct
{
    uintptr_t BaseAddrPhysical;
    uintptr_t BaseAddrVirtual;
    uintptr_t BaseAddr; // Use this to address local APIC!
} KrLocalAPICState;

typedef struct
{
    /* Prekernel environment collected information by Bootloader. */
    KrSystemInfoPack SystemInfoPack;

    /** Global Descriptor Table Information */
    KrGlobalDescriptorTableState StateGDT;

    /** Interrupt Descriptor Table Information */
    uintptr_t AddrIDT; // Address to IDT entries array.

    /** Local APIC Information */
    KrLocalAPICState StateLocalAPIC;
} KrKernelState;

extern KrKernelState g_KernelState;

#endif // !YCH_KERNEL_KERNEL_STATE_H
