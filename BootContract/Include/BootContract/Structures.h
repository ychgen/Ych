#ifndef YCH_BOOT_CONTRACT_STRUCTURES
#define YCH_BOOT_CONTRACT_STRUCTURES

#include <stdint.h>

#define KR_MEMORY_TYPE_RESERVED               0
#define KR_MEMORY_TYPE_LOADER_CODE            1
#define KR_MEMORY_TYPE_LOADER_DATA            2
#define KR_MEMORY_TYPE_BOOT_SERVICES_CODE     3
#define KR_MEMORY_TYPE_BOOT_SERVICES_DATA     4
#define KR_MEMORY_TYPE_RUNTIME_SERVICES_CODE  5
#define KR_MEMORY_TYPE_RUNTIME_SERVICES_DATA  6
#define KR_MEMORY_TYPE_CONVENTIONAL_MEMORY    7
#define KR_MEMORY_TYPE_UNUSABLE_MEMORY        8
#define KR_MEMORY_TYPE_ACPI_RECLAIM_MEMORY    9
#define KR_MEMORY_TYPE_ACPI_MEMORY_NVS       10
#define KR_MEMORY_TYPE_MMIO                  11
#define KR_MEMORY_TYPE_MMIO_PORT_SPACE       12
#define KR_MEMORY_TYPE_PAL_CODE              13
#define KR_MEMORY_TYPE_PERSISTENT_MEMORY     14

typedef struct __attribute__((packed))
{
    uintptr_t PhysicalFramebufferAddress;
    uint64_t  FramebufferSize;
    uint32_t  FramebufferWidth;
    uint32_t  FramebufferHeight;
    uint32_t  PixelsPerScanLine;
    uint32_t  Pad;
} KrGraphicsInfo;

typedef struct __attribute__((packed))
{
    uint64_t PhysicalBase; // Physical base address of this memory region.
    uint64_t PageCount;    // Number of pages in this memory region. To calculate size in bytes, use `MemDesc.PageCount * MemMapInfo.PageSize`
    uint64_t Attributes;   // Bit field of certain attributes.
    uint32_t Type;         // What kind of memory region this is.
    uint32_t Pad;          // padding, irrelevant
} KrMemoryDescriptor;

typedef struct __attribute__((packed))
{
    uint64_t PhysicalAddress;   // Physical address of first descriptor entry. At this address resides `KrMemoryDescriptor[MemMapInfo.NumberOfEntries]`
    uint64_t MemoryMapSize;     // Total size of the memory map in bytes.
    uint64_t EntryCount;        // Number of descriptor entries.
    uint32_t PageSize;          // Size per each page.
    uint32_t Pad;               // padding, irrelevant
} KrMemoryMapInfo;

/**
 * ALWAYS EXPAND THIS STRUCT STARTING AT THE END, DON'T INSERT ANYTHING ABOVE CURRENT ONES
 * BOOTELVT DEPENDS ON CERTAIN OFFSETS OF ALREADY EXISTING FIELDS!
 * 
 * YOU HAVE BEEN WARNED.
 * YOU HAVE BEEN WARNED.
 * YOU HAVE BEEN WARNED.
 */
typedef struct __attribute__((packed))
{
    // Validation magic value.
    uint32_t        Magic;

    uint64_t        KernelBinarySize; // Size of the loaded kernel binary file in bytes.
    uintptr_t       KernelPhysicalBase; // Where kernel resides physically.
    uintptr_t       KernelVirtualBase;
    uintptr_t       KernelStackPhysicalBase;
    uintptr_t       KernelStackVirtualBase;
    uintptr_t       KernelBootstrapArenaBase; // `Bootstrap Kernel Arena` virtual base address.
    uint64_t        KernelBootstrapArenaSize; // Size of the `Bootstrap Kernel Arena`.
    uint64_t        KernelReserveSize;        // Size (in bytes) if the area kernel reserves for itself.

    KrGraphicsInfo  GraphicsInfo;
    KrMemoryMapInfo MemoryMapInfo;
} KrSystemInfoPack;

#endif // !YCH_BOOT_CONTRACT_STRUCTURES
