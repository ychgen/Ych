#ifndef YCH_KERNEL_CPU_GDT_H
#define YCH_KERNEL_CPU_GDT_H

#include <stdint.h>

#define GDT_ACCESS_DPL_HIGHEST_PRIVILEGE  0x0 // Ring 0
#define GDT_ACCESS_DPL_LOWEST_PRIVILEGE   0x3 // Ring 3
#define GDT_SEGMENT_DESCRIPTOR_ENTRY_SIZE 8

// Pseudo struct representing a GDT segment descriptor. Do not write directly! Use `KrEncodeSegmentDescriptor`
typedef struct
{
    uint32_t Limit; // 20 bit limit
    uint32_t Base;  // 32 bit limit

    uint8_t Access_Accessed;   // 1 bit
    uint8_t Access_ReadWrite;  // 1 bit
    uint8_t Access_DC;         // 1 bit
    uint8_t Access_Executable; // 1 bit
    uint8_t Access_DescType;   // 1 bit
    uint8_t Access_DPL;        // 2 bit
    uint8_t Access_Present;    // 1 bit

    uint8_t Flags_Reserved;     // 1 bit
    uint8_t Flags_LongModeCode; // 1 bit
    uint8_t Flags_Size;         // 1 bit
    uint8_t Flags_Granularity;  // 1 bit
} KrGlobalDescriptorTableSegmentDescriptor;

typedef struct __attribute__((packed))
{
    uint16_t Limit;
    uint64_t Base;
} KrGlobalDescriptorTableRegister;

typedef enum
{
    ENCODE_SEGMENT_DESCRIPTOR_SUCCESS,
    ENCODE_SEGMENT_DESCRIPTOR_LIMIT_TOO_LARGE
} KrEncodeSegmentDescriptor_Result;

// Encodes a pseudo KrGlobalDescriptorTableSegmentDescriptor to an actual GDT entry format, writing to pDest.
KrEncodeSegmentDescriptor_Result KrEncodeSegmentDescriptor(void* pDest, const KrGlobalDescriptorTableSegmentDescriptor* pSrc);

/// @brief Constructs a segment selector for the Global Descriptor Table.
/// @param RPL Requested Privilege Level, min 0 and max 3.
/// @param i Index into the descriptor entries. 13 bit value at max.
/// @return Constructed segment selector, or 0 if failure due to invalid parameters.
uint16_t KrConstructSegmentSelector(uint8_t RPL, uint16_t i);

/// @brief Loads the GDTR register with given register data and reloads segment registers.
/// @param rData Data to load the GDTR register with.
/// @param sslCode Segment selector for the code segment.
/// @param sslData Segment selector for the data segment.
void KrLoadGlobalDescriptorTable(const KrGlobalDescriptorTableRegister* rData, uint16_t sslCode, uint16_t sslData);
// NOTE: KrLoadGlobalDescriptorTable is implemented in GDT.asm

#endif // !YCH_KERNEL_GDT_H
