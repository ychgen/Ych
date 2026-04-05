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

// Non-kernel, for Bootelvt
#define BOOTELVT_LOAD_ADDRESS    0x100000
/** The physical address that the Bootloader will load the kernel to. */
#define KERNEL_LOAD_ADDRESS      0x200000 // 2MiBs
/** This much memory area will be completely reserved to kernel at all times */
#define KERNEL_RESERVE_SIZE      0x8000000 // 128MiBs
/** KrSystemInfoPack.Magic must equal this. */
#define SYSTEM_INFO_PACK_MAGIC   0x4B594348 // "KYCH"

#define MEMORY_MAP_PHYSADDR      0x4000
#define FRAMEBUFFER_VIRTUAL_ADDR 0xFFFFFFFFC0000000

#define BOOTELVT_FILE_NAME_ON_DISK_U16LE L"\\Ych\\Bootelvt.bin"
#define KERNEL_FILE_NAME_ON_DISK          "\\Ych\\Krnlych.kr"
#define KERNEL_FILE_NAME_ON_DISK_U16LE   L"\\Ych\\Krnlych.kr"

typedef struct __attribute__((packed))
{
    uint64_t PhysicalFramebufferAddress;
    uint64_t FramebufferSize;
    uint32_t FramebufferWidth;
    uint32_t FramebufferHeight;
    uint32_t PixelsPerScanLine;
} KrGraphicsInfo;

typedef struct __attribute__((aligned(8)))
{
    uint32_t Type;
    uint64_t PhysicalStart;
    uint64_t VirtualStart;
    uint64_t NumberOfPages; // Number of 4KiB pages. Multiply by 4096 to get byte size.
    uint64_t Attributes;
} KrMemoryDescriptor;

typedef struct __attribute__((packed))
{
    uint64_t PhysicalAddress; // Address of first descriptor entry
    uint64_t MemoryMapSize;   // Size of the memory map
    uint64_t DescriptorSize;  // Size per each descriptor entry
    uint64_t NumberOfEntries; // Number of descriptor entries (essentially `MemoryMapSize / DescriptorSize`)
    uint32_t DescriptorVersion;
} KrMemoryMapInfo;

/** ALWAYS EXPAND THIS STRUCT STARTING AT THE END, DON'T INSERT ANYTHING ABOVE CURRENT ONES
 *  BOOTELVT DEPENDS ON CERTAIN OFFSETS OF FIELDS!
 */
typedef struct __attribute__((packed))
{
    uint32_t        Magic;
    uint64_t        KernelBinarySize; // Size of the loaded kernel binary file in bytes.
    
    uintptr_t       AddrKernelLoad;
    uintptr_t       AddrKernelSpaceEnd; // Physical Address where kernel reserved space ends.
    uintptr_t       AddrInitialStack;   // Stack is always put at: AddrKernelSpaceEnd. It is 2MiB. Therefore, last 2MiB of kernel reserved area is the stack.

    KrGraphicsInfo  GraphicsInfo;
    KrMemoryMapInfo MemoryMapInfo;
} KrSystemInfoPack;

#endif // !YCH_BOOT_CONTRACT_H
