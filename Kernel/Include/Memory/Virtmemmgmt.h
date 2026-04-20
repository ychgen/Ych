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

/**
 * @brief Initializes the Virtual Memory Management (VMM) subsystem.
 * 
 * @return TRUE if just initialized, FALSE if initialization failed or the subsystem was already initialized.
 */
BOOL KrInitVirtmemmgmt(VOID);

BOOL KrMapVirt(const VOID* pVirt, const VOID* pPhys, SIZE szRegionSize, DWORD dwAcquisitionType, DWORD dwFlags);

/**
 * @brief Acquires a virtual address space, locking it in. Optionally acquires physical backing for those pages if `COMMIT` is specified.
 * 
 * @param pHintAddress Hint to the allocator to acquire near this address.
 * @param szRegionSize Size of the region to acquire. Will be rounded up to the next page boundary.
 * @param dwAcquisitionType Bit field specifying *how* the acquisition should be done. `RESERVE` and `COMMIT` can be combined together.
 * @param dwFlags Bit field describing various flags about the virtual pages to acquire.
 * @return Address to the acquired virtual address space if the acquisition was successful, NULLPTR otherwise.
 */
VOID* KrAcquireVirt(const VOID* pHintAddress, SIZE szRegionSize, DWORD dwAcquisitionType, DWORD dwFlags);

/**
 * @brief Relinquishes previously acquired virtual address space and potentially any committed physical pages if `KR_PAGE_RELINQUISH` is used as the operation.
 * 
 * @param pBaseAddress The base address to start relinquishing from.
 * @param szRegionSize If `dwOperation` is `KR_PAGE_RELINQUISH`, must be 0. If `dwOperation` is `KR_PAGE_DECOMMIT`, all pages spanning across (pBaseAddress + szRegionSize) will be decommitted if non-zero. If non-zero, decommits the entire region, pBaseAddress must be what KrAcquireVirt originally returned.
 * @param dwOperation  Type of relinquishment to proceed with, either `KR_PAGE_DECOMMIT` or `KR_PAGE_RELINQUISH`.
 */
BOOL  KrRelinquishVirt(const VOID* pBaseAddress, SIZE szRegionSize, DWORD dwOperation);

/**
 * @brief Converts a physical memory address to a virtual one, by using the direct map.
 * 
 * @param pAddrPhysical The physical address to convert.
 * @return The virtual address.
 */
VOID* KrPhysicalToVirtual(VOID* pAddrPhysical);

const KrVirtmemmgmtState* GetVirtmemmgmtState(VOID);

#endif // !YCH_KERNEL_MEMORY_VIRTMEMMGMT_H
