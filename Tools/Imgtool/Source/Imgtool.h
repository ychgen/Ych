#ifndef YCH_TOOLS_IMGTOOL_H
#define YCH_TOOLS_IMGTOOL_H

#include <stddef.h>
#include <stdint.h>

#define MBR_NUMBER_OF_PARTITIONS     4
#define MBR_BOOT_INDICATOR_NO        0x00
#define MBR_BOOT_INDICATOR_BOOTABLE  0x80
#define MBR_SYSTEM_ID_GPT_PROTECTIVE 0xEE
#define MBR_PARTITION_TABLE_OFFSET   0x01BE
#define MBR_SIGNATURE_OFFSET         0x01FE
#define MBR_SIGNATURE                0xAA55

typedef struct __attribute__((packed))
{
    uint8_t  BootIndicator; // 0x80 = bootable
    uint8_t  StartCHS[3];
    uint8_t  SystemID; // If this is 0, it means partition is unused.
    uint8_t  EndCHS[3];
    uint32_t StartLBA;
    uint32_t Size; // Total sectors in partition
} MBR_PARTITION_ENTRY;

typedef MBR_PARTITION_ENTRY MBR_PARTITION_TABLE[MBR_NUMBER_OF_PARTITIONS];

#define GPT_PARTITION_HEADER_SIGNATURE    "EFI PART"
#define GPT_INITIAL_REVISION               0x00010000

#define GPT_EFI_SYSTEM_PARTITION_UUID    { 0x28, 0x73, 0x2A, 0xC1, 0x1F, 0xF8, 0xD2, 0x11, 0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B }
#define GPT_MICROSOFT_BASIC_DATA         { 0xA2, 0xA0, 0xD0, 0xEB, 0xE5, 0xB9, 0x33, 0x44, 0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7 }

#define GPT_ATTRIBUTE_REQUIRED           ( 1 << 0 )
#define GPT_ATTRIBUTE_NO_BLOCK_IO        ( 1 << 1 )
#define GPT_ATTRIBUTE_LEGACY_BOOT        ( 1 << 2 )

#define GPT_PARTITION_NAME_BYTES_PER_CHAR  2
#define GPT_PARTITION_NAME_CHAR_COUNT      36
#define GPT_PARTITION_NAME_SIZE           ( GPT_PARTITION_NAME_CHAR_COUNT * GPT_PARTITION_NAME_BYTES_PER_CHAR )

// Checksums use CCITT32 ANSI CRC
typedef struct __attribute__((packed))
{
    uint8_t  Signature[8];
    uint32_t Revision;
    uint32_t HeaderSize;
    uint32_t Checksum; // CRC32
    uint8_t  Reserved[4];
    uint64_t SelfLBA; // LBA of this header
    uint64_t AlternateLBA; // LBA of the other header
    uint64_t FirstUsableLBA;
    uint64_t LastUsableLBA;
    uint8_t  DiskGUID[16];
    uint64_t PartitionTableLBA; // Start LBA of Partition Entry Array
    uint32_t NoPartitionEntries; // Number of partition entries
    uint32_t BytesPerPartitionEntry; // Number of bytes per each entry in the partition entry array
    uint32_t PartitionTableChecksum; // CRC32 of partition entry array
} GPT_PARTITION_HEADER;

typedef struct __attribute__((packed))
{
    uint8_t  PartitionTypeGUID[16]; // Zero = unused entry, set GUID of what sort of partition this is
    uint8_t  PartitionGUID[16];     // GUID of this specific partition entry
    uint64_t StartLBA;
    uint64_t EndLBA;
    uint64_t Attributes;
    uint8_t  PartitionName[GPT_PARTITION_NAME_SIZE];
} GPT_PARTITION_ENTRY;

void Imgtool_LBAToCHS(uint8_t* pCHS, uint64_t LBA);
void Imgtool_WriteASCIIToUNICODE16LE(uint8_t* pDest, const char* pSrc);

void InitTableCRC32(void);
uint32_t ChecksumCRC32(const void* pData, size_t szData);

#endif // !YCH_TOOLS_IMGTOOL_H
