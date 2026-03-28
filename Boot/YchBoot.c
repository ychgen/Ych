#include <efi.h>
#include <efilib.h>

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* pSystemTable)
{
    InitializeLib(ImageHandle, pSystemTable);
    Print(L"YchBoot!\n");

    while (1);
    return EFI_SUCCESS;
}
