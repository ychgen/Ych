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

#define KR_DIRECT_MAP_PML4_INDEX          256 // Direct mapping starts at this PML4 index.
#define KR_RECURSIVE_PML4_INDEX           510 // PML4[KR_RECURSIVE_PML4_INDEX] = PHYSICAL_OF(PML4)
#define KR_KERNEL_RESERVED_PML4_INDEX     511

#define KR_KERNEL_PDPT_KERNEL_INDEX       510
#define KR_KERNEL_PDPT_FRAME_BUFFER_INDEX 511

// PROCID
#define KR_VMM_PROCID_INVALID ((UINT) 0)
#define KR_VMM_PROCID_KERNEL  ((UINT) 1)

/**
 * @brief Constructs a virtual address from PML4, PDPT, PD and PT indices and an offset (up to 4KiB, 2MiB or 1GiB depending on the page).
 * If you are referring to a Huge Page (1GiB), leave IndexPD and IndexPT as 0 and your Offset can be larger (up to 1GiB).
 * If you are referring to a Large Page (2MiB), leave IndexPT as 0 and your Offset can be larger (up to 2MiB).
 * If you provide garbage parameters, it's completely your fault, do not blame me.
 * If you provide an Offset larger than the sort of page you want can address, it will bleed into indices and fuck everything up.
 * These are my kindest warnings to you, if you need validation, do checks and stuff before using this macro. I suffered enough writing it.
 */
#define KR_MAKE_VIRTUAL(IndexPML4, IndexPDPT, IndexPD, IndexPT, Offset) \
    ( ( UINTPTR ) ( ((QWORD)((-((QWORD)((IndexPML4) >> 8) & ((QWORD)(1))) & ((QWORD)(0xFFFF000000000000))))) | \
    ( ( ( QWORD ) ( IndexPML4 ) ) & 0x1FF ) << 39 ) | ( ( ( ( QWORD ) ( IndexPDPT ) ) & 0x1FF ) << 30 ) | \
    ( ( ( ( QWORD ) ( IndexPD ) ) & 0x1FF ) << 21 ) | ( ( ( ( QWORD ) ( IndexPT ) ) & 0x1FF ) << 12 ) | \
    ( ( ( ( QWORD ) ( Offset ) ) ) ) )

/** ===================================== */
/** Acquisition types */
/** ===================================== */

/*
 * @brief Will reserve the pages, but they won't be committed until accessed, the first access to each individual page will commit that specific one.
 * This is just purely address space reservation.
 */
#define KR_ACQUIRE_RESERVE 1
// Will make sure there is actual physical backing to the pages.
#define KR_ACQUIRE_COMMIT  2
// Mapped and committed (to the given fixed physical addr) immediately, immutable. Persisent during system uptime, effective until total system reset.
#define KR_ACQUIRE_STATIC  4

/** ===================================== */
/** Various flags for page allocation.    */
/** NOTE: No READ flag. If a page exists, it is readable, period. */
/** ===================================== */

// Pages are readable from.
#define KR_PAGE_FLAG_READ           (1 << 0)
// Pages are writable to.
#define KR_PAGE_FLAG_WRITE          (1 << 1)
// Pages can be executed as if containing code. NOTE: If NX bit is unsupported, ineffective. Check GetVirtmemmgmtState()->bNoExecuteSupport (after init).
#define KR_PAGE_FLAG_EXECUTE        (1 << 2)
// All accesses to the pages are uncacheable and write combining is allowed enabling burst writes. Mutually exclusive with KR_PAGE_FLAG_UNCACHEABLE.
#define KR_PAGE_FLAG_WRITE_COMBINE  (1 << 3)
// Specifies the allocation of large pages of 2MiB.
#define KR_PAGE_FLAG_LARGE          (1 << 4)
// Can only be used with AcquisitionType=RESERVE, it means any access to this page is considered a fatal oopsy daisy (Kernel Meltdown or Process Termination).
#define KR_PAGE_FLAG_GUARD          (1 << 5)
// The processor cannot cache the pages. All read and write operations occur normally with no cache involvement. Mutually exclusive with KR_PAGE_FLAG_WRITE_COMBINE.
#define KR_PAGE_FLAG_UNCACHEABLE    (1 << 6)

/** ===================================== */
/** Relinquishment types */
/** ===================================== */

