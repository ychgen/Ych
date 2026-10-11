#include "ACPI/PCIe.h"

#include "Core/KernelState.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

static VOID PcieProbeFunction(PCIeEndpointFound Callback, UCHAR Bus, UCHAR Device, UCHAR Function);

UINTPTR PcieGetVoffset(UCHAR Bus, UCHAR Device, UCHAR Function, USHORT Register)
{
    UINTPTR OffsetIntoVaddr = ((UINTPTR) Bus << 20) | ((UINTPTR) Device << 15) | ((UINTPTR) Function << 12) | (Register & 0xFFF);
    return g_KernelState.MmioInfo.VaddrPciEcamBase + OffsetIntoVaddr;
}

DWORD PcieReadDword(UCHAR Bus, UCHAR Device, UCHAR Function, USHORT Register)
{
    return *(volatile DWORD*)(PcieGetVoffset(Bus, Device, Function, Register));
}

WORD PcieReadWord(UCHAR Bus, UCHAR Device, UCHAR Function, USHORT Register)
{
    return *(volatile WORD*)(PcieGetVoffset(Bus, Device, Function, Register));
}

BYTE PcieReadByte(UCHAR Bus, UCHAR Device, UCHAR Function, USHORT Register)
{
    return *(volatile BYTE*)(PcieGetVoffset(Bus, Device, Function, Register));
}

VOID PcieEnumerate(PCIeEndpointFound Callback)
{
    for (WORD Bus = PCIE_MIN_BUS; Bus <= PCIE_MAX_BUS; Bus++)
    {
        for (UCHAR Dev = PCIE_MIN_DEV; Dev < PCIE_MAX_DEV; Dev++)
        {
            WORD VendorID = PcieReadWord(Bus, Dev, 0, PCIE_CFG_REG_VENDOR_ID);
            if (VendorID == 0x0000 || VendorID == 0xFFFF)
            {
                continue;
            }

            PcieProbeFunction(Callback, Bus, Dev, 0);

            UCHAR HeaderType = PcieReadByte(Bus, Dev, 0, PCIE_CFG_REG_HEADER_TYPE);
            if (HeaderType & (1 << 7))
            {
                for (UCHAR Fun = 1; Fun < PCIE_MAX_FUN; Fun++)
                {
                    PcieProbeFunction(Callback, Bus, Dev, Fun);
                }
            }
        }
    }
}

CSTR PcieGetClassString(UCHAR Class)
{
    switch (Class)
    {
    case PCI_CLASS_LEGACY:                            return "Unclassified Legacy Device";
    case PCI_CLASS_MASS_STORAGE_CONTROLLER:           return "Mass Storage Controller";
    case PCI_CLASS_NETWORK_CONTROLLER:                return "Network Controller";
    case PCI_CLASS_DISPLAY_CONTROLLER:                return "Display Controller";
    case PCI_CLASS_MULTIMEDIA_DEVICE:                 return "Multimedia Device";
    case PCI_CLASS_MEMORY_CONTROLLER:                 return "Memory Controller";
    case PCI_CLASS_BRIDGE_DEVICE:                     return "Bridge Device";
    case PCI_CLASS_SIMPLE_COMMUNICATIONS_CONTROLLERS: return "Simple Communications Controller";
    case PCI_CLASS_BASE_SYSTEM_PERIPHERALS:           return "Base System Peripheral Device";
    case PCI_CLASS_INPUT_DEVICES:                     return "Input Device";
    case PCI_CLASS_DOCKING_STATIONS:                  return "Docking Station";
    case PCI_CLASS_PROCESSORS:                        return "Processor";
    case PCI_CLASS_SERIAL_BUS_CONTROLLERS:            return "Serial Bus Controller";
    case PCI_CLASS_WIRELESS_CONTROLLERS:              return "Wireless Controller";
    case PCI_CLASS_INTELLIGENT_CONTROLLERS:           return "Intelligent Controller";
    default: break;
    }
    return "Unclassified PCIe Device";
}

static VOID PcieProbeFunction(PCIeEndpointFound Callback, UCHAR Bus, UCHAR Device, UCHAR Function)
{
    WORD VendorID = PcieReadWord(Bus, Device, Function, PCIE_CFG_REG_VENDOR_ID);

    if (VendorID == 0x0000 || VendorID == 0xFFFF)
    {
        return;
    }

    WORD DeviceID = PcieReadWord(Bus, Device, Function, PCIE_CFG_REG_DEVICE_ID);
    BYTE Class = PcieReadByte(Bus, Device, Function, PCIE_CFG_REG_CLASS);
    BYTE SubClass = PcieReadByte(Bus, Device, Function, PCIE_CFG_REG_SUBCLASS);
    BYTE ProgIF = PcieReadByte(Bus, Device, Function, PCIE_CFG_REG_PROG_IF);

    if (Callback)
    {
        Callback(PcieGetVoffset(Bus, Device, Function, 0), VendorID, DeviceID, Class, SubClass, Bus, Device, Function, ProgIF);
    }
}
