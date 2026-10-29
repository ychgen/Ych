#include "Intmgmt/IOAPIC.h"

#include "Core/KernelState.h"
#include "Memory/MMIO.h"

#define IOREGSEL 0x00 // I/O Register Select
#define IOREGWIN 0x10 // I/O Register Window

#define IOAPICID  0x0
#define IOAPICVER 0x1
#define IOREDTBL  0x10 // For First IRQ

#define IRQMASK   (1UL << 16) // Set: Disable, Clear: Enable

STATIC IoApicInfo IoApics[MAX_IO_APICS] = {0};
STATIC DWORD IoApicIDs[MAX_IO_APICS] = {0};
STATIC UINT IoApicCount = 0;

STATIC DWORD IoApicRead(IoApicInfo* IoApic, UINT Regidx);
STATIC QWORD IoApicRead64(IoApicInfo* IoApic, UINT Regidx);
STATIC VOID  IoApicWrite(IoApicInfo* IoApic, UINT Regidx, DWORD dwValue);
STATIC VOID  IoApicWrite64(IoApicInfo* IoApic, UINT Regidx, QWORD qwValue);

BOOL IoApicsInit(VOID)
{
    for (UINT i = 0; i < IoApicCount; i++)
    {
        IoApicInfo* IoApic = IoApics + IoApicIDs[i];
        if (IoApic->INIT)
        {
            continue;
        }

        IoApic->VaddrBase = MmioMapDevice("I/O APIC", IoApic->PaddrBase, 4096, MMIO_NF);
        if (!IoApic->VaddrBase)
        {
            return FALSE;
        }

        DWORD dwVerReg = IoApicRead(IoApic, IOAPICVER);
        IoApic->Version = dwVerReg & 0xFF;
        IoApic->MaxRedirEntry = (dwVerReg >> 16) & 0xFF;

        IoApic->INIT = TRUE;
    }

    return TRUE;
}

BOOL SetRedirectIRQ(DWORD IRQ, DWORD ProcessorID, UCHAR IntVector)
{
    IoApicInfo* IoApic = FindIoApicForIRQ(IRQ);
    if (!IoApic || ProcessorID >= MAX_SMP_PROCESSORS)
    {
        return FALSE;
    }

    // Delivery Mode = Fixed, Destination Mode = Physical, Pin Polarity = Active-High, Trigger-Mode = Edge
    // Masked = Off (IRQ Active)
    CONST QWORD RedirEntry = IntVector | (((QWORD) ProcessorID) << 56);
    CONST DWORD N = IRQ - IoApic->FirstIRQ;
    CONST DWORD Regidx = IOREDTBL + N * 2;

    IoApicWrite64(IoApic, Regidx, RedirEntry);
    return TRUE;
}

BOOL SetEnableIRQ(DWORD IRQ, BOOL bEnable)
{
    IoApicInfo* IoApic = FindIoApicForIRQ(IRQ);
    if (!IoApic)
    {
        return FALSE;
    }

    CONST DWORD N = IRQ - IoApic->FirstIRQ;
    CONST DWORD Regidx = IOREDTBL + N * 2;

    QWORD RedirEntry = IoApicRead64(IoApic, Regidx);
    if (bEnable)
    {
        RedirEntry &= ~IRQMASK;
    }
    else
    {
        RedirEntry |= IRQMASK;
    }
    IoApicWrite64(IoApic, Regidx, RedirEntry);

    return TRUE;
}

IoApicInfo* FindIoApicForIRQ(DWORD IRQ)
{
    for (UINT i = 0; i < IoApicCount; i++)
    {
        IoApicInfo* CONST IoApic = IoApics + IoApicIDs[i];
        if (!(IoApic->E_V && IoApic->INIT))
        {
            continue;
        }
        CONST DWORD MaxIRQ = IoApic->FirstIRQ + IoApic->MaxRedirEntry;
        if (IRQ >= IoApic->FirstIRQ && IRQ <= MaxIRQ)
        {
            return IoApic;
        }
    }
    return NULLPTR;
}

VOID _SetIoApicCount(DWORD NrIoApics)
{
    IoApicCount = NrIoApics;
}

IoApicInfo* GetIoApics(VOID)
{
    return IoApics;
}

DWORD* GetIoApicIDs(VOID)
{
    return IoApicIDs;
}

UINT GetIoApicCount(VOID)
{
    return IoApicCount;
}

STATIC DWORD IoApicRead(IoApicInfo* IoApic, UINT Regidx)
{
    MmioWrite32(IoApic->VaddrBase, IOREGSEL, Regidx);
    return MmioRead32(IoApic->VaddrBase, IOREGWIN);
}

STATIC QWORD IoApicRead64(IoApicInfo* IoApic, UINT Regidx)
{
    DWORD Lodword = IoApicRead(IoApic, Regidx);
    DWORD Hidword = IoApicRead(IoApic, Regidx + 1);

    return ((QWORD) Hidword) << 32 | (QWORD) Lodword;
}

STATIC VOID IoApicWrite(IoApicInfo* IoApic, UINT Regidx, DWORD dwValue)
{
    MmioWrite32(IoApic->VaddrBase, IOREGSEL, Regidx);
    MmioWrite32(IoApic->VaddrBase, IOREGWIN, dwValue);
}

STATIC VOID IoApicWrite64(IoApicInfo* IoApic, UINT Regidx, QWORD qwValue)
{
    IoApicWrite(IoApic, Regidx, (DWORD) qwValue);
    IoApicWrite(IoApic, Regidx + 1, (DWORD)(qwValue >> 32));
}
