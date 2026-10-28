#ifndef YCH_KERNEL_DEVDRIV_TIMER_ACPI_PM_TIMER_H
#define YCH_KERNEL_DEVDRIV_TIMER_ACPI_PM_TIMER_H

#include "Devdriv/Interfaces/TimerDeviceDriver.h"

extern TimerDeviceDriver g_AcpiPmTimerDriver;

BOOL AcpiPmTimerDevProbe(struct DeviceNode* pDev, DriverMatchMethod* pClamationMethodRef);
BOOL AcpiPmTimerDevInit(struct DeviceNode* pDev);
VOID AcpiPmTimerDevShutdown(struct DeviceNode* pDev);

VOID AcpiPmTimerGetTicks(VOID* pOutData);
UINT AcpiPmTimerGetTimerBitWidth(VOID);
UINT AcpiPmTimerGetTimerFrequency(VOID);

#endif // !YCH_KERNEL_DEVDRIV_TIMER_ACPI_PM_TIMER_H
