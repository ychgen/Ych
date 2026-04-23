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

/**
 * @brief Constructs a virtual address from PML4, PDPT, PD and PT indices and an offset (up to 4KiB, 2MiB or 1GiB depending on the page).
 * If you are referring to a Huge Page (1GiB), leave IndexPD and IndexPT as 0 and your Offset can be larger (up to 1GiB).
 * If you are referring to a Large Page (2MiB), leave IndexPT as 0 and your Offset can be larger (up to 2MiB).
 */
#define KR_MAKE_VIRTUAL(IndexPML4, IndexPDPT, IndexPD, IndexPT, Offset) \
    ( ( UINTPTR ) ( ((QWORD)((-((QWORD)((IndexPML4) >> 8) & ((QWORD)(1))) & ((QWORD)(0xFFFF000000000000))))) | ( ( ( QWORD ) ( IndexPML4 ) ) & 0x1FF ) << 39 ) | ( ( ( ( QWORD ) ( IndexPDPT ) ) & 0x1FF ) << 30 ) | ( ( ( ( QWORD ) ( IndexPD ) ) & 0x1FF ) << 21 ) | ( ( ( ( QWORD ) ( IndexPT ) ) & 0x1FF ) << 12 ) | ( ( ( ( QWORD ) ( Offset ) ) ) ) )

/** ===================================== */
/** Allocation types */
/** ===================================== */

/*
 * @brief Will reserve the pages, but they won't be committed until accessed, the first access to each individual page will commit that specific one.
 * This is just purely address space reservation.
 */
#define KR_PAGE_ALLOCATE_RESERVE   (1 << 0)
// Will make sure there is actual physical backing to the pages.
#define KR_PAGE_ALLOCATE_COMMIT    (1 << 1)

/** ===================================== */
/** Various flags for page allocation.    */
/** NOTE: No READ flag. If a page exists, it is readable, period. */
/** ===================================== */

// Pages are writable to.
#define KR_PAGE_FLAG_WRITE         (1 << 0)
// Pages can be executed as if containing code.
#define KR_PAGE_FLAG_EXECUTE       (1 << 1)
// All accesses to the pages are uncacheable and write combining is allowed enabling burst writes.
#define KR_PAGE_FLAG_WRITE_COMBINE (1 << 2)
// Specifies the allocation of a large page of 2MiB. KrAcquireVirt will return FALSE if processor doesn't support large pages.
#define KR_PAGE_FLAG_LARGE_PAGE    (1 << 3)

/** ===================================== */
/** Relinquishment types */
/** ===================================== */

// Decommitts the committed physical pages, but keeping the virtual address space reserved.
#define KR_PAGE_DECOMMIT   1
// Completely relinquishes the virtual address space and any committed physical pages associated.
#define KR_PAGE_RELINQUISH 2

/** ===================================== */

typedef struct
{
    /** If TRUE, the processor is capable of 1GiB huge pages. */
    BOOL  bHugePageSupport  : 1;
    /** If TRUE, the processor is capable of pages being NX. */
    BOOL  bNoExecuteSupport : 1;

    // Total Page Table Entry Structures
    ULONG TotalPTEs;

    ULONG HugePages;  // Number of huge pages.
    ULONG LargePages; // Number of large pages.
    ULONG SmallPages; // Number of small pages.
    ULONG TotalPages; // Number of total pages, i.e. Huges + Larges + Smalls.

    /** @brief Base virtual address of where direct mapping of system memory starts. */
    UINTPTR AddrDirectMapBase;
    /** @brief This one is like a cheat code to edit PTEs directly. */
    UINTPTR AddrRecursiveMapBase;

    // Total number of currently acquired VMR (Virtual Memory Region)s.
    UINT NumVMRs;
} KrVirtmemmgmtState;

typedef struct KrVirtualMemoryRegion
{
    UINTPTR VirtAddrBase;
    SIZE    szPageCount;
    WORD    wAcquisitionType;
    WORD    wFlags;

    struct KrVirtualMemoryRegion* pPrev; // Previous Node
    struct KrVirtualMemoryRegion* pNext; // Next Node
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
 * @param pAddrVirt The base virtual address to map at. Must be 4 KiB aligned, otherwise will be rejected.
 * @param pAddrPhys The base physical address to map. Must be 4 KiB aligned, otherwise will be rejected.
 * @param szRegionSize The size of the region to map. It is rounded up to the nearest page boundary.
 * @param wAcquisitionType Nerdy stuff to be honest.
 * @param wFlags Some more nerdy stuff.
 * @return TRUE if the mapping was created, FALSE if not.
 */
BOOL KrMapVirt(UINTPTR pAddrVirt, UINTPTR pAddrPhys, SIZE szRegionSize, WORD wAcquisitionType, WORD wFlags);

UINTPTR KrPhysToVirt(UINTPTR AddrPhys);

const KrVirtmemmgmtState* GetVirtmemmgmtState(VOID);

#endif // !YCH_KERNEL_MEMORY_VIRTMEMMGMT_H
