#ifndef YCH_KERNEL_CPU_IDT_H
#define YCH_KERNEL_CPU_IDT_H

#include <stdint.h>

#define KR_INTERRUPT_DESCRIPTOR_ENTRY_SIZE        16
#define KR_NUMBER_OF_INTERRUPT_DESCRIPTOR_ENTRIES 256
#define KR_INTERRUPT_DESCRIPTOR_TABLE_SIZE        ( KR_INTERRUPT_DESCRIPTOR_ENTRY_SIZE * KR_NUMBER_OF_INTERRUPT_DESCRIPTOR_ENTRIES )

#define KR_GATE_TYPE_INTERRUPT 0xE
#define KR_GATE_TYPE_TRAP      0xF

typedef struct __attribute__((packed))
{
    uint16_t OffsetLow;
    uint16_t SegmentSelector;
    uint8_t  IST;
    uint8_t  Attributes;
    uint16_t OffsetMid;
    uint32_t OffsetHigh;
    uint32_t Reserved;
} KrInterruptDescriptor;

typedef struct __attribute__((packed))
{
    uint16_t Limit;
    uint64_t Base;
} KrInterruptDescriptorTableRegister;

void KrEncodeInterruptDescriptor(void* pDest, uint64_t offset, uint16_t ssl, uint8_t ist, uint8_t gateType, uint8_t dpl);
KrInterruptDescriptor KrConstructInterruptDescriptor(uint64_t offset, uint16_t ssl, uint8_t ist, uint8_t gateType, uint8_t dpl);
void KrLoadInterruptDescriptorTable(const KrInterruptDescriptorTableRegister* rData);

#endif // !YCH_KERNEL_CPU_IDT_H
