#include "Devdriv/Timer/AcpiPmTimer.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

#include "Devdriv/Device.h"
#include "KRTL/Krnlmem.h"
#include "Memory/MMIO.h"
#include "CPU/PortIO.h"
#include "ACPI/ACPI.h"

static UINTPTR sg_AcpiPmTimerAddr = 0;
static UINT sg_AcpiPmTimerBitWidth = 0;
static BYTE sg_AcpiPmTimerAddrSpace = -1;

DriverMatchMethod g_AcpiPmTimerDriverMatchMethods[] = {
    { .OnBus = BUS_IVLD } // terminator
};
TimerDeviceDriver g_AcpiPmTimerDriver = {
    .Object = {
        .Name = "ACPI Power Management Timer Driver",
        .Type = DRIVER_TIMER,
        .pInterface = &g_AcpiPmTimerDriver,
        .pMatchMethods = g_AcpiPmTimerDriverMatchMethods,
        .DevProbe = AcpiPmTimerDevProbe,
        .DevInit = AcpiPmTimerDevInit,
        .DevShutdown = AcpiPmTimerDevShutdown
    },
    .GetTicks = AcpiPmTimerGetTicks,
    .GetTimerBitWidth = AcpiPmTimerGetTimerBitWidth,
    .GetTimerFrequency = AcpiPmTimerGetTimerFrequency
};

BOOL AcpiPmTimerDevProbe(struct DeviceNode* pDev, DriverMatchMethod* pClamationMethodRef)
{
    return KrtlBufferEqual(pDev->Name, "ACPI Power Management Timer", 28) && pDev->OnBus == BUS_PLATFORM;
}

BOOL AcpiPmTimerDevInit(struct DeviceNode* pDev)
{
    AcpiFadtHeader* pFadtHeader = (AcpiFadtHeader*) pDev->pDriverData;
    if (pFadtHeader->PMTimerLength != 4)
    {
        return FALSE;
    }
    sg_AcpiPmTimerBitWidth = (pFadtHeader->Flags & ACPI_FADT_FLAG_PM_TIMER_CNTR_IS_32BITS) ? 32 : 24;
    
    if (pFadtHeader->X_PMTimerBlock.AddrSpace == ACPI_ADDR_SPACE_MEMORY)
    {
        sg_AcpiPmTimerAddr = MmioMapDevice("ACPI PM Timer Tick", pFadtHeader->X_PMTimerBlock.Address, 4096, MMIO_NF);
        if (!sg_AcpiPmTimerAddr)
        {
            return FALSE;
        }
        sg_AcpiPmTimerAddrSpace = ACPI_ADDR_SPACE_MEMORY;
    }
    else
    {
        sg_AcpiPmTimerAddr = pFadtHeader->X_PMTimerBlock.Address;
        sg_AcpiPmTimerAddrSpace = ACPI_ADDR_SPACE_PORT;
    }

    return TRUE;
}

VOID AcpiPmTimerDevShutdown(struct DeviceNode* pDev)
{
    sg_AcpiPmTimerAddr = 0;
    sg_AcpiPmTimerAddrSpace = -1;
    sg_AcpiPmTimerBitWidth = 0;
}

VOID AcpiPmTimerGetTicks(VOID* pOutData)
{
    if (sg_AcpiPmTimerAddrSpace == ACPI_ADDR_SPACE_MEMORY)
    {
        *(volatile DWORD*) pOutData = *(volatile DWORD*) sg_AcpiPmTimerAddr;
    }
    else
    {
        *(volatile DWORD*) pOutData = DwordInputFromPort((WORD) sg_AcpiPmTimerAddr);
    }
}

UINT AcpiPmTimerGetTimerBitWidth(VOID)
{
    return sg_AcpiPmTimerBitWidth;
}

UINT AcpiPmTimerGetTimerFrequency(VOID)
{
    return 3579545;
}
