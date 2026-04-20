#include "Init/KrInitMemmap.h"

#include "Memory/BootstrapArena.h"
#include "Memory/Physmemmgmt.h"
#include "Core/KernelState.h"
#include "KRTL/Krnlmem.h"

VOID KrSwapMemdescs(KrMemoryDescriptor* pLHS, KrMemoryDescriptor* pRHS)
{
    KrMemoryDescriptor Temp = *pLHS;
    *pLHS = *pRHS;
    *pRHS = Temp;
}

UINT KrPartDescarray(KrMemoryDescriptor* Array, UINT Start, UINT Limit)
{
    UINTPTR Pivot = Array[Limit].PhysicalBase;
    INT i = (INT) Start;
    i--;

    for (UINT j = Start; j < Limit; j++)
    {
        if (Array[j].PhysicalBase < Pivot)
        {
            i++;
            KrSwapMemdescs(&Array[i], &Array[j]);
        }
    }

    KrSwapMemdescs(&Array[i + 1], &Array[Limit]);
    return i + 1;
}

VOID KrQuicksortMemmap(KrMemoryDescriptor* Map, UINT Start, UINT Limit)
{
    while (Start < Limit)
    {
        UINT pi = KrPartDescarray(Map, Start, Limit);
        if (pi - Start < Limit - pi)
        {
            KrQuicksortMemmap(Map, Start, pi - 1);
            Start = pi + 1;
        }
        else
        {
            KrQuicksortMemmap(Map, pi + 1, Limit);
            Limit = pi - 1;
        }
    }
}

VOID KrInitMemmap(const KrMemoryMapInfo* pMemmapInfo)
{
    // Copy over the memory map
    {
        // Make sure barena is 16-aligned
        KrBootstrapArenaAlign(16);

        g_KernelState.MemoryMapInfo = *pMemmapInfo;
        g_KernelState.MemoryMap = KrBootstrapArenaAcquire(pMemmapInfo->MemoryMapSize);
        KrtlContiguousCopyBuffer(g_KernelState.MemoryMap, (VOID*) pMemmapInfo->PhysicalAddress, pMemmapInfo->MemoryMapSize);
        KrQuicksortMemmap(g_KernelState.MemoryMap, 0, pMemmapInfo->EntryCount - 1);
    }
    // Build CanonicalMemoryMap (used when you just want to iterate regions classified as usable)
    {
        // Make sure barena is 16-aligned, enjoy General Protection Fault otherwise
        KrBootstrapArenaAlign(16);
        // Make sure enough space exists (who cares about wasting space anyway)
        g_KernelState.CanonicalMemoryMap = KrBootstrapArenaAcquire(pMemmapInfo->MemoryMapSize);
        for (UINT i = 0; i < pMemmapInfo->EntryCount; i++)
        {
            KrMemoryDescriptor* pFwDescriptor = g_KernelState.MemoryMap + i;
            
            if (!KrIsUsableMemoryRegionType(pFwDescriptor->Type))
            {
                continue;
            }

            /** Super Region  */
            UINTPTR PhysAddrRegionBase = pFwDescriptor->PhysicalBase;
            ULONG   CanonicalPageCount = pFwDescriptor->PageCount;

            // Peek ahead
            UINT j = i + 1;
            while (j < pMemmapInfo->EntryCount)
            {
                KrMemoryDescriptor* pFwDescAhead = (KrMemoryDescriptor*) g_KernelState.MemoryMap + j;
                
                // Two requirements:
                //   Did we exhaust the usable chain?
                //   Are the regions actually sequential in the address space?
                // Russian roulette ass.
                if (!KrIsUsableMemoryRegionType(pFwDescAhead->Type) ||
                    (PhysAddrRegionBase + CanonicalPageCount * pMemmapInfo->PageSize) != pFwDescAhead->PhysicalBase)
                {
                    break;
                }
                
                // Usable region(s) stacked, combine.
                CanonicalPageCount += pFwDescAhead->PageCount;
                j++;
            }
            i = j - 1;

            KrMemoryDescriptor* pCanonicalDescriptor = g_KernelState.CanonicalMemoryMap + g_KernelState.NumCanonicalMapEntries++;
            pCanonicalDescriptor->PhysicalBase = PhysAddrRegionBase;
            pCanonicalDescriptor->PageCount = CanonicalPageCount;
            pCanonicalDescriptor->Type = KR_MEMORY_TYPE_CONVENTIONAL_MEMORY; // because I decided so
            pCanonicalDescriptor->Attributes = 0; // because I decided so
            pCanonicalDescriptor->Pad = 0;
        }
    }
}
