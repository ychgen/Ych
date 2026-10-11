#ifndef YCH_KERNEL_DEVDRIV_INTERFACES_BLOCK_DEVICE_DRIVER_H
#define YCH_KERNEL_DEVDRIV_INTERFACES_BLOCK_DEVICE_DRIVER_H

#include "Devdriv/Driver.h"

typedef struct
{
    Driver Object;

    UINT (*GetSectorSize)(struct DeviceNode* pDev);
    ULONG (*GetSectorCount)(struct DeviceNode* pDev);
} BlockDeviceDriver;

#endif // !YCH_KERNEL_DEVDRIV_INTERFACES_BLOCK_DEVICE_DRIVER_H
