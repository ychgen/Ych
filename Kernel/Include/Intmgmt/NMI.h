#ifndef YCH_KERNEL_INTMGMT_NMI_H
#define YCH_KERNEL_INTMGMT_NMI_H

#include "Core/KernelState.h"
#include "CPU/Interrupt.h"

typedef enum
{
    NMI_REASON_NONE = 0, // No NMI raised by kernel yet (probably hardware caused)
    NMI_REASON_HARMLESS_TEST,
    NMI_REASON_MELTDOWN
} GlobalNmiReason;

typedef struct
{
    MDCODE MdCode;
} NmiMeltdownData;

typedef struct
{
    GlobalNmiReason Reason;
    DWORD InvokingCPU;
    union // Depends on Reason
    {
        NmiMeltdownData Meltdown;
    };
    UCHAR Ack[MAX_SMP_PROCESSORS / 8];
} GlobalNmiInfo;

VOID RaiseGlobalNonmaskableInterrupt(BOOL bInterruptSelf, GlobalNmiReason Reason, VOID* pData, UINT* pOutNrCPUsAcknowledged);
VOID IntmgmtNonmaskableInterrupt(const KrInterruptFrame* pIntfr);

GlobalNmiInfo* GetGlobalNmiInfo(VOID);

#endif // !YCH_KERNEL_INTMGMT_NMI_H
