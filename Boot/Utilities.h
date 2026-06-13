#include <efi.h>
#include <efilib.h>

#include "blstd.h"

BOOLEAN YchCompareGuid(const EFI_GUID* a, const EFI_GUID* b)
{
    return a->Data1 == b->Data1 &&
           a->Data2 == b->Data2 &&
           a->Data3 == b->Data3 &&
           a->Data4[0] == b->Data4[0] &&
           a->Data4[1] == b->Data4[1] &&
           a->Data4[2] == b->Data4[2] &&
           a->Data4[3] == b->Data4[3] &&
           a->Data4[4] == b->Data4[4] &&
           a->Data4[5] == b->Data4[5] &&
           a->Data4[6] == b->Data4[6] &&
           a->Data4[7] == b->Data4[7];
}

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

uintptr_t YchLocateRSDP(EFI_SYSTEM_TABLE* pSystemTable)
{   
    const EFI_GUID Acpi20 = ACPI_20_TABLE_GUID;
    for (UINTN i = 0; i < pSystemTable->NumberOfTableEntries; i++)
    {
        EFI_CONFIGURATION_TABLE* pConfigTable = pSystemTable->ConfigurationTable + i;
        if (YchCompareGuid(&pConfigTable->VendorGuid, &Acpi20))
        {
            return (uintptr_t) pConfigTable->VendorTable;
        }
    }
    return 0;
}
