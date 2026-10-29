#ifndef YCH_KERNEL_INTMGMT_IOAPIC_H
#define YCH_KERNEL_INTMGMT_IOAPIC_H

#include "Krnlych.h"

#define MAX_IO_APICS 256

typedef struct
{
    UINTPTR PaddrBase, VaddrBase;
    DWORD ID;
    DWORD FirstIRQ; // Global System Interrupt Base

    UCHAR Version;
    UCHAR MaxRedirEntry;

    BOOL E_V  : 1;
    BOOL INIT : 1;
} IoApicInfo;

BOOL IoApicsInit(VOID);

BOOL SetRedirectIRQ(DWORD IRQ, DWORD ProcessorID, UCHAR IntVector);
BOOL SetEnableIRQ(DWORD IRQ, BOOL bEnable);
IoApicInfo* FindIoApicForIRQ(DWORD IRQ);

VOID _SetIoApicCount(DWORD NrIoApics);
IoApicInfo* GetIoApics(VOID);
DWORD* GetIoApicIDs(VOID);
UINT GetIoApicCount(VOID);

#endif // !YCH_KERNEL_INTMGMT_IOAPIC_H
