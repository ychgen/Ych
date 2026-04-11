#include "CPU/IDT.h"

void KrEncodeInterruptDescriptor(void* pDest, uint64_t offset, uint16_t ssl, uint8_t ist, uint8_t gateType, uint8_t dpl)
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

KrInterruptDescriptor KrConstructInterruptDescriptor(uint64_t offset, uint16_t ssl, uint8_t ist, uint8_t gateType, uint8_t dpl)
{
    KrInterruptDescriptor desc;
    KrEncodeInterruptDescriptor(&desc, offset, ssl, ist, gateType, dpl);
    return desc;
}

void KrLoadInterruptDescriptorTable(const KrInterruptDescriptorTableRegister* rData)
{
    __asm__ __volatile__
    (
        "lidt %0\n\t"
         :
         : "m"(*rData)
    );
}
