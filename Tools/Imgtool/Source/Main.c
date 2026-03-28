#include "RoutineLib.h"
#include "Imgtool.h"

// Imgtool extern routines
#include "Routines.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    if (!RoutineLibInit())
    {
        puts("Imgtool: Failed to initialize RoutineLib!");
        return EXIT_FAILURE;
    }

    // Must be called before using ChecksumCRC32()
    InitTableCRC32();

    // Routine `MakeStandard`
    {
        Routine makestd = MakeRoutine(Routine_MakeStandard, "MakeStandard", "Creates a disk image with a standard Ych OS layout, that is an EFI partition and the system partition. The PMBR (Protective Master Boot Record) and the GPT (GUID Partition Table) will also be created.");
        AddRoutineParameter(&makestd, MakeStringRoutineParameter ("DiskPath",  "Path to the disk image, if it doesn't exist, it will be created."));
        AddRoutineParameter(&makestd, MakeIntegerRoutineParameter("DiskSize",  "The size of the disk image to be made, in MiB."));
        AddRoutineParameter(&makestd, MakeOptIntegerRoutineParameter("BlockSize", "The size of each logical block/sector of the disk, in bytes. Must be a multiple of 512.", 512));
        RegisterRoutine(&makestd);
    }
    // Routine `ReadMBR`
    {
        Routine readmbr = MakeRoutine(Routine_ReadMBR, "ReadMBR", "Reads the Master Boot Record of a given disk file, primarily its Partition Table.");
        AddRoutineParameter(&readmbr, MakeStringRoutineParameter("DiskPath", "The path to the disk image."));
        RegisterRoutine(&readmbr);
    }
    // Routine `ReadGPT`
    {
        Routine readgpt = MakeRoutine(Routine_ReadMBR, "ReadGPT", "Reads the GUID Partition Table of a given disk file, primarily its header and its Partition Table.");
        AddRoutineParameter(&readgpt, MakeStringRoutineParameter("DiskPath", "The path to the disk image."));
        AddRoutineParameter(&readgpt, MakeOptIntegerRoutineParameter("BlockSize", "The size of each logical block/sector of the disk, in bytes. Must be a multiple of 512.", 512));
        RegisterRoutine(&readgpt);
    }

    return RoutineLibExecute(argc - 1, argv + 1);
}
