#include "BootContract/BootContract.h"

#include "Utilities.h"

#define YCH_COMBINE_INNER(x, y) x##y
#define YCH_COMBINE(x, y) YCH_COMBINE_INNER(x, y)

#define BOOTELVT_FILE_NAME_ON_DISK_U16LE YCH_COMBINE(L, BOOTELVT_FILE_NAME)
#define KERNEL_FILE_NAME_ON_DISK_U16LE YCH_COMBINE(L, KERNEL_FILE_NAME)

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* pSystemTable)
{
    InitializeLib(ImageHandle, pSystemTable);
    Print(L"YchBoot loaded.\n");

    EFI_HANDLE* pSimpleFsHandles = NULL;
    UINTN SimpleFsHandleCount = 0;
    EFI_STATUS Status = uefi_call_wrapper(
        gBS->LocateHandleBuffer,
        5,
        ByProtocol,
        &gEfiSimpleFileSystemProtocolGuid,
        NULL,
        &SimpleFsHandleCount,
        &pSimpleFsHandles
    );

    if (EFI_ERROR(Status))
    {
        Print(L"Failed to locate FS handles: %r\n", Status);
        return Status;
    }

    // For now assume 2nd FAT32 is OS partition, TODO: later change and check each.
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* pFileSystem;
    Status = uefi_call_wrapper(gBS->HandleProtocol, 3, pSimpleFsHandles[1], &gEfiSimpleFileSystemProtocolGuid, (void**) &pFileSystem);
    
    if (EFI_ERROR(Status))
    {
        Print(L"Failed to open OS FS handle: %r\n", Status);
        return Status;
    }

    // Open volume
    EFI_FILE_PROTOCOL* pOsFileSystem;
    Status = uefi_call_wrapper(pFileSystem->OpenVolume, 2, pFileSystem, &pOsFileSystem); 
    if (EFI_ERROR(Status))
    {
        Print(L"Failed to open OS volume: %r\n", Status);
        return Status;
    }

    UINT64 KernelSize;

    Status = YchbootReadFile(pOsFileSystem, BOOTELVT_FILE_NAME_ON_DISK_U16LE, BOOTELVT_LOAD_ADDRESS, NULL);
    if (EFI_ERROR(Status))
    {
        Print(L"CRITICAL BOOT CHAIN ERROR: BOOT ELEVATE (BOOTELVT.BIN) NOT FOUND!\n");
        return Status;
    }
    Status = YchbootReadFile(pOsFileSystem, KERNEL_FILE_NAME_ON_DISK_U16LE, KERNEL_LOAD_ADDRESS, &KernelSize);
    if (EFI_ERROR(Status))
    {
        Print(L"CRITICAL BOOT CHAIN ERROR: KERNEL (KRNLYCH.KR) NOT FOUND!\n");
        return Status;
    }

    EFI_GRAPHICS_OUTPUT_PROTOCOL* pGOP;
    Status = uefi_call_wrapper
    (
        gBS->LocateProtocol,
        3,
        &gEfiGraphicsOutputProtocolGuid,
        NULL,
        (void**) &pGOP
    );

    if (EFI_ERROR(Status))
    {
        Print(L"GOP (Graphics Output Protocol) not found: %r\n", Status);
        return Status;
    }

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION pBestModeCache = {0};
    UINT32 iBestVideoModeCandidate = -1;

    BOOLEAN bIsHypervisor = YchbootIsHypervisorPresent();
    for (UINT32 i = 0; i < pGOP->Mode->MaxMode; i++)
    {
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* pModeInfo;
        UINTN sz;

        Status = uefi_call_wrapper(pGOP->QueryMode, 4, pGOP, i, &sz, &pModeInfo);
        if (EFI_ERROR(Status))
        {
            continue;
        }
        if ((pModeInfo->HorizontalResolution * 9) / 16 != pModeInfo->VerticalResolution)
        {
            continue;
        }

        if (pModeInfo->PixelFormat == PixelBlueGreenRedReserved8BitPerColor)
        {
            if ( pModeInfo->VerticalResolution > pBestModeCache.VerticalResolution ||
                (pModeInfo->VerticalResolution == pBestModeCache.VerticalResolution && pModeInfo->HorizontalResolution > pBestModeCache.HorizontalResolution))
            {
                iBestVideoModeCandidate = i;
                pBestModeCache = *pModeInfo;
            }
            // On VM choose smaller resolution.
            if (bIsHypervisor && pModeInfo->HorizontalResolution == 1280 && pModeInfo->VerticalResolution == 720)
            {
                break;
            }
        }
    }

    if (iBestVideoModeCandidate != -1 && pBestModeCache.VerticalResolution && pBestModeCache.HorizontalResolution)
    {
        Status = uefi_call_wrapper(pGOP->SetMode, 2, pGOP, iBestVideoModeCandidate);
        if (EFI_ERROR(Status))
        {
            Print(L"WARN: GOP SetVideoMode failed: %r. Boot-time default video mode will be used.\n", Status);
        }
    }

    KrSystemInfoPack BootInfo = {0};

    BootInfo.Magic                    = SYSTEM_INFO_PACK_MAGIC;
    BootInfo.KernelReserveSize        = KERNEL_RESERVE_SIZE;
    BootInfo.KernelBinarySize         = (uint64_t) KernelSize;
    BootInfo.KernelPhysicalBase       = KERNEL_LOAD_ADDRESS;
    BootInfo.KernelVirtualBase        = KERNEL_VIRTUAL_ADDRESS;
    BootInfo.KernelStackPhysicalBase  = BootInfo.KernelPhysicalBase + BootInfo.KernelReserveSize;
    BootInfo.KernelStackVirtualBase   = BootInfo.KernelVirtualBase + BootInfo.KernelReserveSize;
    BootInfo.KernelBootstrapArenaBase = BOOTSTRAP_KERNEL_AREA_BEGIN;
    BootInfo.KernelBootstrapArenaSize = BOOTSTRAP_KERNEL_AREA_SIZE;

    BootInfo.GraphicsInfo.PhysicalFramebufferAddress = pGOP->Mode->FrameBufferBase;
    BootInfo.GraphicsInfo.FramebufferSize = pGOP->Mode->FrameBufferSize;
    BootInfo.GraphicsInfo.FramebufferWidth = pGOP->Mode->Info->HorizontalResolution;
    BootInfo.GraphicsInfo.FramebufferHeight = pGOP->Mode->Info->VerticalResolution;
    BootInfo.GraphicsInfo.PixelsPerScanLine = pGOP->Mode->Info->PixelsPerScanLine;

    // RSDP
    BootInfo.PhysAddrRSDP = YchbootLocateRSDP(pSystemTable);

    UINTN szMemoryMap = 0;
    EFI_MEMORY_DESCRIPTOR* pMemoryMap = NULL;
    UINTN kMapKey;      // Memory map key (ExitBootServices must be called with this)
    UINTN szDescriptor; // Descriptor size
    UINT32 vDescriptor; // Descriptor version

    Status = uefi_call_wrapper(gBS->GetMemoryMap, 5, &szMemoryMap, pMemoryMap, &kMapKey, &szDescriptor, &vDescriptor);
    if (Status != EFI_BUFFER_TOO_SMALL) // Status is expected to be EFI_BUFFER_TOO_SMALL.
    {
        Print(L"Failed to acquire the memory map: %r\n", Status);
        return Status;
    }

    szMemoryMap += szDescriptor * 2;
    pMemoryMap   = AllocatePool(szMemoryMap);

    Status = uefi_call_wrapper(gBS->GetMemoryMap, 5, &szMemoryMap, pMemoryMap, &kMapKey, &szDescriptor, &vDescriptor);
    if (EFI_ERROR(Status))
    {
        Print(L"Failed to acquire the memory map(2nd pass): %r\n", Status);
        return Status;
    }
    
    // Call ExitBootServices, exiting UEFI Firmware completely. OS now has full control over memory hereafter.
    uefi_call_wrapper(gBS->ExitBootServices, 2, ImageHandle, kMapKey);

    BootInfo.MemoryMapInfo.PhysicalAddress = MEMORY_MAP_ADDRESS;
    BootInfo.MemoryMapInfo.EntryCount = szMemoryMap / szDescriptor;
    BootInfo.MemoryMapInfo.MemoryMapSize = BootInfo.MemoryMapInfo.EntryCount * sizeof(KrMemoryDescriptor);
    BootInfo.MemoryMapInfo.PageSize = 4096; // per UEFI MEMORY_DESCRIPTOR standard
    BootInfo.MemoryMapInfo.Pad = 0;

    KrMemoryDescriptor* pDescEntries = (KrMemoryDescriptor*) BootInfo.MemoryMapInfo.PhysicalAddress;
    for (UINT64 i = 0; i < szMemoryMap / szDescriptor; i++)
    {
        EFI_MEMORY_DESCRIPTOR* pEfiDesc = (EFI_MEMORY_DESCRIPTOR*)(((UINT8*) pMemoryMap) + i * szDescriptor);
        KrMemoryDescriptor* pDesc = pDescEntries + i;
        pDesc->PhysicalBase = pEfiDesc->PhysicalStart;
        pDesc->PageCount = pEfiDesc->NumberOfPages;
        pDesc->Attributes = 0;
        pDesc->Type = pEfiDesc->Type;
        pDesc->Pad = 0;
    }

    // Clear interrupts and jump to BOOTELVT.
    UINT64 szstruct=sizeof(KrSystemInfoPack);
    __asm__ __volatile__ (
        "cli\n\t"
        "mov %[stack], %%rsp\n\t"
        "mov %[BootInfosize], %%rcx\n\t"
        "mov %[BootInfo], %%rsi\n\t"
        "jmp *%[bootelvt]\n\t"
         :
         : [stack]    "r"(BootInfo.KernelStackPhysicalBase),
           [BootInfosize] "r"(szstruct),
           [BootInfo] "r"(&BootInfo),
           [bootelvt]   "r"((uintptr_t) BOOTELVT_LOAD_ADDRESS)
         : "rdi", "rcx"
    );

    while (1);
    return EFI_SUCCESS;
}
