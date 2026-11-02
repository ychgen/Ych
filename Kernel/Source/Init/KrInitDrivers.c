#include "Init/KrInitDrivers.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

#include "Core/KernelState.h"

#include "Devdriv/BlockDevice/NVMe.h"
#include "Devdriv/Timer/AcpiPmTimer.h"
#include "Devdriv/Keyboard/PS2Keyboard.h"

#include "Devdriv/Device.h"

VOID InitBuiltinDrivers(VOID)
{
    RegisterDriver(&g_GenericNvmeDriver.Object);
    RegisterDriver(&g_AcpiPmTimerDriver.Object);
    RegisterDriver(&g_PS2_KeyboardDriver.Object);
}