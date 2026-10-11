#ifndef YCH_KERNEL_DEVDRIV_BUSES_H
#define YCH_KERNEL_DEVDRIV_BUSES_H

#include "Krnlych.h"

#define PCI_ANY_VENDOR_ID  ((USHORT) 0xFFFF)
#define PCI_ANY_DEVICE_ID  ((USHORT) 0xFFFF)
#define PCI_ANY_BUS        ((UCHAR ) 0xFF  )
#define PCI_ANY_DEVICE     ((UCHAR ) 0xFF  )
#define PCI_ANY_FUNCTION   ((UCHAR ) 0xFF  )
#define PCI_ANY_PROG_IF    ((UCHAR ) 0xFF  )
#define PCI_ANY_CLASS      ((UCHAR ) 0xFF  )
#define PCI_ANY_SUBCLASS   ((UCHAR ) 0xFF  )

typedef enum
{
    BUS_IVLD,
    BUS_ROOT,
    BUS_PLATFORM,
    BUS_PCI,
    BUS_USB
} BusType;

typedef struct
{
    UINTPTR VaddrCfgSpace;
    USHORT VendorID, DeviceID;
    UCHAR  Bus, Device, Function, ProgIF;
    UCHAR  Class, SubClass;
} PciDeviceId;

#endif // !YCH_KERNEL_DEVDRIV_BUSES_H
