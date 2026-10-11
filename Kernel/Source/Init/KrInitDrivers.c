#include "Init/KrInitDrivers.h"

#include "Devdriv/BlockDevice/NVMe.h"

VOID InitBuiltinDrivers(VOID)
{
    RegisterDriver(&g_GenericNvmeDriver.Object);
}
