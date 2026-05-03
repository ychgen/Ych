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

#include "Krnlych.h"

// Size of `physical` pages. Don't be a fool and use this for anything virtual-related.
#define KR_PAGE_SIZE 4096

/* Physical Page ID. DWORD_MAX * KR_PAGE_SIZE = ~16TiB addressable. Way more than enough in my entire life time probably. */
typedef            DWORD    PAGEID;
// This page ID as always reserved as INVALID.
#define KR_INVALID_PAGEID ((PAGEID) -1)

/** =============================================== */
/** KR_PMM_ACQUIRE_XXX for KrAcquirePhysicalPages() */

// Pages acquired are not guaranteed to be contiguous.
#define KR_PMM_ACQUIRE_SPARSE 1U
// Pages acquired are guaranteed to be contiguous.
#define KR_PMM_ACQUIRE_DENSE  2U

/** =============================================== */

typedef struct
{
    ULONG  TotalPages;    // Total amount of physical pages.
    ULONG  UnusablePages; // Total amount of physical pages that cannot be used for reasons like reserved by the platform, MMIO, kernel reserved etc.
    ULONG  AcquiredPages; // Total amount of physical pages currently acquired and managed by Physmemmgmt.

    PAGEID AcquireHint;   // Current default page acquisition hint.
} KrPhysmemmgmtState;

/**
 * @brief Initializes the PMM (Physical Memory Management) subsystem.
 * 
 * @return TRUE if initialization was successful, FALSE otherwise or if already initialized.
 */
BOOL KrInitPhysmemmgmt(VOID);

/**
 * @brief Acquires a set amount of physical pages.
 * Whether or not the pages are contiguous depends on `dwAcquisitionMethod`.
 * 
 * State of `pOutIDs` post-return of this function is:
 * Index `0` to Index `(NumAcquiredPages i.e. Return Value - 1)` are valid Page IDs to newly-acquired pages.
 * Index `NumAcquiredPages i.e. Return Value` to `dwToAcquire` are set to KR_INVALID_PAGEID.
 * 
 * @param idHint Hint for the search algorithm. Will try to find pages near this one.
 * @param dwAcquisitionMethod Specifies how the allocation should be done. Uses KR_PMM_ACQUIRE_XXX macros.
 * @param pOutIDs Output array to write the acquired page IDs to. Caller is responsible for making sure pOutIDs contains at least `dwToAcquire` element slots.
 * @param uToAcquire The number of pages to acquire.
 * @return The amount of pages actually acquired. The result might be partial. Caller is responsible for handling that.
 */
DWORD KrAcquirePhysicalPages(PAGEID idHint, DWORD dwAcquisitionMethod, PAGEID* pOutIDs, UINT uToAcquire);

/**
 * @brief Acquires a singular physical page. Useful when you genuinely need only one singular physical case.
 * In any case where you need more than one, consider using KrAcquirePhysicalPages(). You can configure it way more in-depth as well.
 * This function internally uses it anyway, asks for 1 page.
 * 
 * @param idHint Hint for the search algorithm. Will try to find a page near this one.
 * @return ID to the acquired page if the acquisition was successful, KR_INVALID_PAGEID otherwise.
 */
PAGEID KrAcquirePhysicalPage(PAGEID idHint);

/**
 * @brief Relinquishes a physical page back to the PMM.
 * 
 * @param PageID ID of the page to relinquish.
 * @return TRUE if the page was relinquished, FALSE if it wasn't (for example trying to relinquish a reserved page).
 */
BOOL KrRelinquishPhysicalPage(PAGEID PageID);

/**
 * @brief Marks an existing and acquired page as reserved, preventing it from being relinquished.
 * Keep in mind that this is a one-way function. This action cannot be reversed. Make sure you have a good reason for it.
 * 
 * @param PageID ID of the page to reserve.
 * @return TRUE if the page got reserved, FALSE otherwise (example: given page is not even acquired at all.).
 */
BOOL  KrReservePhysicalPage(PAGEID PageID);

// You should not use this unless you have a very good reason to. That's why I am not even going to document it.
BOOL  KrSetPhysicalPageAcquisitionHint(PAGEID PageID);

/**
 * @brief Checks if a physical page was initialized as reserved during
 * Physmemmgmt initialization or later explicitly via `KrReservePhysicalPage()`.
 * Reserved pages can never be relinquished, `KrRelinquishPhysicalPage()` will reject and return FALSE.
 * 
 * @param PageID ID of the page to check.
 * @return TRUE if the page is reserved, false otherwise.
 */
BOOL  KrIsPhysicalPageReserved(PAGEID PageID);

/**
 * @brief Gets the physical address of a page.
 * NOTE: Returned address is directly and strictly physical. No virtual or d-map shenanigans by itself.
 * 
 * @param PageID Page ID of the page to get the physical address of.
 * @return Physical address of the page or NULLPTR if the ID is invalid.
 */
UINTPTR KrGetPhysicalPageAddress(PAGEID PageID);

/**
 * @brief Gets the ID of a page from a given physical address.
 * 
 * @param PhysAddr The physical address of the page to get the address of.
 * @return ID the page that contains the given physical address.
 */
PAGEID  KrGetPhysicalPageID(UINTPTR PhysAddr);

CSTR    KrMemoryRegionTypeToString(DWORD dwType);
BOOL    KrIsUsableMemoryRegionType(DWORD dwType);

/**
 * @brief Checks if the PMM subsystem has been initialized.
 * 
 * @return TRUE if initialized, FALSE otherwise.
 */
BOOL    KrIsPhysmemmgmtInitialized(VOID);

/**
 * @brief Gets the current PMM state.
 * 
 * @return Pointer to the state struct if initialized, NULLPTR otherwise.
 */
const KrPhysmemmgmtState* KrGetPhysmemmgmtState(VOID);

#endif // !YCH_KERNEL_MEMORY_PHYSMEMMGMT_H
