#include "BootContract/BootContract.h"

#include <efi.h>
#include <efilib.h>
#include "blstd.h"

#define YCH_COMBINE_INNER(x, y) x##y
#define YCH_COMBINE(x, y) YCH_COMBINE_INNER(x, y)

#define BOOTELVT_FILE_NAME_ON_DISK_U16LE YCH_COMBINE(L, BOOTELVT_FILE_NAME)
#define KERNEL_FILE_NAME_ON_DISK_U16LE YCH_COMBINE(L, KERNEL_FILE_NAME)

EFI_STATUS YchReadFile(EFI_FILE_PROTOCOL* root, const CHAR16* filepath, VOID* ldAddr, UINT64* pOutFileSize)
{
    EFI_FILE_PROTOCOL* file;
    EFI_STATUS status = uefi_call_wrapper(root->Open, 5, root, &file, filepath, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status))
    {
        Print(L"Failed to open file `%s`: %r\n", filepath, status);
        return status;
    }

    EFI_FILE_INFO* fileInfo;
    UINTN fileInfoSize = sizeof(EFI_FILE_INFO) + 1024;
    fileInfo = (EFI_FILE_INFO*) uefi_call_wrapper(AllocatePool, 1, fileInfoSize);
    status = uefi_call_wrapper(file->GetInfo, 4, file, &gEfiFileInfoGuid, &fileInfoSize, fileInfo);
    if (EFI_ERROR(status))
    {
        Print(L"Failed to get file size `%s`: %r\n", filepath, status);
        return status;
    }
    UINTN szFile = (UINTN) fileInfo->FileSize;
    uefi_call_wrapper(FreePool, 1, fileInfo);

    status = uefi_call_wrapper(file->Read, 3, file, &szFile, ldAddr);
    if (EFI_ERROR(status))
    {
        Print(L"Failed to load file into memory `%s`: %r\n", filepath, status);
        return status;
    }
    uefi_call_wrapper(file->Close, 1, file);
    if(pOutFileSize)
    {
        *pOutFileSize = szFile;
    }
    return status;
}

