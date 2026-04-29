#include "Memory/PrimitiveHeap.h"

#include "Memory/Physmemmgmt.h"
#include "Memory/Virtmemmgmt.h"

typedef struct KR_ALIGNED(8) KrPrimitiveHeapRegion
{
    PAGEID PageID;
    DWORD  Bitmap; // A 4KiB page can have 32 elements of 128 byte size.
    struct KrPrimitiveHeapRegion* pPrev;
    struct KrPrimitiveHeapRegion* pNext;
} KrPrimitiveHeapRegion;

KrPrimitiveHeapRegion g_RootPHR = {
    .PageID = KR_INVALID_PAGEID,
    .Bitmap = 0,
    .pPrev  = NULLPTR,
    .pNext  = NULLPTR
};
KrPrimitiveHeapRegion* g_pTailPHR = NULLPTR;
// TODO: perhaps add a g_pHintPHR to start search from. For now we just start from root always.

BOOL KrPrimitiveInit(VOID)
{
    if (g_RootPHR.PageID != KR_INVALID_PAGEID)
    {
        return FALSE;
    }

    PAGEID idPage = KrAcquirePhysicalPage(KR_INVALID_PAGEID);
    if (idPage == KR_INVALID_PAGEID)
    {
        return FALSE;
    }

    g_RootPHR.PageID = idPage;
    g_RootPHR.Bitmap = 1; // Stepping 0 is always reserved for PHR struct.
    g_RootPHR.pPrev  = NULLPTR;
    g_RootPHR.pNext  = NULLPTR;

    g_pTailPHR = &g_RootPHR;
    return TRUE;
}

VOID* KrPrimitiveAcquire(SIZE szAcquisition)
{
    // Multi-stepping or cross-PHR not allowed for now or for ever.
    if (szAcquisition > KR_PRIMITIVE_HEAP_STEPPING)
    {
        return NULLPTR;
    }

    KrPrimitiveHeapRegion* pRegion = &g_RootPHR;
    while (pRegion)
    {
        if (pRegion->Bitmap == 0xFFFFFFFF)
        {
            pRegion = pRegion->pNext;
            continue;
        }
        
        BYTE FreeStepping = (BYTE) __builtin_ctz(~pRegion->Bitmap);
        pRegion->Bitmap |= (1 << FreeStepping);
        return ((BYTE*) KrPhysToVirt(KrGetPhysicalPageAddress(pRegion->PageID))) + FreeStepping * KR_PRIMITIVE_HEAP_STEPPING;
    }
    if (!pRegion) // No region found?
    {
        PAGEID PageID = KrAcquirePhysicalPage(KR_INVALID_PAGEID);
        if (!PageID)
        {
            return NULLPTR;
        }
        pRegion = (KrPrimitiveHeapRegion*) KrPhysToVirt(KrGetPhysicalPageAddress(PageID));
        pRegion->PageID = PageID;
        pRegion->Bitmap = 3; // Bit 0 and 1 set. 0 for PHR itself, 1 for what we will return.
        pRegion->pPrev  = g_pTailPHR;
        pRegion->pNext  = NULLPTR;

        g_pTailPHR->pNext = pRegion;
        g_pTailPHR = pRegion;

        return ((BYTE*) pRegion) + KR_PRIMITIVE_HEAP_STEPPING; // Return Stepping 1, since Stepping 0 is Region Descriptor
    }

    // unreachable
    return NULLPTR;
}

BOOL KrPrimitiveRelinquish(VOID* pAddr)
{
    const UINT SteppingsPerRegion = KR_PAGE_SIZE / KR_PRIMITIVE_HEAP_STEPPING;

    KrPrimitiveHeapRegion* pRegion = &g_RootPHR;
    while (pRegion)
    {
        UINTPTR VirtAddrRegionBase = KrPhysToVirt(KrGetPhysicalPageAddress(pRegion->PageID));
        UINTPTR VirtAddrRegionEnd  = VirtAddrRegionBase + KR_PAGE_SIZE;

        if ((UINTPTR) pAddr >= VirtAddrRegionBase && (UINTPTR) pAddr < VirtAddrRegionEnd)
        {
            BYTE Stepping = (BYTE)(((UINTPTR) pAddr - VirtAddrRegionBase) / SteppingsPerRegion); // convert to offset and then get the stepping
            if (!(pRegion->Bitmap & (1 << Stepping)))//check if we are being scammed
            {
                return FALSE;
            }
            pRegion->Bitmap &= ~(1 << Stepping);
            return TRUE;
        }

        pRegion = pRegion->pNext;
    }
    return FALSE;
}
