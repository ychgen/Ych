#ifndef YCH_KERNEL_CPU_IDT_H
#define YCH_KERNEL_CPU_IDT_H

#include "Krnlych.h"

#define KR_INTERRUPT_DESCRIPTOR_ENTRY_SIZE        16
#define KR_NUMBER_OF_INTERRUPT_DESCRIPTOR_ENTRIES 256
#define KR_INTERRUPT_DESCRIPTOR_TABLE_SIZE        ( KR_INTERRUPT_DESCRIPTOR_ENTRY_SIZE * KR_NUMBER_OF_INTERRUPT_DESCRIPTOR_ENTRIES )

#define KR_GATE_TYPE_INTERRUPT 0xE
#define KR_GATE_TYPE_TRAP      0xF

typedef struct __attribute__((packed))
{
    WORD  OffsetLow;
    WORD  SegmentSelector;
    BYTE  IST;
    BYTE  Attributes;
    WORD  OffsetMid;
    DWORD OffsetHigh;
    DWORD Reserved;
} KrInterruptDescriptor;

typedef struct __attribute__((packed))
{
    WORD  Limit;
    QWORD Base;
} KrInterruptDescriptorTableRegister;

VOID KrEncodeInterruptDescriptor(VOID* pDest, QWORD offset, WORD ssl, BYTE ist, BYTE gateType, BYTE dpl);
KrInterruptDescriptor KrConstructInterruptDescriptor(QWORD offset, WORD ssl, BYTE ist, BYTE gateType, BYTE dpl);
VOID KrLoadInterruptDescriptorTable(const KrInterruptDescriptorTableRegister* rData);

#endif // !YCH_KERNEL_CPU_IDT_H
