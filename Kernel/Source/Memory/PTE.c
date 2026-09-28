#include "Memory/PTE.h"

#include "Core/KernelState.h"
#include "Memory/Physmemmgmt.h"
#include "Memory/Virtmemmgmt.h"

#define PTE_PWT        (1UL <<  3)
#define PTE_PCD        (1UL <<  4)
#define PTE_NONPT_PS   (1UL <<  7) // For PDPTEs and PDEs, bit 7 is always the PS bit.
#define PTE_PT_PAT     (1UL <<  7) // For PTEs, the last level of paging. Bit 7 is always the PAT bit. 
#define PTE_NONPT_PAT  (1UL << 12) // For PDPTEs and PDEs with PS=1. Bit 7 is always the PS bit so bit 12 is the PAT bit.

static QWORD KrMakeFlagsForPTEv2(KrTypePTE eType, QWORD qwBaseFlags, KrPatSelect PatSelect);

PTE PteEncodeEntry(KrTypePTE Type, UINTPTR PhysAddrBase, QWORD qwBaseFlags, KrPatSelect PatSelect)
{
    const QWORD AlignmentRequirement = KrGetPteTypeAlignment(Type);

    // Leftover bits from alignment.
    if (PhysAddrBase & (AlignmentRequirement - 1))
    {
        g_KernelState.DevCheckStats.NumIvldEncodeOfPTEs++;
        return KR_PTE2_ENCODE_FAILURE_DUE_TO_ALIGNMENT;
    }

    if (KrIsVirtmemmgmtInitialized())
    {
        const KrVirtmemmgmtState* pStateVMM = KrGetVirtmemmgmtState();
        if (!pStateVMM->bNoExecuteSupport)
        {
            qwBaseFlags &= ~(PTE_NX);
        }
    }

    return KrMakeFlagsForPTEv2(Type, qwBaseFlags, PatSelect) | PhysAddrBase;
}

PAGESTRUCT PteGetPageStruct(PAGESTRUCT pContainer, KrTypePTE ReadType, USHORT Index)
{
    KR_UNUSED(ReadType);
    if (pContainer[Index] & PTE_PRESENT)
    {
        return (PAGESTRUCT) KrPhysToVirt(pContainer[Index] & PTE_PHYSADDR_MASK);
    }
    return NULLPTR;
}

PAGESTRUCT PteGetOrAcquirePageStruct(PAGESTRUCT pContainer, KrTypePTE ReadType, USHORT Index, QWORD qwAcqFlags, KrPatSelect pslAcq)
{
    if (pContainer[Index] & PTE_PRESENT)
    {
        return (PAGESTRUCT) KrPhysToVirt(pContainer[Index] & PTE_PHYSADDR_MASK);
    }

    PAGEID ID = PmAcquirePage(PAGE_TYPE_PAGE_STRUCT, IVLDPGID);
    if (ID == IVLDPGID)
    {
        return NULLPTR;
    }

    UINTPTR PaddrPs = KrGetPhysicalPageAddress(ID);
    pContainer[Index] = PteEncodeEntry(ReadType, PaddrPs, qwAcqFlags, pslAcq);
    if (pContainer[Index] == KR_PTE2_ENCODE_FAILURE_DUE_TO_ALIGNMENT)
    {
        PmRelinquishPage(ID);
        return NULLPTR;
    }
    return (PAGESTRUCT) KrPhysToVirt(PaddrPs);
}

UINTPTR KrMakeVirtual(KrVirtualAddressMode AddressMode, KrVirtualAddress Vidx)
{
    UINTPTR Address = 0;

    if (Vidx.PML4 & (1 << 9)) // sign extension
    {
        Address = 0xFFFF000000000000;
    }

    Address |= ((QWORD)(Vidx.PML4 & 0x1FF)) << 39;
    Address |= ((QWORD)(Vidx.PDPT & 0x1FF)) << 30;

    switch (AddressMode)
    {
    case VADDR_HUGE:
    {
        Address |= ((QWORD)(Vidx.Offset & 0x3FFFFFFF));
        break;
    }
    case VADDR_LARGE:
    {
        Address |= ((QWORD)(Vidx.PD & 0x1FF)) << 21;
        Address |= ((QWORD)(Vidx.Offset & 0x1FFFFF)) << 12;
        break;
    }
    case VADDR_SMALL:
    {
        Address |= ((QWORD)(Vidx.PD & 0x1FF)) << 21;
        Address |= ((QWORD)(Vidx.PT & 0x1FF)) << 12;
        Address |= ((QWORD)(Vidx.Offset) & 0xFFF);
        break;
    }
    }

    return Address;
}

