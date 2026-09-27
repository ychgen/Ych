#ifndef YCH_KERNEL_MEMORY_MMIO_H
#define YCH_KERNEL_MEMORY_MMIO_H

#include "Krnlych.h"

#define MMIO_FLAG_USE_LARGE_PAGES (1 << 0)
#define MMIO_FLAG_WRITE_COMBINE   (1 << 1) // Uses WC policy instead of UC

UINTPTR MmioAllocateRange(UINT Alignment, SIZE Size);
UINTPTR MmioMapDevice(UINTPTR PaddrDeviceBase, SIZE DeviceAddressSize, BYTE Flags);

#endif // !YCH_KERNEL_MEMORY_MMIO_H
