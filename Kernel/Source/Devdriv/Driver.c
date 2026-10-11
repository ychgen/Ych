#include "Devdriv/Driver.h"

#include "Core/KernelState.h"

#include "Memory/Krnlheap.h"

#define INITIAL_DRIVERS_RESERVE_COUNT 32

BOOL RegisterDriver(Driver* pDriver)
{
    if (!pDriver || pDriver->bRegistered)
    {
        return FALSE;
    }

    if (!g_KernelState.DriverInfo.__NrCap)
    {
        g_KernelState.DriverInfo.pTable = HeapAcquire(INITIAL_DRIVERS_RESERVE_COUNT * sizeof(Driver*));
        if (!g_KernelState.DriverInfo.pTable)
        {
            return FALSE;
        }
        g_KernelState.DriverInfo.__NrCap = INITIAL_DRIVERS_RESERVE_COUNT;
        g_KernelState.DriverInfo.NrLoaded = 0;
    }
    else if (g_KernelState.DriverInfo.NrLoaded == g_KernelState.DriverInfo.__NrCap)
    {
        g_KernelState.DriverInfo.pTable = HeapReAcquire(g_KernelState.DriverInfo.pTable, sizeof(Driver*) * (g_KernelState.DriverInfo.__NrCap + INITIAL_DRIVERS_RESERVE_COUNT));
        if (!g_KernelState.DriverInfo.pTable)
        {
            return FALSE;
        }
        g_KernelState.DriverInfo.__NrCap += INITIAL_DRIVERS_RESERVE_COUNT;
    }

    g_KernelState.DriverInfo.pTable[g_KernelState.DriverInfo.NrLoaded++] = pDriver;
    pDriver->bRegistered = TRUE;

    return TRUE;
}