KrVirtualAddress KrUnmakeVirtual(KrVirtualAddressMode AddressMode, UINTPTR Address)
{
    KrVirtualAddress Result = {0};
    Result.PML4 = (Address >> 39) & 0x1FF;
    Result.PDPT = (Address >> 30) & 0x1FF;

    switch (AddressMode)
    {
    case VADDR_SMALL:
    {
        Result.PD     = (Address >> 21) & 0x1FF;
        Result.PT     = (Address >> 12) & 0x1FF;
        Result.Offset = (Address >> 00) & 0xFFF;
        break;
    }
    case VADDR_LARGE:
    {
        Result.PD     = (Address >> 21) & 0x1FF;
        Result.Offset = (Address >> 12) & 0x1FFFFF;

        Result.PT = 0;
        break;
    }
    case VADDR_HUGE:
    {
        Result.Offset = (Address >> 21) & 0x3FFFFFFF;
        break;
    }
    }

    return Result;
}

CSTR KrPteTypeToString(KrTypePTE Type)
{
    switch (Type)
    {
    case PML4_ENTRY:    return "PML4E";
    case PDPT_ENTRY:    return "PDPTE";
    case PDP1GB_ENTRY: return "PDP1GBE";
    case PD_ENTRY:      return "PDE";
    case PD2MB_ENTRY:   return "PD2MBE";
    case PT_ENTRY:      return "PTE";
    default: break;
    }
    return "IVLDPTETYPE";
}

KrTypePTE KrGetParentPteType(KrTypePTE Type)
{
    switch (Type)
    {
    case PML4_ENTRY: return INVALID_PTE_TYPE;
    case PDP1GB_ENTRY:
    case PDPT_ENTRY:
        return PML4_ENTRY;
    case PD2MB_ENTRY:
    case PD_ENTRY:
        return PDPT_ENTRY;
    case PT_ENTRY: return PD_ENTRY;
    default: break;
    }
    return INVALID_PTE_TYPE;
}

BOOL KrIsLeafPteType(KrTypePTE Type)
{
    switch (Type)
    {
    case PDP1GB_ENTRY:
    case PD2MB_ENTRY:
    case PT_ENTRY:
    {
        return TRUE;
    }
    default:
    {
        break;
    }
    }

    return FALSE;
}

QWORD KrGetPteTypeAlignment(KrTypePTE Type)
{
    switch (Type)
    {
    case PML4_ENTRY: // PML4 stores physaddr of PDPTE. That must be 4KiB-aligned.
    case PDPT_ENTRY: // This rule applies to all PTEs that point to other sort of PTEs.
    case PD_ENTRY:   // This too.
    case PT_ENTRY:
    {
        return 0x1000;
    }
    case PDP1GB_ENTRY:
    {
        return 0x40000000;
    }
    case PD2MB_ENTRY:
    {
        return 0x200000;
    }
    default: break;
    }
    return 0;
}

static QWORD KrMakeFlagsForPTEv2(KrTypePTE eType, QWORD qwBaseFlags, KrPatSelect PatSelect)
{
    // The problematic ones (7 and 12), we also reset PWT and PCD because they are handled with PatSelect.
    qwBaseFlags &= ~(PTE_PWT | PTE_PCD | (1 << 7) | (1 << 12));

    if (eType == PDP1GB_ENTRY || eType == PD2MB_ENTRY)
    {
        qwBaseFlags |= PTE_NONPT_PS; // Page Size Bit
    }

    if (PatSelect.PWT)
    {
        qwBaseFlags |= PTE_PWT;
    }
    if (PatSelect.PCD)
    {
        qwBaseFlags |= PTE_PCD;
    }
    if (PatSelect.PAT)
    {
        switch (eType)
        {
        case PDP1GB_ENTRY:
        case PD2MB_ENTRY:
        {
            qwBaseFlags |= PTE_NONPT_PAT;
            break;
        }
        case PT_ENTRY:
        {
            qwBaseFlags |= PTE_PT_PAT;
            break;
        }
        default:
        {
            break; // PAT unsupported in this paging hierarchy
        }
        }
    }

    return qwBaseFlags;
}