// Decommits the committed physical pages, but keeping the virtual address space reserved.
#define KR_PAGE_DECOMMIT   1
// Completely relinquishes the virtual address space and any committed physical pages associated.
#define KR_PAGE_RELINQUISH 2

/** ===================================== */
/** Return types */
/** ===================================== */

typedef        DWORD                   KrMapResult;

#define KR_MAP_RESULT_SUCCESS        ((KrMapResult)  0)
#define KR_MAP_RESULT_CONTRADICTION  ((KrMapResult)  1)
#define KR_MAP_RESULT_SPACE_OCCUPIED ((KrMapResult)  2)
#define KR_MAP_RESULT_UNALIGNED      ((KrMapResult)  3)
#define KR_MAP_RESULT_UNIMPLEMENTED  ((KrMapResult)  4)
#define KR_MAP_RESULT_UNPAGED        ((KrMapResult)  5)

/** ===================================== */

typedef struct
{
    /** If TRUE, the processor is capable of 1GiB huge pages. */
    BOOL  bHugePageSupport  : 1;
    /** If TRUE, the processor is capable of pages being NX. */
    BOOL  bNoExecuteSupport : 1;

    /* ================== */
    /* Direct Map Related */
    /* ================== */
        // Total Page Table Entry Structures
        ULONG TotalPTEs;

        ULONG HugePages;  // Number of huge  (1GiB) pages.
        ULONG LargePages; // Number of large (2MiB) pages.
        ULONG SmallPages; // Number of small (4KiB) pages.
        ULONG TotalPages; // Number of total pages, i.e. Huges + Larges + Smalls.

        /** @brief Base virtual address of where direct mapping of system memory starts. */
        UINTPTR VirtAddrDmapBase;
    /* ================== */
    /* DMap Related Sect End... */
    /* ================== */

    /** @brief This one is like a cheat code to edit PTEs directly. Use KrGetAddrOfPTE(). */
    UINTPTR VirtAddrRecursiveBase;

    // Virtual address of the kernel itself.
    UINTPTR VirtAddrKernel;

    // Total number of currently mapped VMR (Virtual Memory Region)s.
    UINT NumVMRs;
} KrVirtmemmgmtState;

// Keep this struct less than or equal to 128 bytes otherwise you will taste a static assertion.
typedef struct KrVirtualMemoryRegion
{
    UINTPTR VirtAddrBase; // Base address of the region, i.e. the starting point
    SIZE    szPageCount;  // Number of pages (size depends on wFlags)
    WORD    wAcquisitionType; // Acquisition manner, RESERVE/COMMIT/STATIC
    WORD    wFlags; // Various flags for the allocated (or to be allocated) pages.
    UINT    uProcID; // 0 = Invalid, 1 = Kernel, anything else = actual Process ID.

    struct KrVirtualMemoryRegion* pPrev; // Pointer to Previous Node
    struct KrVirtualMemoryRegion* pNext; // Pointer to Next Node
} KrVirtualMemoryRegion;

/**
 * @brief Initializes the Virtual Memory Management (VMM) subsystem.
 * 
 * @return TRUE if just initialized, FALSE if initialization failed or the subsystem was already initialized.
 */
BOOL KrInitVirtmemmgmt(VOID);

/**
 * @brief Creates a new virtual address space mapping if possible.
 * 
 * @param uProcID Process ID of the process that this virtual mapping belongs to.
 * @param pAddrVirt The base virtual address to map at. Must be 4 KiB aligned, otherwise will be rejected.
 * @param pAddrPhys The base physical address to map. Must be 4 KiB aligned, otherwise will be rejected.
 * @param szRegionSize The size of the region to map. It is rounded up to the nearest page boundary.
 * @param wAcquisitionType Nerdy stuff to be honest.
 * @param wFlags Some more nerdy stuff.
 * @return Status/result value telling you if it was successful or if not, what went wrong.
 */
KrMapResult KrMapVirt(UINT uProcID, UINTPTR pAddrVirt, UINTPTR pAddrPhys, SIZE szRegionSize, WORD wAcquisitionType, WORD wFlags);

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
const KrVirtualMemoryRegion* KrGetRootVMR(VOID);

#endif // !YCH_KERNEL_MEMORY_VIRTMEMMGMT_H
