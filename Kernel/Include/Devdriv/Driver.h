#ifndef YCH_KERNEL_DEVDRIV_DRIVER_H
#define YCH_KERNEL_DEVDRIV_DRIVER_H

#include "Devdriv/Buses.h"

#define MAX_DRIVER_NAME 256

struct DeviceNode;

typedef enum
{
    DRIVER_UNSPECIFIED = 0,
    DRIVER_BLOCK_DEVICE,
    DRIVER_VIDEO,
    DRIVER_NETWORK,
    DRIVER_KEYBOARD,
    DRIVER_TIMER,
} DriverType;

typedef struct
{
    BusType OnBus; // Last entry carries type BUS_IVLD
    union
    {
        PciDeviceId Pci;
    };
} DriverMatchMethod;

#define DRIVMM_TERMINATOR (DriverMatchMethod) { .OnBus = BUS_IVLD }

typedef struct Driver
{
    CHAR Name[MAX_DRIVER_NAME];
    DriverType Type;
    VOID* pInterface;
    DriverMatchMethod* pMatchMethods;

    BOOL (*Register)(VOID); // Called by Devdrivmgmt once driver is registered
    VOID (*Unregister)(VOID); // Called by Devdrivmgmt once driver is registered

    BOOL (*DevProbe)(struct DeviceNode* pDev, DriverMatchMethod* pClamationMethodRef);
    BOOL (*DevInit)(struct DeviceNode* pDev);
    VOID (*DevShutdown)(struct DeviceNode* pDev);

    BOOL bRegistered;
} Driver;

BOOL RegisterDriver(Driver* pDriver);

#endif // !YCH_KERNEL_DEVDRIV_DRIVER_H
