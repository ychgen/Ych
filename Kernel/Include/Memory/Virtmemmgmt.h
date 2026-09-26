/**
 * 
 * Virtmemmgmt is the part of the kernel responsible for managing virtual memory.
 * It handles acquisition and releasing of virtual addresses and mapping.
 * Bookkeeping of the virtual memory, basically.
 * It is also called VMM or Virtual Memory Management.
 * 
 */

#ifndef YCH_KERNEL_MEMORY_VIRTMEMMGMT_H
#define YCH_KERNEL_MEMORY_VIRTMEMMGMT_H

#include "Krnlych.h"

#include "Memory/Vmmdef/MakeVirt.h"
#include "Memory/Vmmdef/Indices.h"

#define VMR_FLAG_CACHING_PROTOCOL ((1 << 1) | (1 << 0))
#define VMR_FLAG_PAGE_SIZE        (1 << 2)
#define VMR_FLAG_ALLOW_CODE_EXEC  (1 << 3)
#define VMR_FLAG_READABLE         (1 << 4)
#define VMR_FLAG_WRITABLE         (1 << 5)
#define VMR_FLAG_GUARD            (1 << 6)
#define VMR_FLAG_STATIC           (1 << 7)

#define VMR_CACHE_PROTOCOL_WRITE_BACK    0b00 // WB
#define VMR_CACHE_PROTOCOL_UNCACHEABLE   0b01 // UC
#define VMR_CACHE_PROTOCOL_WRITE_COMBINE 0b10 // WC

// Keep this struct less than or equal to 64 bytes (Region Node Allocator mandates this due to its design)
typedef struct KrVirtualMemoryRegion
{
    struct KrAddressSpace*        pAddressSpace; // Owning address space.
    struct KrVirtualMemoryRegion* pPrev; // Pointer to Previous Node
    struct KrVirtualMemoryRegion* pNext; // Pointer to Next Node

    UINTPTR VaddrStart; // Inclusive start
    UINTPTR VaddrEnd;   // Exclusive end

    // But   7  : VMR Mapping is Static (cannot grow, modify or commit post acquisition of the region)
    // Bit   6  : Pages are Guard Pages
    // Bit   5  : Pages Can be Written to
    // Bit   4  : Pages Can be Read from
    // Bit   3  : Allow Code Execution
    // Bit   2  : Pages Page Size (clear = 4 KiB, set = 2 MiB)
    // Bits 1-0 : Caching policy
    BYTE    Flags;
} KrVirtualMemoryRegion;

typedef struct KrAddressSpace
{
    UINTPTR PaddrRoot; // Root-level paging structure physical address (CR3 value for this address space)
    // VMR linked list is always sorted. Goes from smallest to largest, always.
    struct KrVirtualMemoryRegion* pRootVMR;
    struct KrVirtualMemoryRegion* pTailVMR;
    UINT NrVMRs;
} KrAddressSpace;

typedef struct
{
    BOOL  bInitialized : 1;

    BOOL  bHugePageSupport  : 1; // Processor's 1 GiB page capability
    BOOL  bNoExecuteSupport : 1; // Processor's PTE NX bit capability

    struct { /* Direct-Map Related */
        ULONG TotalPageStructs; // Total Paging Structures allocated for direct-mapping.

        ULONG HugePages;  // Number of huge  (1GiB) pages.
        ULONG LargePages; // Number of large (2MiB) pages.
        ULONG SmallPages; // Number of small (4KiB) pages.
        ULONG TotalPages; // Number of total pages, i.e. Huges + Larges + Smalls.

        UINTPTR VirtAddrBase; // Base virtual address of where direct mapping of system memory starts.
    } DmapInfo;

    KrAddressSpace KernelAddressSpace;
} KrVirtmemmgmtState;

/**
 * @brief Initializes the Virtual Memory Management (VMM) subsystem.
 * 
 * @return TRUE if just initialized, FALSE if initialization failed or the subsystem was already initialized.
 */
BOOL KrInitVirtmemmgmt(VOID);

KrVirtualMemoryRegion* KrLocateVMR(KrAddressSpace* pAddressSpace, UINTPTR Vaddr);
BOOL KrVrangeOverlapsVMR(KrVirtualMemoryRegion* pNode, UINTPTR VaddrStart, UINTPTR VaddrEnd);
BOOL KrVrangeOverlapsAnyVMRs(KrAddressSpace* pAddressSpace, UINTPTR VaddrStart, UINTPTR VaddrEnd);
BOOL KrFindInsertPointVMR(KrAddressSpace* pAddressSpace, UINTPTR VaddrStart, UINTPTR VaddrEnd, KrVirtualMemoryRegion** pBefore, KrVirtualMemoryRegion** pAfter);

KrVirtualMemoryRegion* KrAcquireVMR(KrAddressSpace* pAddressSpace, UINTPTR VaddrStart, UINTPTR VaddrEnd, WORD Flags);

/**
 * @brief Converts a physical conventional memory address to a virtual one the kernel
 * can access any time after `KrInitVirtmemmgmt(VOID)` was called and it had succeeded.
 * This uses the DMAP (Direct Map) set up by the Virtmemmgmt.
 * Given address must belong to a conventional memory page as defined by the `CanonicalMemoryMap`.
 * 
 * @param AddrPhys The physical address to convert.
 * @return Virtual address to `AddrPhys`.
 */
UINTPTR KrPhysToVirt(UINTPTR AddrPhys);

/**
 * @brief Converts a virtual conventional memory d-mapped address to a physical one.
 * This uses the DMAP (Direct Map) set up by the Virtmemmgmt.
 * 
 * @param AddrPhys The virtual address to convert.
 * @return Physical address to `AddrVirt`.
 */
UINTPTR KrVirtToPhys(UINTPTR AddrVirt);

const KrVirtmemmgmtState* KrGetVirtmemmgmtState(VOID);
BOOL KrIsVirtmemmgmtInitialized(VOID);
KrAddressSpace* KrGetKernelAddressSpace(VOID);

#endif // !YCH_KERNEL_MEMORY_VIRTMEMMGMT_H
