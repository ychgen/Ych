#include <efi.h>
#include <efilib.h>

#include "blstd.h"

BOOLEAN YchbootCompareGuid(const EFI_GUID* LHS, const EFI_GUID* RHS)
{
    return LHS->Data1    == RHS->Data1    &&
           LHS->Data2    == RHS->Data2    &&
           LHS->Data3    == RHS->Data3    &&
           LHS->Data4[0] == RHS->Data4[0] &&
           LHS->Data4[1] == RHS->Data4[1] &&
           LHS->Data4[2] == RHS->Data4[2] &&
           LHS->Data4[3] == RHS->Data4[3] &&
           LHS->Data4[4] == RHS->Data4[4] &&
           LHS->Data4[5] == RHS->Data4[5] &&
           LHS->Data4[6] == RHS->Data4[6] &&
           LHS->Data4[7] == RHS->Data4[7];
}

EFI_STATUS YchbootReadFile(EFI_FILE_PROTOCOL* pRoot, const CHAR16* pFilepath, uintptr_t AddrLoad, UINT64* pOutFileSize)
{
    EFI_FILE_PROTOCOL* pFile;
    EFI_STATUS Status = uefi_call_wrapper(pRoot->Open, 5, pRoot, &pFile, pFilepath, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(Status))
    {
        Print(L"Failed to open file `%s`: %r\n", pFilepath, Status);
        return Status;
    }

    EFI_FILE_INFO* pFileInfo;
    UINTN FileInfoSize = sizeof(EFI_FILE_INFO) + 1024;
    pFileInfo = (EFI_FILE_INFO*) uefi_call_wrapper(AllocatePool, 1, FileInfoSize);
    Status = uefi_call_wrapper(pFile->GetInfo, 4, pFile, &gEfiFileInfoGuid, &FileInfoSize, pFileInfo);
    if (EFI_ERROR(Status))
    {
        Print(L"Failed to get file size `%s`: %r\n", pFilepath, Status);
        return Status;
    }
    UINTN FileSize = (UINTN) pFileInfo->FileSize;
    uefi_call_wrapper(FreePool, 1, pFileInfo);

    Status = uefi_call_wrapper(pFile->Read, 3, pFile, &FileSize, (void*) AddrLoad);
    if (EFI_ERROR(Status))
    {
        Print(L"Failed to load file into memory `%s`: %r\n", pFilepath, Status);
        return Status;
    }
    uefi_call_wrapper(pFile->Close, 1, pFile);
    if (pOutFileSize)
    {
        *pOutFileSize = FileSize;
    }
    return Status;
}

uintptr_t YchbootLocateRSDP(EFI_SYSTEM_TABLE* pSystemTable)
{   
    const EFI_GUID Acpi20 = ACPI_20_TABLE_GUID;
    for (UINTN i = 0; i < pSystemTable->NumberOfTableEntries; i++)
    {
        EFI_CONFIGURATION_TABLE* pConfigTable = pSystemTable->ConfigurationTable + i;
        if (YchbootCompareGuid(&pConfigTable->VendorGuid, &Acpi20))
        {
            return (uintptr_t) pConfigTable->VendorTable;
        }
    }
    return 0;
}

UINT32 YchbootIsHypervisorPresent(void)
{
    UINT32 ECX;

    __asm__ __volatile__
    (
        "cpuid"
        : "=c"(ECX)
        : "a"(1)
        : "ebx", "edx"
    );

    return (ECX & (1u << 31)) != 0;
}

