#include "YchBootContract.h"

#include <efi.h>
#include <efilib.h>

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
    status = uefi_call_wrapper(root->Open, 5, root, &krnl, KERNEL_FILE_NAME_ON_DISK_U16LE, EFI_FILE_MODE_READ, 0);
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

    KrSystemInfoPack bootInfo;
    bootInfo.Magic = SYSTEM_INFO_PACK_MAGIC;
    bootInfo.KernelBinarySize = (uint64_t) szKernel;

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
        Print(L"Failed to acquire the memory map2: %r\n", status);
        return status;
    }

    bootInfo.MemoryMapInfo.PhysicalAddress = (uint64_t) pMemoryMap;
    bootInfo.MemoryMapInfo.MemoryMapSize = szMemoryMap;
    bootInfo.MemoryMapInfo.DescriptorSize = szDescriptor;
    bootInfo.MemoryMapInfo.NumberOfEntries = szMemoryMap / szDescriptor;
    bootInfo.MemoryMapInfo.DescriptorVersion = vDescriptor;
    
    // Call ExitBootServices, exiting UEFI Firmware completely. OS now has full control over memory hereafter.
    uefi_call_wrapper(gBS->ExitBootServices, 2, ImageHandle, kMapKey);

    // 2MiB above reserved kernel area
    uint64_t addrStack = KERNEL_RESERVED_AREA_END + 0x200000;

    // Clear interrupts and jump to kernel.
    __asm__ __volatile__ (
        "cli\n\t"
        "mov %[stack], %%rsp\n\t"
        "mov %[bootinfo], %%rdi\n\t"
        "jmp *%[kernel]\n\t"
         :
         : [stack]    "r"(addrStack),
           [bootinfo] "r"(&bootInfo),
           [kernel]   "r"(ldAddrKernel)
         : "rdi"
    );
    
    while (1);
    return EFI_SUCCESS;
}
