#ifndef YCH_KERNEL_PCIE_PCIE_H
#define YCH_KERNEL_PCIE_PCIE_H

#include "Krnlych.h"

#define PCIE_MIN_BUS 0x00
#define PCIE_MAX_BUS 0xFF
#define PCIE_MIN_DEV 0x00
#define PCIE_MAX_DEV 0x20
#define PCIE_MIN_FUN 0x00
#define PCIE_MAX_FUN 0x08

#define PCI_CLASS_LEGACY                            ((UCHAR) 0x00)
#define PCI_CLASS_MASS_STORAGE_CONTROLLER           ((UCHAR) 0x01)
#define PCI_CLASS_NETWORK_CONTROLLER                ((UCHAR) 0x02)
#define PCI_CLASS_DISPLAY_CONTROLLER                ((UCHAR) 0x03)
#define PCI_CLASS_MULTIMEDIA_DEVICE                 ((UCHAR) 0x04)
#define PCI_CLASS_MEMORY_CONTROLLER                 ((UCHAR) 0x05)
#define PCI_CLASS_BRIDGE_DEVICE                     ((UCHAR) 0x06)
#define PCI_CLASS_SIMPLE_COMMUNICATIONS_CONTROLLERS ((UCHAR) 0x07)
#define PCI_CLASS_BASE_SYSTEM_PERIPHERALS           ((UCHAR) 0x08)
#define PCI_CLASS_INPUT_DEVICES                     ((UCHAR) 0x09)
#define PCI_CLASS_DOCKING_STATIONS                  ((UCHAR) 0x0A)
#define PCI_CLASS_PROCESSORS                        ((UCHAR) 0x0B)
#define PCI_CLASS_SERIAL_BUS_CONTROLLERS            ((UCHAR) 0x0C)
#define PCI_CLASS_WIRELESS_CONTROLLERS              ((UCHAR) 0x0D)
#define PCI_CLASS_INTELLIGENT_CONTROLLERS           ((UCHAR) 0x0E)

#define PCI_SUBCLASS_SCSI                           ((UCHAR) 0x00)
#define PCI_SUBCLASS_IDE                            ((UCHAR) 0x01)
#define PCI_SUBCLASS_FLOPPY_DISK_CONTROLLER         ((UCHAR) 0x02)
#define PCI_SUBCLASS_IPI_BUS_CONTROLLER             ((UCHAR) 0x03)
#define PCI_SUBCLASS_RAID_CONTROLLER                ((UCHAR) 0x04)
#define PCI_SUBCLASS_ATA_CONTROLLER                 ((UCHAR) 0x05)
#define PCI_SUBCLASS_SATA_CONTROLLER                ((UCHAR) 0x06)
#define PCI_SUBCLASS_SAS_CONTROLLER                 ((UCHAR) 0x07)
#define PCI_SUBCLASS_NVM_CONTROLLER                 ((UCHAR) 0x08)

#define PCIE_CFG_REG_VENDOR_ID       0x0000 // Word
#define PCIE_CFG_REG_DEVICE_ID       0x0002 // Word
#define PCIE_CFG_REG_COMMAND         0x0004 // Word
#define PCIE_CFG_REG_STATUS          0x0006 // Word
#define PCIE_CFG_REG_REVISION_ID     0x0008 // Byte
#define PCIE_CFG_REG_PROG_IF         0x0009 // Byte
#define PCIE_CFG_REG_SUBCLASS        0x000A // Byte
#define PCIE_CFG_REG_CLASS           0x000B // Byte
#define PCIE_CFG_REG_CACHE_LINE_SIZE 0x000C // Byte
#define PCIE_CFG_REG_LATENCY_TIMER   0x000D // Byte
#define PCIE_CFG_REG_HEADER_TYPE     0x000E // Byte

typedef VOID (*PCIeEndpointFound)(UINTPTR VaddrCfgSpace, WORD VendorID, WORD DeviceID, UCHAR Class, UCHAR SubClass, UCHAR Bus, UCHAR Device, UCHAR Function, UCHAR ProgIF);

UINTPTR PcieGetVoffset(UCHAR Bus, UCHAR Device, UCHAR Function, USHORT Register);
DWORD PcieReadDword(UCHAR Bus, UCHAR Device, UCHAR Function, USHORT Register);
WORD PcieReadWord(UCHAR Bus, UCHAR Device, UCHAR Function, USHORT Register);
BYTE PcieReadByte(UCHAR Bus, UCHAR Device, UCHAR Function, USHORT Register);

VOID PcieEnumerate(PCIeEndpointFound Callback);
CSTR PcieGetClassString(UCHAR Class);

#endif // !YCH_KERNEL_PCIE_PCIE_H
