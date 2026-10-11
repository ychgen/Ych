#ifndef YCH_KERNEL_DEVDRIV_BLOCK_DEVICES_NVME_H
#define YCH_KERNEL_DEVDRIV_BLOCK_DEVICES_NVME_H

#include "Devdriv/Interfaces/BlockDeviceDriver.h"

extern BlockDeviceDriver g_GenericNvmeDriver;

BOOL NvmeDevProbe(struct DeviceNode* pDev, DriverMatchMethod* pClamationMethodRef);
BOOL NvmeDevInit(struct DeviceNode* pDev);
VOID NvmeDevShutdown(struct DeviceNode* pDev);

UINT NvmeGetSectorSize(struct DeviceNode* pDev);
ULONG NvmeGetSectorCount(struct DeviceNode* pDev);

#endif // !YCH_KERNEL_DEVDRIV_BLOCK_DEVICES_NVME_H
