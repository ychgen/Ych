#ifndef YCH_KERNEL_MEMORY_PRIVATE_VMM_REGION_NODE_ALLOCATOR_H
#define YCH_KERNEL_MEMORY_PRIVATE_VMM_REGION_NODE_ALLOCATOR_H

#include "Memory/Physmemmgmt.h"
#include "Memory/Virtmemmgmt.h"

#include "KRTL/Krnlmem.h"

#define RNA_SLOT_SIZE 64
#define RNA_NUMBER_OF_SLOTS (KR_PAGE_SIZE / RNA_SLOT_SIZE)

typedef struct
{
    QWORD  Bitmap; // Slot occupency bitmap
    PAGEID This;
    PAGEID Prev;
    PAGEID Next;
    UCHAR  NrSlots; // Number of slots occupied, this must always equal popcnt(Bitmap). Value of 1 means only meta slot exists, making the slot redundant
} KrRnaMetaSlot;

KR_STATIC_ASSERT(sizeof(KrRnaMetaSlot) <= RNA_SLOT_SIZE, "Rna Meta Slot exceeds slot size!");

PAGEID g_RnaRootPageID = IVLDPGID;

static VOID* RnaGetSlotVptr(PAGEID PageID, UCHAR SlotID);
static KrRnaMetaSlot* RnaGetMetaSlotVptr(PAGEID PageID);

static BOOL RnaInit(VOID)
{
    if (g_RnaRootPageID != IVLDPGID)
    {
        return FALSE;
    }

    g_RnaRootPageID = KrAcquirePhysicalPage(PAGE_TYPE_RNA, IVLDPGID);
    if (g_RnaRootPageID == IVLDPGID)
    {
        return FALSE;
    }
    KrRnaMetaSlot* pRootMeta = RnaGetMetaSlotVptr(g_RnaRootPageID);
    KrtlContiguousZeroBuffer(pRootMeta, KR_PAGE_SIZE);
    KrReservePhysicalPage(g_RnaRootPageID); // make the root reserved as we will never be relinquishing it

    pRootMeta->This    = g_RnaRootPageID;
    pRootMeta->Prev    = IVLDPGID;
    pRootMeta->Next    = IVLDPGID;
    pRootMeta->Bitmap  = 1;
    pRootMeta->NrSlots = 1;

    return TRUE;
}

static KrVirtualMemoryRegion* RnaAcquireNode(VOID)
{
    KrRnaMetaSlot* pChosen = RnaGetMetaSlotVptr(g_RnaRootPageID);

    for (;;)
    {
        if (pChosen->Bitmap != QWORD_MAX)
        {
            break;
        }
        if (pChosen->Next != IVLDPGID)
        {
            pChosen = RnaGetMetaSlotVptr(pChosen->Next);
            continue;
        }

        PAGEID ID = KrAcquirePhysicalPage(PAGE_TYPE_RNA, IVLDPGID);
        if (ID == IVLDPGID)
        {
            return (KrVirtualMemoryRegion*) NULLPTR;
        }
        KrRnaMetaSlot* pNew = RnaGetMetaSlotVptr(ID);
        KrtlContiguousZeroBuffer(pNew, KR_PAGE_SIZE);

        pNew->Prev = pChosen->This;
        pChosen->Next = ID;
        pNew->Next = IVLDPGID;
        pNew->Bitmap = 1;
        pNew->NrSlots = 1;

        pChosen = pNew;
        break;
    }

    UCHAR FreeSlot = __builtin_ctzl(~pChosen->Bitmap);
    pChosen->Bitmap |= (1 << FreeSlot);
    pChosen->NrSlots++;

    return (KrVirtualMemoryRegion*) RnaGetSlotVptr(pChosen->This, FreeSlot);
}

static BOOL RnaRelinquishNode(KrVirtualMemoryRegion* pNode)
{
    const UINTPTR VaddrNode = (UINTPTR) pNode;
    PAGEID SearchID = g_RnaRootPageID;

    do
    {
        KrRnaMetaSlot* pMeta = RnaGetMetaSlotVptr(SearchID);

        const UINTPTR VaddrStart = (UINTPTR) pMeta;
        const UINTPTR VaddrEnd   = VaddrStart + KR_PAGE_SIZE;

        if (VaddrNode >= VaddrStart && VaddrNode < VaddrEnd)
        {
            break;
        }

        SearchID = pMeta->Next;
    } while (SearchID != IVLDPGID);

    if (SearchID == IVLDPGID)
    {
        return FALSE;
    }

    KrRnaMetaSlot* pMeta = RnaGetMetaSlotVptr(SearchID);
    const PTRDIFF OffsetIntoPage = VaddrNode - (UINTPTR) pMeta;
    const UCHAR WhichSlot = OffsetIntoPage / RNA_SLOT_SIZE;

    pMeta->Bitmap &= ~(1 << WhichSlot);
    if (--pMeta->NrSlots == 1)
    {
        if (pMeta->Prev != IVLDPGID)
        {
            RnaGetMetaSlotVptr(pMeta->Prev)->Next = pMeta->Next;
        }
        if (pMeta->Next != IVLDPGID)
        {
            RnaGetMetaSlotVptr(pMeta->Next)->Prev = pMeta->Prev;
        }
    }
    KrRelinquishPhysicalPage(pMeta->This);

    return TRUE;
}

static VOID* RnaGetSlotVptr(PAGEID PageID, UCHAR SlotID)
{
    return (VOID*)(((UCHAR*) KrPhysToVirt(KrGetPhysicalPageAddress(PageID))) + (SlotID * RNA_SLOT_SIZE));
}

static KrRnaMetaSlot* RnaGetMetaSlotVptr(PAGEID PageID)
{
    return (KrRnaMetaSlot*) RnaGetSlotVptr(PageID, 0);
}

#endif // !YCH_KERNEL_MEMORY_PRIVATE_VMM_REGION_NODE_ALLOCATOR_H
