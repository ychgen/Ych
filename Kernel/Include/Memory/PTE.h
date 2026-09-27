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

#define PTE_PRESENT  (1UL <<  0)
#define PTE_WRITABLE (1UL <<  1)
#define PTE_USER     (1UL <<  2)
#define PTE_ACCESSED (1UL <<  5)
#define PTE_DIRTY    (1UL <<  6)
#define PTE_GLOBAL   (1UL <<  8)

/** Repurpose for kernel usage later! We are free to use these ourselves for things like CoW or guard page bit for example. */
#define PTE_AVL_0    (1UL <<  9)
#define PTE_AVL_1    (1UL << 10)
#define PTE_AVL_2    (1UL << 11)

#define PTE_NX       (1UL << 63)

// Mask the entirety of any entry with this to get its physical address.
#define PTE_PHYSADDR_MASK 0x000FFFFFFFFFF000UL

typedef QWORD PTE;
typedef PTE* PAGESTRUCT;

typedef enum
{
    VADDR_SMALL,
    VADDR_LARGE,
    VADDR_HUGE
} KrVirtualAddressMode;

typedef struct
{
    UINT PML4;
    UINT PDPT;
    UINT PD;
    UINT PT;
    UINT Offset;
} KrVirtualAddress;

UINTPTR KrMakeVirtual(KrVirtualAddressMode AddressMode, KrVirtualAddress Vidx);
KrVirtualAddress KrUnmakeVirtual(KrVirtualAddressMode AddressMode, UINTPTR Address);

typedef enum
{
    PML4_ENTRY,
    PDPT_ENTRY,
    PDP1GB_ENTRY,
    PD_ENTRY,
    PD2MB_ENTRY,
    PT_ENTRY,

    INVALID_PTE_TYPE = 0xFFFFFFFF
} KrTypePTE;

CSTR KrPteTypeToString(KrTypePTE Type);
KrTypePTE KrGetParentPteType(KrTypePTE Type);
BOOL KrIsLeafPteType(KrTypePTE Type);
QWORD KrGetPteTypeAlignment(KrTypePTE Type);

/**
 * @brief Encodes a page table entry.
 * 
 * @param Type What sort of PTE you are encoding.
 * @param PhysAddrBase The base physical address. It has to be aligned correctly otherwise the function will laugh at you (aka return KR_PTE2_ENCODE_FAILURE_DUE_TO_ALIGNMENT).
 * @param qwBaseFlags Flags QUADWORD. It will be passed through KrMakeFlagsForPTEv2 by this function for sanitization & verification.
 * @param PatSelect Will be passed through KrMakeFlagsForPTEv2 by this function.
 * @return Encoded PTE QUADWORD if successful, error value otherwise.
 */
PTE PteEncodeEntry(KrTypePTE Type, UINTPTR PhysAddrBase, QWORD qwBaseFlags, KrPatSelect PatSelect);

// Gets page struct by reading PTE entry of type ReadType at Index in pContainer and if present converts the physical address to virtual and returns, return NULLPTR if not present.
PAGESTRUCT PteGetPageStruct(PAGESTRUCT pContainer, KrTypePTE ReadType, USHORT Index);

// Same idea as PteGetPageStruct() but if nonpresent, allocates a new physical page to be used as a pagestruct. qwAcqFlags and pslAcq are used if acquisition is made, irrelevant if read.
PAGESTRUCT PteGetOrAcquirePageStruct(PAGESTRUCT pContainer, KrTypePTE ReadType, USHORT Index, QWORD qwAcqFlags, KrPatSelect pslAcq);

#endif // !YCH_KERNEL_MEMORY_PTEV2_H
