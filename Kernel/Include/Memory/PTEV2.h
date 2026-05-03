/**
 * File Origin Date: `01.05.2026` (DD.MM.YYYY format)
 * 
 * The PTEv2 API is the replacement for the legacy & deprecated PTE API.
 * Newer code should exclusively use this.
 * Older code should gradually and carefully convert to this.
 * 
 * This API also participates in the new DevCheck idea.
 * This API is also public unlike the PTE API which is apart of PrivateVMM.
 */

#ifndef YCH_KERNEL_MEMORY_PTEV2_H
#define YCH_KERNEL_MEMORY_PTEV2_H

#include "Krnlych.h"
#include "CPU/PAT.h"

#define KR_PTE2_ENCODE_FAILURE_DUE_TO_ALIGNMENT (~(0UL)) // QWORD_MAX

#define KR_PAGE_STRUCTURE_ENTRY_COUNT 512
#define KR_PAGE_STRUCTURE_ENTRY_SIZE    8
#define KR_PAGE_STRUCTURE_SIZE       ( KR_PAGE_STRUCTURE_ENTRY_COUNT * KR_PAGE_STRUCTURE_ENTRY_SIZE ) // 512 * 8 = 4 KiB per structure like PML4, PDPT, PD and PT.

#define KR_PTE_PRESENT  (1UL <<  0)
#define KR_PTE_WRITABLE (1UL <<  1)
#define KR_PTE_USER     (1UL <<  2)
#define KR_PTE_ACCESSED (1UL <<  5)
#define KR_PTE_DIRTY    (1UL <<  6)
#define KR_PTE_GLOBAL   (1UL <<  8)

/** Repurpose for kernel usage later! We are free to use these ourselves for things like CoW or guard page bit for example. */
#define KR_PTE_AVL_0    (1UL <<  9)
#define KR_PTE_AVL_1    (1UL << 10)
#define KR_PTE_AVL_2    (1UL << 11)

#define KR_PTE_NX       (1UL << 63)

// Mask the entirety of any entry with this to get its physical address.
#define KR_PTE_PHYSADDR_MASK 0x000FFFFFFFFFF000UL

// Quick reminder for anyone reading: A Page Table Entry is 8 bytes. A paging structure contains 512 entries.
// Childish to reiterate, but you'll like it after starting at paging code for hours.
typedef QWORD PTE;

typedef enum
{
    PML4_ENTRY,
    PDPT_ENTRY,
    PDPT1GB_ENTRY,
    PD_ENTRY,
    PD2MB_ENTRY,
    PT_ENTRY,

    INVALID_PTE_TYPE = 0xFFFFFFFF
} KrTypePTE;
CSTR KrpteTypeToString(KrTypePTE eType);

/**
 * @brief Returns the previous paging structure hierarchy based on a given one.
 * 
 * This means (in `input -> output` format):
 * PML4_ENTRY -> INVALID_PTE_TYPE (we don't support 5-level paging so there is no upper hierarchy of PML4)
 * PDPT_ENTRY OR PDPT1GB_ENTRY -> PML4_ENTRY
 * PD_ENTRY OR PD2MB_ENTRY -> PDPT_ENTRY
 * PT_ENTRY -> PD_ENTRY
 * 
 * @param eType The paging hierarchy to take the one-step upper hierarchy of.
 * @return The one-step upper hierarchy.
 */
KrTypePTE KrpteGetUpperType(KrTypePTE eType);

/**
 * @brief Checks if a given KrTypePTE enum value is a `leaf` type.
 * Leaf means that the PTE entry directly maps to a physical frame, not to another table.
 * 
 * Leaf types are:
 *   -> PDPT1GB_ENTRY
 *   -> PD2MB_ENTRY
 *   -> PT_ENTRY
 * 
 * All other types are not leaf types and are table types.
 * 
 * @param eType The PTE type to check.
 * @return TRUE if leaf type, FALSE otherwise.
 */
BOOL KrpteIsLeafType(KrTypePTE eType);

/**
 * @brief Returns the alignment requirement for a PTE of given type.
 * 
 * @param eType The PTE type.
 * @return The alignment requirement, be it to a physical page or to another PTE. If you want to check for alignent on an address, do `-1` on this and mask the address against it.
 */
QWORD KrpteGetTypeAlignment(KrTypePTE eType);

/**
 * @brief Encodes a page table entry.
 * 
 * @param eType What sort of PTE you are encoding.
 * @param PhysAddrBase The base physical address. It has to be aligned correctly otherwise the function will laugh at you (aka return KR_PTE2_ENCODE_FAILURE_DUE_TO_ALIGNMENT).
 * @param qwBaseFlags Flags QUADWORD. It will be passed through KrMakeFlagsForPTEv2 by this function for sanitization & verification.
 * @param PatSelect Will be passed through KrMakeFlagsForPTEv2 by this function.
 * @return Encoded PTE QUADWORD if successful, error value otherwise.
 */
PTE KrpteEncodeEntry(KrTypePTE eType, UINTPTR PhysAddrBase, QWORD qwBaseFlags, KrPatSelect PatSelect);

#endif // !YCH_KERNEL_MEMORY_PTEV2_H
