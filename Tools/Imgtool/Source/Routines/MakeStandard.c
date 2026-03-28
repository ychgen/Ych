#include "RoutineLib.h"
#include "Imgtool.h"

#include <uuid/uuid.h>

#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#define NO_PARTITIONS_TO_ALLOCATE  128 // Number of possible GPT partition entries
#define MINIMUM_DISK_SIZE_MIB      128 // In MiB, minimum disk size allowed
#define EFI_SYSTEM_PART_ADDRESS   (1024 * 1024) // 1 MiB
#define EFI_SYSTEM_PART_SIZE_MIB   100 // In MiB

#define STD_LAYOUT_PARTITION_COUNT 2

#define print(...) printf("Imgtool.MakeStandard: " __VA_ARGS__)

FILE* pDisk = NULL;
uint8_t* pPartitionEntryArray = NULL;

int MakeStd_AbortSafely()
{
    if (pDisk) { fclose(pDisk); pDisk = NULL; }
    if (pPartitionEntryArray) { free(pPartitionEntryArray); pPartitionEntryArray = NULL; }
    return ROUTINE_FAIL;\
}

int Routine_MakeStandard(const Routine* pSelf)
{
    const char* pDiskPath  = pSelf->Parameters[0].StrValue;
          int   iDiskSize  = pSelf->Parameters[1].IntValue; // In MiB
          int   iBlockSize = pSelf->Parameters[2].IntValue;

    if (iDiskSize < MINIMUM_DISK_SIZE_MIB)
    {
        print("Disk size must be at least %d MiB, however %d MiB was provided.\n", MINIMUM_DISK_SIZE_MIB, iDiskSize);
        return ROUTINE_FAIL;
    }

    if (iBlockSize > 0 && iBlockSize % 512)
    {
        print("Block size must be positive and must be a multiple of 512, the provided value of %d is not.\n", iBlockSize);
        return ROUTINE_FAIL;
    }

    size_t rawDiskSize = ((size_t) iDiskSize) * 1024 * 1024; // MiB to bytes
    size_t blockCount  = (rawDiskSize + ((size_t) iBlockSize) - 1) / (size_t) iBlockSize;

    pDisk = fopen(pDiskPath, "wb");
    if (!pDisk)
    {
        print("Failed to open disk `%s`!\n", pDiskPath);
        return ROUTINE_FAIL;
    }

    if (ftruncate(fileno(pDisk), blockCount * iBlockSize) != 0)
    {
        print("Failed to truncate disk `%s` up to %d MiB.\n", pDiskPath, iDiskSize);
        return MakeStd_AbortSafely();
    }

    // MBR lives in the first sector, therefore we can count it at address 0
    // Partition Table starts at offset 0x01BE into the MBR. So 0 + 0x01BE, we seek to 0x01BE or 446 in dec (MBR_PARTITION_TABLE_OFFSET)
    // We are only going to write 1 entry so we seek to the first entry's address
    if (fseek(pDisk, MBR_PARTITION_TABLE_OFFSET, SEEK_SET) != 0)
    {
        print("Failed to seek to the MBR Partition Table at offset 0x%X into disk `%s`.\n", MBR_PARTITION_TABLE_OFFSET, pDiskPath);
        return MakeStd_AbortSafely();
    }
    {
        // Per GPT specification, we have one MBR partition entry with the following values:
        MBR_PARTITION_ENTRY parGPT;
        parGPT.BootIndicator = MBR_BOOT_INDICATOR_NO;
        parGPT.SystemID      = MBR_SYSTEM_ID_GPT_PROTECTIVE;
        parGPT.StartLBA      = 1; // LBA of GPT Partition Header
        parGPT.Size          = blockCount - 1; // `Set to the size in logical blocks of the disk, minus one`
        // Populate CHS fields
        Imgtool_LBAToCHS(parGPT.StartCHS, parGPT.StartLBA);
        Imgtool_LBAToCHS(parGPT.EndCHS, parGPT.Size);

        if (fwrite(&parGPT, 1, sizeof(MBR_PARTITION_ENTRY), pDisk) != sizeof(MBR_PARTITION_ENTRY))
        {
            print("Failed to write the protective MBR onto disk `%s`.\n", pDiskPath);
            return MakeStd_AbortSafely();
        }
        // Write signature
        if (fseek(pDisk, MBR_SIGNATURE_OFFSET, SEEK_SET) != 0)
        {
            print("Failed to seek to the MBR signature at offset 0x%X into disk `%s`.\n", MBR_SIGNATURE, pDiskPath);
            return MakeStd_AbortSafely();
        }
        uint16_t signature = MBR_SIGNATURE;
        if (fwrite(&signature, 1, sizeof(uint16_t), pDisk) != sizeof(uint16_t))
        {
            print("Failed to write the MBR signature onto disk `%s`.\n", pDiskPath);
            return MakeStd_AbortSafely();
        }
    }

    // calculate how many blocks the partition table will occupy
    size_t partitionTableBlockCount = (NO_PARTITIONS_TO_ALLOCATE * sizeof(GPT_PARTITION_ENTRY) + iBlockSize - 1) / iBlockSize;

    GPT_PARTITION_HEADER header;
    memcpy(header.Signature, GPT_PARTITION_HEADER_SIGNATURE, 8);
    header.Revision = GPT_INITIAL_REVISION;
    header.HeaderSize = sizeof(GPT_PARTITION_HEADER);
    header.Checksum = 0; // calculated later
    memset(header.Reserved, 0, 4);
    header.SelfLBA = 1;
    header.AlternateLBA = blockCount - 1; // Mirror header resides on last addressable block
    header.FirstUsableLBA = 2 + partitionTableBlockCount; // +2(1 for PMBR, 2 for GPTheader) + partition table blocks
    header.LastUsableLBA = header.AlternateLBA - 1 - partitionTableBlockCount;
    uuid_generate(header.DiskGUID); // Disk GUID
    header.PartitionTableLBA = 2; // right after primary gpt pt header
    header.NoPartitionEntries = NO_PARTITIONS_TO_ALLOCATE;
    header.BytesPerPartitionEntry = sizeof(GPT_PARTITION_ENTRY);
    header.PartitionTableChecksum = 0; // set later

    // Allocate memory to represent partition entry array
    size_t partEntryArrayRawSize = header.NoPartitionEntries * header.BytesPerPartitionEntry;
    pPartitionEntryArray = malloc(partEntryArrayRawSize);
    if (!pPartitionEntryArray)
    {
        printf("Failed to allocate memory for partition entry array representation.\n");
        return MakeStd_AbortSafely();
    }

    // Store part type GUIDs
    uint8_t efiSystemPartGUID[16] = GPT_EFI_SYSTEM_PARTITION_UUID;
    uint8_t fat32PartGUID[16] = GPT_MICROSOFT_BASIC_DATA;

    // Create the Ych OS standard layout, one EFI system partition, one OS main partition.
    GPT_PARTITION_ENTRY* partEFI = ((GPT_PARTITION_ENTRY*) pPartitionEntryArray);
    GPT_PARTITION_ENTRY* partOS  = partEFI + 1;

    memcpy(partEFI->PartitionTypeGUID, efiSystemPartGUID, 16);
    uuid_generate(partEFI->PartitionGUID); // unique partition GUID
    partEFI->StartLBA   = EFI_SYSTEM_PART_ADDRESS / iBlockSize; // In Ych OS standard layout, EFI system partition starts at 1MiB mark
    partEFI->EndLBA     = partEFI->StartLBA + ((EFI_SYSTEM_PART_SIZE_MIB * 1024 * 1024) / iBlockSize) - 1; // In Ych OS standard layout, EFI system partition spans across 100 MiB. -1 for inclusive LBA
    partEFI->Attributes = GPT_ATTRIBUTE_REQUIRED;
    Imgtool_WriteASCIIToUNICODE16LE(partEFI->PartitionName, "EFI System Partition");

    memcpy(partOS->PartitionTypeGUID, fat32PartGUID, 16);
    uuid_generate(partOS->PartitionGUID); // unique partition GUID
    partOS->StartLBA = partEFI->EndLBA + 1; // Next LBA after EFI System Partition
    partOS->EndLBA = header.LastUsableLBA; // Spans across the entire disk.
    partOS->Attributes = GPT_ATTRIBUTE_REQUIRED;
    Imgtool_WriteASCIIToUNICODE16LE(partOS->PartitionName, "Ych Operating System");

    // Zero rest of the array space (must be done)
    memset((void*)(partEFI + STD_LAYOUT_PARTITION_COUNT), 0, partEntryArrayRawSize - sizeof(GPT_PARTITION_ENTRY) * STD_LAYOUT_PARTITION_COUNT);

    // Checksum calculated over `header.NoPartitionEntries * header.BytesPerPartitionEntry` even over non touched allocated spaces.
    header.PartitionTableChecksum = ChecksumCRC32(pPartitionEntryArray, partEntryArrayRawSize);
    // Now calculate header checksum (Checksum field already zeroed, see above)
    header.Checksum = ChecksumCRC32(&header, header.HeaderSize);

    // Alternate header (at last addressable LBA)
    GPT_PARTITION_HEADER alternate = header;
    alternate.SelfLBA = header.AlternateLBA;
    alternate.AlternateLBA = header.SelfLBA;
    alternate.PartitionTableLBA = header.LastUsableLBA + 1; // mirrored ptable
    
    // Checksum of alternate header
    alternate.Checksum = 0;
    alternate.Checksum = ChecksumCRC32(&alternate, alternate.HeaderSize);

    // Write headers to disk (both headers)
    if (fseek(pDisk, header.SelfLBA * iBlockSize, SEEK_SET) != 0)
    {
        print("Failed to seek to primary GPT partition table header on disk.");
        return MakeStd_AbortSafely();
    }
    if (fwrite(&header, 1, sizeof(GPT_PARTITION_HEADER), pDisk) != sizeof(GPT_PARTITION_HEADER))
    {
        print("Failed to write the primary GPT partition table header to disk.");
        return MakeStd_AbortSafely();
    }
    // Mirror header
    if (fseek(pDisk, alternate.SelfLBA * iBlockSize, SEEK_SET) != 0)
    {
        print("Failed to seek to mirror GPT partition table header on disk.");
        return MakeStd_AbortSafely();
    }
    if (fwrite(&alternate, 1, sizeof(GPT_PARTITION_HEADER), pDisk) != sizeof(GPT_PARTITION_HEADER))
    {
        print("Failed to write the mirror GPT partition table header to disk.");
        return MakeStd_AbortSafely();
    }

    // Write partition table to disk (at both proper locations)
    if (fseek(pDisk, header.PartitionTableLBA * iBlockSize, SEEK_SET) != 0)
    {
        print("Failed to seek to partition entry array on disk.");
        return MakeStd_AbortSafely();
    }
    if (fwrite(pPartitionEntryArray, sizeof(GPT_PARTITION_ENTRY), STD_LAYOUT_PARTITION_COUNT, pDisk) != STD_LAYOUT_PARTITION_COUNT)
    {
        print("Failed to write partition entry array to disk.");
    }
    // Mirror table (by using alternate header's ptablelba field)
    if (fseek(pDisk, alternate.PartitionTableLBA * iBlockSize, SEEK_SET) != 0)
    {
        print("Failed to seek to mirror partition entry array on disk.");
        return MakeStd_AbortSafely();
    }
    if (fwrite(pPartitionEntryArray, sizeof(GPT_PARTITION_ENTRY), STD_LAYOUT_PARTITION_COUNT, pDisk) != STD_LAYOUT_PARTITION_COUNT)
    {
        print("Failed to write mirror partition entry array to disk.");
        return MakeStd_AbortSafely();
    }
    
    MakeStd_AbortSafely();
    return ROUTINE_OK;
}
