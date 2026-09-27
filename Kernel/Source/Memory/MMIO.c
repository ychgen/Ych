#include "Memory/MMIO.h"

#include "Memory/Vmmdef/MakeVirt.h"
#include "Memory/Vmmdef/Indices.h"
#include "Memory/Virtmemmgmt.h"

#include "KRTL/Krnlmem.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

UINTPTR MmioAllocateRange(UINT Alignment, SIZE Size)
{
    static UINTPTR g_VaddrMmioHead = KR_MAKE_VIRTUAL(KRNL_PML4_IDX, KRNL_MMIO_PDPT_IDX, 0, 0, 0);
    UINTPTR VaddrBase = (g_VaddrMmioHead + (Alignment - 1)) & (~((UINTPTR) Alignment - 1));
    g_VaddrMmioHead = VaddrBase + Size;
    return VaddrBase;
}

UINTPTR MmioMapDevice(UINTPTR PaddrDeviceBase, SIZE DeviceAddressSize, BYTE Flags)
{
    // todo: handle unaligned dev base properly, for now we just error out
    
    const UINT PageSize = Flags & MMIO_FLAG_USE_LARGE_PAGES ? 0x200000 : 0x1000;
    if (!KrtlIsPowerOfTwoAligned(PaddrDeviceBase, PageSize))
    {
        return 0;
    }
    if (!KrtlIsPowerOfTwoAligned(PaddrDeviceBase + DeviceAddressSize, PageSize))
    {
        return 0;
    }
    UINTPTR VaddrDeviceBase = MmioAllocateRange(PageSize, DeviceAddressSize);

    WORD RegionFlags = VMR_FLAG_READABLE | VMR_FLAG_WRITABLE |
                       VMR_FLAG_STATIC | ((Flags & MMIO_FLAG_WRITE_COMBINE) ? VMR_CACHE_PROTOCOL_WRITE_COMBINE : VMR_CACHE_PROTOCOL_UNCACHEABLE);

    KrVirtualMemoryRegion* pNode = VmAcquireRegion(KrGetKernelAddressSpace(), VaddrDeviceBase, VaddrDeviceBase + DeviceAddressSize, RegionFlags);
    if (!pNode)
    {
        return 0;
    }

    if (!VmMapStatic(pNode, PaddrDeviceBase))
    {
        VmRelinquishRegion(pNode);
        return 0;
    }

    return VaddrDeviceBase;
}
