#include <efi.h>
#include <efilib.h>

#define KERNEL_FILE_NAME    L"\\Ych\\Krnlych.img"
#define KERNEL_LOAD_ADDRESS   0x100000
#define KERNEL_PARAM_ADDRESS  0x4000000 // 64 MiB

typedef struct __attribute__((packed))
{
    UINT64 PhysicalFrameBufferAddress;
    UINT64 FramebufferSize;
    UINT32 FramebufferWidth;
    UINT32 FramebufferHeight;
    UINT32 PixelsPerScanLine;
} KrGraphicsInfo;

typedef struct __attribute__((packed))
{
    KrGraphicsInfo GraphicsInfo;
} KrBootInfo;

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

    EFI_FILE_PROTOCOL* krnl;
    status = uefi_call_wrapper(root->Open, 5, root, &krnl, KERNEL_FILE_NAME, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status))
    {
        Print(L"Failed to open kernel file: %r\n", status);
        return status;
    }

    EFI_FILE_INFO* fileInfo;
    UINTN fileInfoSize = sizeof(EFI_FILE_INFO) + 1024;
    fileInfo = (EFI_FILE_INFO*) uefi_call_wrapper(AllocatePool, 1, fileInfoSize);
    status = uefi_call_wrapper(krnl->GetInfo, 4, krnl, &gEfiFileInfoGuid, &fileInfoSize, fileInfo);
    if (EFI_ERROR(status))
    {
        Print(L"Failed to get kernel size: %r\n", status);
        return status;
    }
    UINTN szKernel = (UINTN) fileInfo->FileSize;
    uefi_call_wrapper(FreePool, 1, fileInfo);

    void* ldAddrKernel = (void*) KERNEL_LOAD_ADDRESS;
    status = uefi_call_wrapper(krnl->Read, 3, krnl, &szKernel, ldAddrKernel);
    if (EFI_ERROR(status))
    {
        Print(L"Failed to load kernel into memory: %r\n", status);
        return status;
    }
    uefi_call_wrapper(krnl->Close, 1, krnl);

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

    KrBootInfo* pBootInfo = (KrBootInfo*) KERNEL_PARAM_ADDRESS;
    pBootInfo->GraphicsInfo.PhysicalFrameBufferAddress = pGOP->Mode->FrameBufferBase;
    pBootInfo->GraphicsInfo.FramebufferSize = pGOP->Mode->FrameBufferSize;
    pBootInfo->GraphicsInfo.FramebufferWidth = pGOP->Mode->Info->HorizontalResolution;
    pBootInfo->GraphicsInfo.FramebufferHeight = pGOP->Mode->Info->VerticalResolution;
    pBootInfo->GraphicsInfo.PixelsPerScanLine = pGOP->Mode->Info->PixelsPerScanLine;

    UINTN memMapSize = 0;
    EFI_MEMORY_DESCRIPTOR* memMap = NULL;
    UINTN mapKey;
    UINTN descriptorSize;
    UINT32 descriptorVersion;

    uefi_call_wrapper(gBS->GetMemoryMap, 5, &memMapSize, memMap, &mapKey, &descriptorSize, &descriptorVersion);
    // allocate memMap buffer, call again
    uefi_call_wrapper(gBS->ExitBootServices, 2, ImageHandle, mapKey);

    __asm__ __volatile__("jmp *%0" : : "r"(ldAddrKernel));
    
    while (1);
    return EFI_SUCCESS;
}
