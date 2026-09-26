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

typedef            DWORD    PAGEID; /* Physical Page ID. DWORD_MAX * KR_PAGE_SIZE = ~16TiB addressable. Way more than enough in my entire life time probably. */
#define IVLDPGID ((PAGEID) -1) // This page ID as always reserved as INVALID.

#define PAGE_ACQ_SPARSE         (1 << 0) // Pages acquired are not guaranteed to be contiguous.
#define PAGE_ACQ_DENSE          (1 << 1) // Pages acquired are guaranteed to be contiguous.
#define PAGE_ACQ_BASE_OUT_ONLY  (1 << 2) // Only valid with PAGE_ACQ_DENSE. Modifies behavior so that only the starting page ID is written to *pOutIDs.
#define PAGE_ACQ_FOR_USER       (1 << 3)

#define PAGE_TYPE_INVALID     0
#define PAGE_TYPE_GENERAL     1 // General purpose allocation
#define PAGE_TYPE_PAGE_STRUCT 2 // Paging level structure like PDPT structure, PD structure, PT structure.
#define PAGE_TYPE_RNA         3 // Is a `Region Node Allocator` page containing RNA pages storing KrVirtualMemoryRegion nodes.
#define PAGE_TYPE_BOOKKEEPING 4 // Contains KrPhysicalPageMeta[] structures
#define PAGE_TYPE_RESERVED    5 // Page reserved before PMM bringup completion

#define PAGE_FLAG_PINNED (1 << 0)
#define PAGE_FLAG_USER   (1 << 1) // Page is user-level, non-kernel acquired.

typedef struct // Never mark this struct as packed! Also keep it as small as humanly possible.
{
    WORD RefCount;
    union
    {
        // 16 bit field whose value depends on Meta.Type
        WORD Auxiliary;
    };
    BYTE Flags;
    BYTE Type;
} KrPhysicalPageMeta;

typedef enum
{
    PMM_INIT_STAGE_NONE        = 0,
    PMM_INIT_STAGE_BASIC       = 1,
    PMM_INIT_STAGE_BOOKKEEPING = 2,

    PMM_INIT_STAGE_FULLY_OPERATIONAL = PMM_INIT_STAGE_BOOKKEEPING
} PmmInitStage;

typedef struct
{
    PmmInitStage InitStage;

    struct
    {
        UINTPTR PaddrMetaArray;
        UINTPTR VaddrMetaArray;

        UINTPTR PaddrHighest;
        BYTE* pAdvisoryBitmap;
        BYTE* pPrimaryBitmap;
        SIZE  BitmapSize;
        UINT  NrPages;
    } Private;
    
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
 * @brief Initializes the `struct KrPhysicalPageMeta[]` array.
 * Must be called after PMM initialization and VMM d-map initialization.
 * 
 * @return TRUE if successful, FALSE otherwise.
 */
BOOL KrInitPhysMetaArray(VOID);

/**
 * @brief Acquires a set amount of physical pages.
 * Whether or not the pages are contiguous depends on `dwAcquisitionMethod`.
 * 
 * State of `pOutIDs` post-return of this function is:
 * Range `0` to `(NumAcquiredPages i.e. Return Value - 1)` is valid Page IDs to newly-acquired pages.
 * Range `NumAcquiredPages i.e. Return Value` to `dwToAcquire - 1` is set to IVLDPGID.
 * 
 * @param pOutIDs Output array to write the acquired page IDs to. Caller is responsible for making sure pOutIDs contains at least `dwToAcquire` element slots UNLESS base-out-only mode where function only uses 1 slot.
 * @param uToAcquire The number of pages to acquire.
 * @param dwAcquisitionMethod Specifies how the allocation should be done. Uses KR_PMM_ACQUIRE_XXX macros.
 * @param HintID Hint for the search algorithm. Will try to find pages near this one.
 * @return The amount of pages actually acquired. The result might be partial. Caller is responsible for handling that.
 */
DWORD KrAcquirePhysicalPages(PAGEID* pOutIDs, UINT uToAcquire, BYTE PageType, DWORD dwAcquisitionMethod, PAGEID HintID);

/**
 * @brief Acquires a singular physical page. Useful when you genuinely need only one singular physical case.
 * In any case where you need more than one, consider using KrAcquirePhysicalPages(). You can configure it way more in-depth as well.
 * This function internally uses it anyway, asks for 1 page.
 * 
 * @param HintID Hint for the search algorithm. Will try to find a page near this one.
 * @return ID to the acquired page if the acquisition was successful, IVLDPGID otherwise.
 */
PAGEID KrAcquirePhysicalPage(BYTE PageType, PAGEID HintID);

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

KrPhysicalPageMeta* KrGetPageMeta(PAGEID PageID);

CSTR    KrMemoryRegionTypeToString(DWORD dwType);
BOOL    KrIsUsableMemoryRegionType(DWORD dwType);

/**
 * @brief Gets the current PMM state.
 * 
 * @return Pointer to the state struct if initialized, NULLPTR otherwise.
 */
const KrPhysmemmgmtState* KrGetPhysmemmgmtState(VOID);

#endif // !YCH_KERNEL_MEMORY_PHYSMEMMGMT_H
