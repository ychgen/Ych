#include "Init/KrInitGDT.h"

#include "Core/KernelState.h"
#include "KRTL/Krnlmem.h"
#include "CPU/GDT.h"

/** Adjust accordingly if GDT layout changes. */
#define KR_STANDARD_GPT_NUMBER_OF_DESCRIPTOR_ENTRIES 3 // NULL        Segment Descriptor
                                                       // Kernel Code Segment Descriptor
                                                       // Kernel Data Segment Descriptor

#define KR_STANDARD_GPT_SIZE ( KR_STANDARD_GPT_NUMBER_OF_DESCRIPTOR_ENTRIES * GDT_SEGMENT_DESCRIPTOR_ENTRY_SIZE )

__attribute__((aligned(16))) uint8_t g_KrSegmentDescriptorsGDT[KR_STANDARD_GPT_SIZE];

void KrInitGDT(void)
{
    KrGlobalDescriptorTableSegmentDescriptor  pDescs[KR_STANDARD_GPT_NUMBER_OF_DESCRIPTOR_ENTRIES];
    
    KrGlobalDescriptorTableSegmentDescriptor* pNullDesc     = pDescs;
    KrGlobalDescriptorTableSegmentDescriptor* pKrnlCodeDesc = pDescs + 1;
    KrGlobalDescriptorTableSegmentDescriptor* pKrnlDataDesc = pDescs + 2;

    // NULL entry (0th entry is always null)
    KrtlContiguousZeroBuffer(pNullDesc, sizeof(KrGlobalDescriptorTableSegmentDescriptor));
    
    // Code segment entry
    pKrnlCodeDesc->Base  = 0x00000000;
    pKrnlCodeDesc->Limit = 0xFFFFF;
    pKrnlCodeDesc->Access_Accessed = 1; // Always set to one for compatibility, some CPUs like to fault.
    pKrnlCodeDesc->Access_ReadWrite = 1; // Code segment readable
    pKrnlCodeDesc->Access_DC = 0; // Conforming bit: 0 means can only be executed with specified ring in DPL
    pKrnlCodeDesc->Access_Executable = 1; // Code segment
    pKrnlCodeDesc->Access_DescType = 1; // Code/data = 1, system = 0
    pKrnlCodeDesc->Access_DPL = GDT_ACCESS_DPL_HIGHEST_PRIVILEGE; // Ring 0
    pKrnlCodeDesc->Access_Present = 1;
    pKrnlCodeDesc->Flags_Reserved = 0;
    pKrnlCodeDesc->Flags_LongModeCode = 1; // Long Mode code segment: of course
    pKrnlCodeDesc->Flags_Size = 0; // Ignored and irrelevant because LMC = 1 above, should be set to 0
    pKrnlCodeDesc->Flags_Granularity = 1; //4KiB units
    
    // Data segment entry
    pKrnlDataDesc->Base  = 0x00000000;
    pKrnlDataDesc->Limit = 0xFFFFF;
    pKrnlDataDesc->Access_Accessed = 1; // Always set to one for compatibility, some CPUs like to fault.
    pKrnlDataDesc->Access_ReadWrite = 1; // Data segment writable
    pKrnlDataDesc->Access_DC = 0; // Data segment grows up
    pKrnlDataDesc->Access_Executable = 0; // Data segment
    pKrnlDataDesc->Access_DescType = 1; // Code/data = 1, system = 0
    pKrnlDataDesc->Access_DPL = GDT_ACCESS_DPL_HIGHEST_PRIVILEGE; // Ring 0
    pKrnlDataDesc->Access_Present = 1;
    pKrnlDataDesc->Flags_Reserved = 0;
    pKrnlDataDesc->Flags_LongModeCode = 0;
    pKrnlDataDesc->Flags_Size = 0; // set to 0 for data segments
    pKrnlDataDesc->Flags_Granularity = 1; //4KiB units

    for (int i = 0; i < 3; i++)
    {
        KrEncodeSegmentDescriptor((void*) g_KrSegmentDescriptorsGDT + GDT_SEGMENT_DESCRIPTOR_ENTRY_SIZE * i, pDescs + i);
    }
    uint16_t sslKrnlCode = KrConstructSegmentSelector(GDT_ACCESS_DPL_HIGHEST_PRIVILEGE, 1); // 1st Entry, Ring 0
    uint16_t sslKrnlData = KrConstructSegmentSelector(GDT_ACCESS_DPL_HIGHEST_PRIVILEGE, 2); // 2nd Entry, Ring 0

    KrGlobalDescriptorTableRegister GDTR;
    GDTR.Limit = KR_STANDARD_GPT_SIZE - 1;
    GDTR.Base = (uint64_t) g_KrSegmentDescriptorsGDT;
    KrLoadGlobalDescriptorTable(&GDTR, sslKrnlCode, sslKrnlData);

    g_KernelState.StateGDT.AddrDescriptorTable       = (uintptr_t) g_KrSegmentDescriptorsGDT;
    g_KernelState.StateGDT.NumberOfDescriptorEntries = KR_STANDARD_GPT_NUMBER_OF_DESCRIPTOR_ENTRIES;
    g_KernelState.StateGDT.KernelCodeSegmentSelector = sslKrnlCode;
    g_KernelState.StateGDT.KernelDataSegmentSelector = sslKrnlData;
}