BOOLEAN IsOVMF(void)
{
    CHAR16 *fwVendor = ST->FirmwareVendor;
    if (fwVendor == NULL) return FALSE;

    if (StrStr(fwVendor, L"EDK II") != NULL) {
        return TRUE;
    }
    return FALSE;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* pSystemTable)
{
    InitializeLib(ImageHandle, pSystemTable);
    Print(L"YchBoot loaded.\n");

    EFI_HANDLE* pSimpleFsHandles = NULL;
    UINTN simpleFsHandleCount = 0;
    EFI_STATUS status = uefi_call_wrapper(
        gBS->LocateHandleBuffer,
        5,
        ByProtocol,
        &gEfiSimpleFileSystemProtocolGuid,
        NULL,
        &simpleFsHandleCount,
        &pSimpleFsHandles
    );

    if (EFI_ERROR(status))
    {
        Print(L"Failed to locate FS handles: %r\n", status);
        return status;
    }

    // For now assume 2nd FAT32 is OS partition, TODO: later change and check each.
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;
    status = uefi_call_wrapper(gBS->HandleProtocol, 3, pSimpleFsHandles[1], &gEfiSimpleFileSystemProtocolGuid, (void**) &fs);
    
    if (EFI_ERROR(status))
    {
        Print(L"Failed to open OS FS handle: %r\n", status);
        return status;
    }

    // Open volume
    EFI_FILE_PROTOCOL* root;
    status = uefi_call_wrapper(fs->OpenVolume, 2, fs, &root); 
    if (EFI_ERROR(status))
    {
        Print(L"Failed to open OS volume: %r\n", status);
        return status;
    }

    VOID* ldAddrBootelvt = (VOID*) BOOTELVT_LOAD_ADDRESS;
    VOID* ldAddrKrnl     = (VOID*) KERNEL_LOAD_ADDRESS;
    UINT64 szKrnl;

    status = YchReadFile(root, BOOTELVT_FILE_NAME_ON_DISK_U16LE, ldAddrBootelvt, NULL);
    if (EFI_ERROR(status))
    {
        Print(L"CRITICAL BOOT CHAIN ERROR: BOOT ELEVATE (BOOTELVT.BIN) NOT FOUND!\n");
        return status;
    }
    status = YchReadFile(root, KERNEL_FILE_NAME_ON_DISK_U16LE, ldAddrKrnl, &szKrnl);
    if (EFI_ERROR(status))
    {
        Print(L"CRITICAL BOOT CHAIN ERROR: KERNEL (KRNLYCH.KR) NOT FOUND!\n");
        return status;
    }

    EFI_GRAPHICS_OUTPUT_PROTOCOL* pGOP;
    status = uefi_call_wrapper
    (
        gBS->LocateProtocol,
        3,
        &gEfiGraphicsOutputProtocolGuid,
        NULL,
        (void**) &pGOP
    );

    if (EFI_ERROR(status))
    {
        Print(L"GOP (Graphics Output Protocol) not found: %r\n", status);
        return status;
    }

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION pBestModeCache = {0};
    UINT32 iBestVideoModeCandidate = 0;

    BOOLEAN bIsOVMF = IsOVMF();
    for (UINT32 i = 0; i < pGOP->Mode->MaxMode; i++)
    {
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* pModeInfo;
        UINTN sz;

        status = uefi_call_wrapper(pGOP->QueryMode, 4, pGOP, i, &sz, &pModeInfo);
        if (EFI_ERROR(status))
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
            if (bIsOVMF && pModeInfo->HorizontalResolution == 1280 && pModeInfo->VerticalResolution == 720)
            {
                break;
            }
        }
    }

    if (iBestVideoModeCandidate && pBestModeCache.VerticalResolution && pBestModeCache.HorizontalResolution)
    {
        status = uefi_call_wrapper(pGOP->SetMode, 2, pGOP, iBestVideoModeCandidate);
        if (EFI_ERROR(status))
        {
            Print(L"WARN: GOP SetVideoMode failed: %r. Boot-time default video mode will be used.\n", status);
        }
    }

    KrSystemInfoPack bootInfo = {0};

    bootInfo.Magic                    = SYSTEM_INFO_PACK_MAGIC;
    bootInfo.KernelReserveSize        = KERNEL_RESERVE_SIZE;
    bootInfo.KernelBinarySize         = (uint64_t) szKrnl;
    bootInfo.KernelPhysicalBase       = (uintptr_t) ldAddrKrnl;
    bootInfo.KernelVirtualBase        = KERNEL_VIRTUAL_ADDRESS;
    bootInfo.KernelStackPhysicalBase  = bootInfo.KernelPhysicalBase + bootInfo.KernelReserveSize;
    bootInfo.KernelStackVirtualBase   = bootInfo.KernelVirtualBase + bootInfo.KernelReserveSize;
    bootInfo.KernelBootstrapArenaBase = BOOTSTRAP_KERNEL_AREA_BEGIN;
    bootInfo.KernelBootstrapArenaSize = BOOTSTRAP_KERNEL_AREA_SIZE;

    bootInfo.GraphicsInfo.PhysicalFramebufferAddress = pGOP->Mode->FrameBufferBase;
    bootInfo.GraphicsInfo.FramebufferSize = pGOP->Mode->FrameBufferSize;
    bootInfo.GraphicsInfo.FramebufferWidth = pGOP->Mode->Info->HorizontalResolution;
    bootInfo.GraphicsInfo.FramebufferHeight = pGOP->Mode->Info->VerticalResolution;
    bootInfo.GraphicsInfo.PixelsPerScanLine = pGOP->Mode->Info->PixelsPerScanLine;

    UINTN szMemoryMap = 0;
    EFI_MEMORY_DESCRIPTOR* pMemoryMap = NULL;
    UINTN kMapKey;      // Memory map key (ExitBootServices must be called with this)
    UINTN szDescriptor; // Descriptor size
    UINT32 vDescriptor; // Descriptor version

    status = uefi_call_wrapper(gBS->GetMemoryMap, 5, &szMemoryMap, pMemoryMap, &kMapKey, &szDescriptor, &vDescriptor);
    if (status != EFI_BUFFER_TOO_SMALL) // status is expected to be EFI_BUFFER_TOO_SMALL.
    {
        Print(L"Failed to acquire the memory map: %r\n", status);
        return status;
    }

    szMemoryMap += szDescriptor * 2;
    pMemoryMap   = AllocatePool(szMemoryMap);

    status = uefi_call_wrapper(gBS->GetMemoryMap, 5, &szMemoryMap, pMemoryMap, &kMapKey, &szDescriptor, &vDescriptor);
    if (EFI_ERROR(status))
    {
        Print(L"Failed to acquire the memory map(2nd pass): %r\n", status);
        return status;
    }
    
    // Call ExitBootServices, exiting UEFI Firmware completely. OS now has full control over memory hereafter.
    uefi_call_wrapper(gBS->ExitBootServices, 2, ImageHandle, kMapKey);

    bootInfo.MemoryMapInfo.PhysicalAddress = MEMORY_MAP_ADDRESS;
    bootInfo.MemoryMapInfo.EntryCount = szMemoryMap / szDescriptor;
    bootInfo.MemoryMapInfo.MemoryMapSize = bootInfo.MemoryMapInfo.EntryCount * sizeof(KrMemoryDescriptor);
    bootInfo.MemoryMapInfo.PageSize = 4096; // per UEFI MEMORY_DESCRIPTOR standard
    bootInfo.MemoryMapInfo.Pad = 0;

    KrMemoryDescriptor* pDescEntries = (KrMemoryDescriptor*) bootInfo.MemoryMapInfo.PhysicalAddress;
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
        "mov %[bootinfosize], %%rcx\n\t"
        "mov %[bootinfo], %%rsi\n\t"
        "jmp *%[bootelvt]\n\t"
         :
         : [stack]    "r"(bootInfo.KernelStackPhysicalBase),
           [bootinfosize] "r"(szstruct),
           [bootinfo] "r"(&bootInfo),
           [bootelvt]   "r"(ldAddrBootelvt)
         : "rdi", "rcx"
    );

    while (1);
    return EFI_SUCCESS;
}
