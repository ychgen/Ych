#include "Init/KrInitDrivers.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

#include "Core/KernelState.h"

#include "Devdriv/BlockDevice/NVMe.h"
#include "Devdriv/Timer/AcpiPmTimer.h"

#include "Devdriv/Device.h"

VOID InitBuiltinDrivers(VOID)
{
    RegisterDriver(&g_GenericNvmeDriver.Object);

    RegisterDriver(&g_AcpiPmTimerDriver.Object);
    {
        DeviceNode AcpiTimerDevice = {
            .Name = "ACPI Power Management Timer",
            .OnBus = BUS_PLATFORM
        };
        
        AcpiTimerDevice.pDriver = &g_AcpiPmTimerDriver.Object;
        AcpiTimerDevice.pDriverData = g_KernelState.AcpiInfo.pFadt;
        if (AcpiPmTimerDevProbe(&AcpiTimerDevice, NULLPTR))
        {
            if (AcpiPmTimerDevInit(&AcpiTimerDevice))
            {
                AddChildDevice(GetSystemRootDevice(), &AcpiTimerDevice);
            }
        }
    }
}