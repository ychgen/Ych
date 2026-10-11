#ifndef YCH_KERNEL_DEVDRIV_PAIR_H
#define YCH_KERNEL_DEVDRIV_PAIR_H

#include "Devdriv/Device.h"
#include "Devdriv/Driver.h"

BOOL IsDriverCompatibleWithDevice(Driver* pDriver, DeviceNode* pDevice, DriverMatchMethod* pOutMatchMethod);
BOOL TryPairDriverWithDevice(Driver* pDriver, DeviceNode* pDevice);

UINT GlobalDevdrivPairAttempt(VOID);

#endif // !YCH_KERNEL_DEVDRIV_PAIR_H