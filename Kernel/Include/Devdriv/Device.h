#ifndef YCH_KERNEL_DEVDRIV_DEVICE_H
#define YCH_KERNEL_DEVDRIV_DEVICE_H

#include "Devdriv/Buses.h"

#define MAX_DEVICE_NAME 128

struct Driver;

typedef struct DeviceNode
{
    CHAR Name[MAX_DEVICE_NAME];
    BusType OnBus;

    union // Context-relevant based on OnBus
    {
        PciDeviceId Pci;
    };

    struct DeviceNode* pParent;
    struct DeviceNode* p1stKid;
    struct DeviceNode* pNxtSib;

    struct Driver* pDriver; // Owning/associated/bound driver
    VOID* pDriverData;
} DeviceNode;

VOID InitDeviceTree(VOID);
BOOL InitDeviceTreeRoot(VOID);
DeviceNode* GetSystemRootDevice(VOID);
UINT GetSystemDeviceCount(VOID);

#endif // !YCH_KERNEL_DEVDRIV_DEVICE_H
