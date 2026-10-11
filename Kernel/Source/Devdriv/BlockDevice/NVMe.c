#include "Devdriv/BlockDevice/NVMe.h"

#include "ACPI/PCIe.h"

DriverMatchMethod g_NvmeDriverMatchMethods[] = {
    { .OnBus = BUS_PCI, .Pci = {
        .VendorID = PCI_ANY_VENDOR_ID,
        .DeviceID = PCI_ANY_DEVICE_ID,
        .Bus = PCI_ANY_BUS,
        .Device = PCI_ANY_DEVICE,
        .Function = PCI_ANY_FUNCTION,
        .Class = PCI_CLASS_MASS_STORAGE_CONTROLLER,
        .SubClass = PCI_SUBCLASS_NVM_CONTROLLER,
        .ProgIF = PCI_ANY_PROG_IF
    } },
    { .OnBus = BUS_IVLD } // terminator
};

BlockDeviceDriver g_GenericNvmeDriver = {
    .Object = {
        .Name = "Ych NVM Express Block Device Driver",
        .Type = DRIVER_BLOCK_DEVICE,
        .pInterface = &g_GenericNvmeDriver,
        .pMatchMethods = g_NvmeDriverMatchMethods,
        .DevProbe = NvmeDevProbe,
        .DevInit = NvmeDevInit,
        .DevShutdown = NvmeDevShutdown
    },
    .GetSectorSize = NvmeGetSectorSize,
    .GetSectorCount = NvmeGetSectorCount
};

BOOL NvmeDevProbe(struct DeviceNode* pDev, DriverMatchMethod* pClamationMethodRef)
{
    KR_UNUSED(pDev);
    KR_UNUSED(pClamationMethodRef);
    return TRUE;
}

BOOL NvmeDevInit(struct DeviceNode* pDev)
{
    KR_UNUSED(pDev);
    return TRUE;
}

VOID NvmeDevShutdown(struct DeviceNode* pDev)
{
    KR_UNUSED(pDev);
}

UINT NvmeGetSectorSize(struct DeviceNode* pDev)
{
    KR_UNUSED(pDev);
    return 0;
}

ULONG NvmeGetSectorCount(struct DeviceNode* pDev)
{
    KR_UNUSED(pDev);
    return 0;
}
