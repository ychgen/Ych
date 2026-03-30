/**
 * 
 * BootContract is the shared expectations between
 * the Bootloader YchBoot and the Kernel Kernelych.
 * This header carries shared types and shared values such as memory addresses.
 * 
 */

#ifndef YCH_BOOT_CONTRACT_H
#define YCH_BOOT_CONTRACT_H

#include <stdint.h>

/** The physical address that the Bootloader will load the kernel to. */
#define KERNEL_LOAD_ADDRESS      0x100000 // 1MiBs
#define KERNEL_RESERVED_AREA_END 0x4000000 // 64MiBs. Area between KERNEL_LOAD_ADDRESS and KERNEL_RESERVED_AREA_END is completely reserved for kernel at all times.
/** KrSystemInfoPack.Magic must equal this. */
#define SYSTEM_INFO_PACK_MAGIC   0x4B594348 // "KYCH"

#define KERNEL_FILE_NAME_ON_DISK        "\\Ych\\Krnlych.kr"
#define KERNEL_FILE_NAME_ON_DISK_U16LE L"\\Ych\\Krnlych.kr"

typedef struct __attribute__((packed))
{
    uint64_t PhysicalFramebufferAddress;
    uint64_t FramebufferSize;
    uint32_t FramebufferWidth;
    uint32_t FramebufferHeight;
    uint32_t PixelsPerScanLine;
} KrGraphicsInfo;

typedef struct __attribute__((packed))
{
    uint64_t PhysicalAddress; // Address of first descriptor entry
    uint64_t MemoryMapSize;   // Size of the memory map
    uint64_t DescriptorSize;  // Size per each descriptor entry
    uint64_t NumberOfEntries; // Number of descriptor entries (essentially `MemoryMapSize / DescriptorSize`)
    uint32_t DescriptorVersion;
} KrMemoryMapInfo;

typedef struct __attribute__((packed))
{
    uint32_t        Magic;
    uint64_t        KernelBinarySize; // Size of the loaded kernel binary file in bytes.
    KrGraphicsInfo  GraphicsInfo;
    KrMemoryMapInfo MemoryMapInfo;
} KrSystemInfoPack;

#endif // !YCH_PACT_OF_INIT_H
