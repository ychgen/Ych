#include "Memory/PTEV2.h"

#include "Core/KernelState.h"
#include "Memory/Virtmemmgmt.h"

#define KR_PTE_PWT        (1UL <<  3)
#define KR_PTE_PCD        (1UL <<  4)
#define KR_PTE_NONPT_PS   (1UL <<  7) // For PDPTEs and PDEs, bit 7 is always the PS bit.
#define KR_PTE_PT_PAT     (1UL <<  7) // For PTEs, the last level of paging. Bit 7 is always the PAT bit. 
#define KR_PTE_NONPT_PAT  (1UL << 12) // For PDPTEs and PDEs with PS=1. Bit 7 is always the PS bit so bit 12 is the PAT bit.

CSTR KrpteTypeToString(KrTypePTE eType)
{
    switch (eType)
    {
    case PML4_ENTRY:    return "PML4E";
    case PDPT_ENTRY:    return "PDPTE";
    case PDPT1GB_ENTRY: return "PDP1GBE";
    case PD_ENTRY:      return "PDE";
    case PD2MB_ENTRY:   return "PD2MBE";
    case PT_ENTRY:      return "PTE";
    default: break;
    }
    return "IVLDPTETYPE";
}

KrTypePTE KrpteGetUpperType(KrTypePTE eType)
{
    switch (eType)
    {
    case PML4_ENTRY: return INVALID_PTE_TYPE;
    case PDPT1GB_ENTRY:
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

BOOL KrpteIsLeafType(KrTypePTE eType)
{
    switch (eType)
    {
    case PDPT1GB_ENTRY:
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

QWORD KrpteGetTypeAlignment(KrTypePTE eType)
{
    switch (eType)
    {
    case PML4_ENTRY: // PML4 stores physaddr of PDPTE. That must be 4KiB-aligned.
    case PDPT_ENTRY: // This rule applies to all PTEs that point to other sort of PTEs.
    case PD_ENTRY:   // This too.
    case PT_ENTRY:
    {
        return 0x1000;
    }
    case PDPT1GB_ENTRY:
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

QWORD KrMakeFlagsForPTEv2(KrTypePTE eType, QWORD qwBaseFlags, KrPatSelect PatSelect)
{
    // The problematic ones (7 and 12), we also reset PWT and PCD because they are handled with PatSelect.
    qwBaseFlags &= ~(KR_PTE_PWT | KR_PTE_PCD | (1 << 7) | (1 << 12));

    if (eType == PDPT1GB_ENTRY || eType == PD2MB_ENTRY)
    {
        qwBaseFlags |= KR_PTE_NONPT_PS; // Page Size Bit
    }

    if (PatSelect.PWT)
    {
        qwBaseFlags |= KR_PTE_PWT;
    }
    if (PatSelect.PCD)
    {
        qwBaseFlags |= KR_PTE_PCD;
    }
    if (PatSelect.PAT)
    {
        switch (eType)
        {
        case PDPT1GB_ENTRY:
        case PD2MB_ENTRY:
        {
            qwBaseFlags |= KR_PTE_NONPT_PAT;
            break;
        }
        case PT_ENTRY:
        {
            qwBaseFlags |= KR_PTE_PT_PAT;
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

PTE KrpteEncodeEntry(KrTypePTE eType, UINTPTR PhysAddrBase, QWORD qwFlags, KrPatSelect PatSelect)
{
    const QWORD AlignmentRequirement = KrpteGetTypeAlignment(eType);

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
            qwFlags &= ~(KR_PTE_NX);
        }
    }

    return KrMakeFlagsForPTEv2(eType, qwFlags, PatSelect) | PhysAddrBase;
}
