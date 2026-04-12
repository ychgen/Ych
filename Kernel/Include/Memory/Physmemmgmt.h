/**
 * 
 * Physmemmgmt is the part of the kernel responsible for managing physical memory.
 * It handles acquisition and releasing of physical pages.
 * Bookkeeping of `conventional` system memory, basically.
 * It is also called PMM or Physical Memory Management.
 * 
 */

#ifndef YCH_KERNEL_MEMORY_PHYSMEMMGMT_H
#define YCH_KERNEL_MEMORY_PHYSMEMMGMT_H

#include "Core/Fundtypes.h"

/* Physical Page ID */
typedef USIZE PAGEID;

#define KR_INVALID_PAGEID ((PAGEID)-1)

#define KR_PHYSICAL_PAGE_STATUS_AVAILABLE   ((BYTE)0)
#define KR_PHYSICAL_PAGE_STATUS_UNAVAILABLE ((BYTE)1)

/// @brief Initializes the PMM (Physical Memory Management).
/// @return TRUE if just initialized, FALSE if it was already initialized or initialization failed.
BOOL KrInitPhysmemmgmt(void);

/// @brief Acquires a physical page.
/// @param idHint Hint for the search algorithm. Will try to find a page near this one.
/// @return Information struct about the acquired physical page.
PAGEID KrAcquirePhysicalPage(PAGEID idHint);

/// @brief Relinquishes a physical page.
/// @param idPage The physical page to release.
BOOL KrRelinquishPhysicalPage(PAGEID idPage);

/// @brief Gets the physical address of a page.
/// @param idPage ID of the page to get the physical address of.
/// @return Physical address of the page.
void* KrGetPhysicalPageAddress(PAGEID idPage);

/// @brief Marks an existing and acquired page as reserved, preventing it from being relinquished.
/// @param idPage Page to reserve.
BOOL  KrReservePhysicalPage(PAGEID idPage);

/// @brief Checks if a physical page was initialized as reserved during Physmemmgmt initialization.
/// @param idPage ID of the page to check.
/// @return FALSE if not reserved, TRUE if reserved.
BOOL  KrIsPhysicalPageReserved(PAGEID idPage);

#endif // !YCH_KERNEL_MEMORY_PHYSMEMMGMT_H
