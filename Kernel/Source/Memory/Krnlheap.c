#include "Memory/Krnlheap.h"

#include "Memory/Virtmemmgmt.h"
#include "KRTL/Krnlmem.h"

#include "Core/KernelState.h"
#include "Earlyvideo/DisplaywideTextProtocol.h"

typedef WORD offset_to_meta_t;
#define MAX_offset_to_meta (~((offset_to_meta_t) 0))

#define HEAP_MIN_PAYLOAD_SIZE 32

typedef struct KR_PACKED HeapHeader
{
    struct HeapHeader* pPrev;
    struct HeapHeader* pNext;
    SIZE PayloadSize;
    UCHAR bFree;
} HeapHeader;

static KrVirtualMemoryRegion* HeapVMR = NULLPTR;
static HeapHeader* RootHeader = NULLPTR;
static UINTPTR VaddrHeapSt = 0;
static UINTPTR VaddrHeapNd = 0;

UINTPTR HeapHeaderPayloadAddress(HeapHeader* pHeader);

HeapHeader* HeapLocateFreeFrom(HeapHeader* pRef);
HeapHeader* HeapLocateFreeOfSizeFrom(HeapHeader* pRef, SIZE N);
HeapHeader* HeapLocateAlignedFreeOfSizeFrom(HeapHeader* pRef, SIZE N, UINT Alignment, UINTPTR* pOutAlignedPayload);

BOOL HeapCanSplitAt(HeapHeader* pRef, UINTPTR Offset);
BOOL HeapTrySplitAt(HeapHeader* pRef, UINTPTR Offset);

BOOL HeapInit(VOID)
{
    static BOOL s_HeapInitialized = FALSE;

    if (s_HeapInitialized)
    {
        return FALSE;
    }

    // The kernel heap spans from PML4 first entry up to MMIO entry.
    VaddrHeapSt = KR_MAKE_VIRTUAL(KRNL_PML4_IDX, KRNL_HEAP_PDPT_IDX, 0, 0, 0);
    VaddrHeapNd = VaddrHeapSt + (KR_MAKE_VIRTUAL(KRNL_PML4_IDX, KRNL_MMIO_PDPT_IDX, 0, 0, 0) - VaddrHeapSt);

    HeapVMR = VmAcquireRegion(KrGetKernelAddressSpace(), VaddrHeapSt, VaddrHeapNd, VMR_FLAG_READABLE | VMR_FLAG_WRITABLE);
    if (!HeapVMR)
    {
        return FALSE;
    }
    VmTagRegion(HeapVMR, "Kernel Heap");
    
    RootHeader = (HeapHeader*) VaddrHeapSt;
    RootHeader->pPrev = RootHeader->pNext = NULLPTR;
    RootHeader->PayloadSize = VaddrHeapNd - VaddrHeapSt - sizeof(HeapHeader);
    RootHeader->bFree = TRUE;
    
    s_HeapInitialized = TRUE;
    return TRUE;
}

VOID* HeapAlignedReAcquire(VOID* Ptr, SIZE N, UINT Alignment)
{
    return NULLPTR; // TODO
}

VOID* HeapAlignedAcquire(SIZE N, UINT Alignment)
{
    if (N == 0 || Alignment == 0 || (Alignment & (Alignment - 1)))
    {
        return NULLPTR;
    }

    if (N < HEAP_MIN_PAYLOAD_SIZE)
    {
        N = HEAP_MIN_PAYLOAD_SIZE;
    }
    N = KrtlAlignUpToPowerOfTwo(N, Alignment);
    
    UINTPTR AddrAlignedPayload;
    HeapHeader* pHeader = HeapLocateAlignedFreeOfSizeFrom(RootHeader, N, Alignment, &AddrAlignedPayload);
    
    if (!pHeader)
    {
        return NULLPTR;
    }

    SIZE ResultDistToHeader = AddrAlignedPayload - (UINTPTR) pHeader;
    if (ResultDistToHeader > (SIZE) MAX_offset_to_meta)
    {
        return NULLPTR;
    }

    *(offset_to_meta_t*)(AddrAlignedPayload - sizeof(offset_to_meta_t)) = (offset_to_meta_t) ResultDistToHeader;
    HeapTrySplitAt(pHeader, (AddrAlignedPayload + N) - (UINTPTR) pHeader);
    
    pHeader->bFree = FALSE;
    return (VOID*) AddrAlignedPayload;
}

VOID* HeapReAcquire(VOID* Ptr, SIZE N)
{
    return HeapAlignedReAcquire(Ptr, N, HEAP_DEFAULT_ALIGNMENT);
}

VOID* HeapAcquire(SIZE N)
{
    return HeapAlignedAcquire(N, HEAP_DEFAULT_ALIGNMENT);
}

