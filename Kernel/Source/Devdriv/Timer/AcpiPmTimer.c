#include "Devdriv/Timer/AcpiPmTimer.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"
#include "Core/KernelState.h"

#include "Devdriv/Device.h"
#include "KRTL/Krnlmem.h"
#include "Memory/MMIO.h"
#include "CPU/PortIO.h"
#include "ACPI/ACPI.h"

CONST CHAR AcpiPmTimerDeviceName[] = "ACPI Power Management Timer";

STATIC DeviceNode AcpiPmTimerDevice = { 0 };
STATIC BOOL       AcpiPmTimerDeviceInitialized = FALSE;
STATIC UINTPTR    AcpiPmTimerAddr = 0;
STATIC UINT       AcpiPmTimerBitWidth = 0;
STATIC BYTE       AcpiPmTimerAddrSpace = -1;

STATIC BOOL AcpiPmTimerDriverRegister(VOID);
STATIC VOID AcpiPmTimerDriverUnregister(VOID);

TimerDeviceDriver g_AcpiPmTimerDriver = {
    .Object = {
        .Name = "ACPI Power Management Timer Driver",
        .Type = DRIVER_TIMER,
        .pInterface = &g_AcpiPmTimerDriver,
        .pMatchMethods = NULLPTR,
        .DevProbe = NULLPTR,
        .DevInit = NULLPTR,
        .DevShutdown = NULLPTR,
        .Register = AcpiPmTimerDriverRegister
    },
    .GetTicks = AcpiPmTimerGetTicks,
    .GetTimerBitWidth = AcpiPmTimerGetTimerBitWidth,
    .GetTimerFrequency = AcpiPmTimerGetTimerFrequency
};

STATIC BOOL AcpiPmTimerDriverRegister(VOID)
{
    AcpiPmTimerDevice.pDriver = &g_AcpiPmTimerDriver.Object;

    if (!AcpiPmTimerDeviceInitialized)
    {
        KrtlContiguousCopyBuffer(AcpiPmTimerDevice.Name, AcpiPmTimerDeviceName, sizeof(AcpiPmTimerDeviceName));
        AcpiPmTimerDevice.pDriverData = g_KernelState.AcpiInfo.pFadt;
        AcpiPmTimerDevice.OnBus = BUS_PLATFORM;

        AddChildDevice(GetSystemRootDevice(), &AcpiPmTimerDevice);
        AcpiPmTimerDeviceInitialized = TRUE;
    }

    if (g_KernelState.AcpiInfo.pFadt->PMTimerLength != 4)
    {
        return FALSE;
    }
    AcpiPmTimerBitWidth = (g_KernelState.AcpiInfo.pFadt->Flags & ACPI_FADT_FLAG_PM_TIMER_CNTR_IS_32BITS) ? 32 : 24;
    
    if (g_KernelState.AcpiInfo.pFadt->X_PMTimerBlock.AddrSpace == ACPI_ADDR_SPACE_MEMORY)
    {
        AcpiPmTimerAddr = MmioMapDevice("ACPI PM Timer Tick", g_KernelState.AcpiInfo.pFadt->X_PMTimerBlock.Address, 4096, MMIO_NF);
        if (!AcpiPmTimerAddr)
        {
            return FALSE;
        }
        AcpiPmTimerAddrSpace = ACPI_ADDR_SPACE_MEMORY;
    }
    else
    {
        AcpiPmTimerAddr = g_KernelState.AcpiInfo.pFadt->X_PMTimerBlock.Address;
        AcpiPmTimerAddrSpace = ACPI_ADDR_SPACE_PORT;
    }

    return TRUE;
}

STATIC VOID AcpiPmTimerDriverUnregister(VOID)
{
    AcpiPmTimerAddr = 0;
    AcpiPmTimerAddrSpace = -1;
    AcpiPmTimerBitWidth = 0;
}

VOID AcpiPmTimerGetTicks(VOID* pOutData)
{
    if (AcpiPmTimerAddrSpace == ACPI_ADDR_SPACE_MEMORY)
    {
        *(volatile DWORD*) pOutData = *(volatile DWORD*) AcpiPmTimerAddr;
    }
    else
    {
        *(volatile DWORD*) pOutData = DwordInputFromPort((WORD) AcpiPmTimerAddr);
    }
}

UINT AcpiPmTimerGetTimerBitWidth(VOID)
{
    return AcpiPmTimerBitWidth;
}

UINT AcpiPmTimerGetTimerFrequency(VOID)
{
    return 3579545;
}
