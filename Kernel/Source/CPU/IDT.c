#include "CPU/IDT.h"

VOID KrEncodeInterruptDescriptor(VOID* pDest, QWORD offset, WORD ssl, BYTE ist, BYTE gateType, BYTE dpl)
{
    KrInterruptDescriptor* desc = (KrInterruptDescriptor*) pDest;
    desc->OffsetLow = offset & 0xFFFF; // low 16 bits
    desc->OffsetMid = (offset >> 16) & 0xFFFF; // mid 16 bits
    desc->OffsetHigh = (offset >> 32) & 0xFFFFFFFF;
    desc->SegmentSelector = ssl;
    desc->IST = ist & 0x7;
    desc->Attributes = (1 << 7) | ((dpl & 0x03) << 5) | (0 << 4) | (gateType & 0xF);
    desc->Reserved = 0;
}

KrInterruptDescriptor KrConstructInterruptDescriptor(QWORD offset, WORD ssl, BYTE ist, BYTE gateType, BYTE dpl)
{
    KrInterruptDescriptor desc;
    KrEncodeInterruptDescriptor(&desc, offset, ssl, ist, gateType, dpl);
    return desc;
}

VOID KrLoadInterruptDescriptorTable(const KrInterruptDescriptorTableRegister* rData)
{
    __asm__ __volatile__
    (
        "lidt %0\n\t"
         :
         : "m"(*rData)
    );
}