BOOL HeapRelinquish(VOID* Ptr)
{
    UINTPTR AddrPtr = (UINTPTR) Ptr;
    if (AddrPtr < VaddrHeapSt || AddrPtr >= VaddrHeapNd) // This doesnt even come from possible heap space bro
    {
        return FALSE;
    }

    offset_to_meta_t OffsetToHeader = *(offset_to_meta_t*)(AddrPtr - sizeof(offset_to_meta_t));
    HeapHeader* pHeader = (HeapHeader*)(AddrPtr - OffsetToHeader);

    if (pHeader->bFree)
    {
        return FALSE; // nonsensical, called with invalid ptr value
    }
    pHeader->bFree = TRUE;

    HeapHeader* pMyPrev = pHeader->pPrev;
    HeapHeader* pMyNext = pHeader->pNext;
    
    // Previous region swallows this
    if (pMyPrev && pMyPrev->bFree)
    {
        pMyPrev->PayloadSize += pHeader->PayloadSize + sizeof(HeapHeader);
        pMyPrev->pNext = pMyNext;
        pMyNext->pPrev = pMyPrev;

        pHeader = pMyPrev; // Important!
    }
    // (Either previous region or this region) swallows the region after the initial region's region
    if (pMyNext && pMyNext->bFree)
    {
        pHeader->PayloadSize += pMyNext->PayloadSize + sizeof(HeapHeader);
        pHeader->pNext = pMyNext->pNext;
        if (pMyNext->pNext)
        {
            pMyNext->pNext->pPrev = pHeader;
        }
    }

    return TRUE;
}

UINTPTR HeapHeaderPayloadAddress(HeapHeader* pHeader)
{
    return ((UINTPTR) pHeader) + sizeof(HeapHeader);
}

HeapHeader* HeapLocateFreeFrom(HeapHeader* pRef)
{
    while (pRef)
    {
        if (pRef->bFree)
        {
            return pRef;
        }
        pRef = pRef->pNext;
    }
    return NULLPTR;
}

HeapHeader* HeapLocateFreeOfSizeFrom(HeapHeader* pRef, SIZE N)
{
    while (pRef)
    {
        if (pRef->bFree && pRef->PayloadSize >= N)
        {
            return pRef;
        }
        pRef = pRef->pNext;
    }
    return NULLPTR;
}

HeapHeader* HeapLocateAlignedFreeOfSizeFrom(HeapHeader* pRef, SIZE N, UINT Alignment, UINTPTR* pOutAlignedPayload)
{
    while (pRef)
    {
        if (pRef->bFree)
        {
            UINTPTR AddrPayloadOriginal = HeapHeaderPayloadAddress(pRef);
            UINTPTR AddrPayloadAligned  = KrtlAlignUpToPowerOfTwo(AddrPayloadOriginal, Alignment);

            SIZE ErrorTolerance = AddrPayloadAligned - AddrPayloadOriginal;
            if (ErrorTolerance < sizeof(offset_to_meta_t))
            {
                AddrPayloadAligned += sizeof(offset_to_meta_t) - ErrorTolerance;
                AddrPayloadAligned  = KrtlAlignUpToPowerOfTwo(AddrPayloadAligned, Alignment);
                ErrorTolerance = AddrPayloadAligned - AddrPayloadOriginal;
            }

            if (pRef->PayloadSize - ErrorTolerance >= N)
            {
                if (pOutAlignedPayload)
                {
                    *pOutAlignedPayload = AddrPayloadAligned;
                }
                return pRef;
            }
        }
        pRef = pRef->pNext;
    }
    return NULLPTR;
}

BOOL HeapCanSplitAt(HeapHeader* pRef, UINTPTR Offset)
{
    const UINTPTR AddrRef = (UINTPTR) pRef;
    const UINTPTR AddrAbsNd = AddrRef + sizeof(HeapHeader) + pRef->PayloadSize;
    
    if (Offset > AddrAbsNd - AddrRef)
    {
        return FALSE;
    }

    const UINTPTR AddrInTrench = AddrRef + Offset;
    const SIZE Difference = AddrAbsNd - AddrInTrench;

    return Difference >= HEAP_MIN_PAYLOAD_SIZE + sizeof(HeapHeader);
}

BOOL HeapTrySplitAt(HeapHeader* pRef, UINTPTR Offset)
{
    if (!HeapCanSplitAt(pRef, Offset))
    {
        return FALSE;
    }
    
    const UINTPTR AddrAbsNd = (UINTPTR) pRef + sizeof(HeapHeader) + pRef->PayloadSize;
    const UINTPTR AddrInto = (UINTPTR) pRef + Offset;

    const SIZE EndToCutDiff = AddrAbsNd - AddrInto;

    HeapHeader* pInsert = (HeapHeader*) AddrInto;
    pInsert->pPrev = pRef;
    pInsert->pNext = pRef->pNext;
    pInsert->bFree = TRUE;
    pInsert->PayloadSize = EndToCutDiff - sizeof(HeapHeader);
    
    if (pInsert->pNext)
    {
        pInsert->pNext->pPrev = pInsert;
    }
    pRef->pNext = pInsert;
    pRef->PayloadSize -= EndToCutDiff;

    return TRUE;
}
