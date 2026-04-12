/**
 * 
 * Physmemmgmt is the part of the kernel responsible for managing virtual memory.
 * It handles acquisition and releasing of virtual addresses and mapping.
 * Bookkeeping of the virtual memory, basically.
 * It is also called VMM or Virtual Memory Management.
 * 
 */

#ifndef YCH_KERNEL_MEMORY_VIRTMEMMGMT_H
#define YCH_KERNEL_MEMORY_VIRTMEMMGMT_H

#include "Core/Fundtypes.h"

/** ===================================== */
/** Allocation types */
/** ===================================== */
// Will reserve the pages, but they won't be committed until accessed, the first access to each individual page will commit it.
#define KR_PAGE_ALLOCATE_RESERVE   (1 << 0)
// Will reserve and commit each one of the pages during acquire phase.
#define KR_PAGE_ALLOCATE_COMMIT    (1 << 1)
/** ===================================== */
/** Various flags for page allocation.    */
/** ===================================== */
// Pages are writable to.
#define KR_PAGE_FLAG_WRITE         (1 << 0)
// Pages can be executed as if containing code.
#define KR_PAGE_FLAG_EXECUTE       (1 << 1)
// All accesses to the pages are uncacheable and write combining is allowed enabling burst writes.
#define KR_PAGE_FLAG_WRITE_COMBINE (1 << 2)
// Specifies the allocation of a large page. KrAcquireVirt will return FALSE if processor doesn't support large pages.
#define KR_PAGE_FLAG_LARGE_PAGE    (1 << 3)
/** ===================================== */

/// @brief Acquires virtual address space and optionally physical memory to back that address space if `COMMIT` is specified.
/// @param pHint Hint address, if possible, the acquisition will happen near this virtual address. Leave as NULLPTR to let the system decide.
/// @param szRegionSize Size of the region to acquire. If not page-aligned, it will be rounded up to the next page boundary.
/// @param dwAllocationType Bit field of how the allocation will proceed, uses KR_PAGE_ALLOCATE macros.
/// @param dwFlags Various flags bit field, uses KR_PAGE_FLAG_WRITE macros and combos.
/// @return Address to the acquired region. NULLPTR if the acquisition failed.
void* KrAcquireVirt(const void* pHint, USIZE szRegionSize, DWORD dwAllocationType, DWORD dwFlags);

/// @brief Relinquishes a previously allocated virtual address space, making it available for use again.
/// @param pAddr Pointer to the region to release.
/// @return TRUE if success, FALSE if failure or NULLPTR.
BOOL KrRelinquishVirt(const void* pAddr);

#endif // !YCH_KERNEL_MEMORY_VIRTMEMMGMT_H
