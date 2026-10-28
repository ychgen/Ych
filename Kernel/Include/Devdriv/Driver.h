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
    DRIVER_INPUT,
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

typedef struct Driver
{
    CHAR Name[MAX_DRIVER_NAME];
    DriverType Type;
    VOID* pInterface;
    DriverMatchMethod* pMatchMethods;

    BOOL (*DevProbe)(struct DeviceNode* pDev, DriverMatchMethod* pClamationMethodRef);
    BOOL (*DevInit)(struct DeviceNode* pDev);
    VOID (*DevShutdown)(struct DeviceNode* pDev);

    BOOL bRegistered;
} Driver;

BOOL RegisterDriver(Driver* pDriver);

#endif // !YCH_KERNEL_DEVDRIV_DRIVER_H
