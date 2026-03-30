#include "Init/KrInitGDT.h"

#include "Memory/Krnlmem.h"

__attribute__((aligned(16))) uint8_t g_KrSegmentDescriptorsGDT[KR_STANDARD_GPT_SIZE];

uint16_t g_sslCode;
uint16_t g_sslData;

void KrInitGDT(void)
{
    KrGlobalDescriptorTableSegmentDescriptor  pDescs[KR_STANDARD_GPT_NUMBER_OF_DESCRIPTOR_ENTRIES];
    KrGlobalDescriptorTableSegmentDescriptor* pNullDesc     = pDescs;
    KrGlobalDescriptorTableSegmentDescriptor* pKrnlCodeDesc = pDescs + 1;
    KrGlobalDescriptorTableSegmentDescriptor* pKrnlDataDesc = pDescs + 2;

    // NULL entry (0th entry is always null)
    KrContiguousZeroBuffer(pNullDesc, sizeof(KrGlobalDescriptorTableSegmentDescriptor));
    
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
    g_sslCode = KrConstructSegmentSelector(GDT_ACCESS_DPL_HIGHEST_PRIVILEGE, 1); // 1st Entry, Ring 0
    g_sslData = KrConstructSegmentSelector(GDT_ACCESS_DPL_HIGHEST_PRIVILEGE, 2); // 2nd Entry, Ring 0

    KrGlobalDescriptorTableRegister GDTR;
    GDTR.Limit = KR_STANDARD_GPT_SIZE - 1;
    GDTR.Base = (uint64_t) g_KrSegmentDescriptorsGDT;
    KrLoadGlobalDescriptorTable(&GDTR, g_sslCode, g_sslData);
}

uint16_t KrGetKernelCodeSegmentSelector(void)
{
    return g_sslCode;
}

uint16_t KrGetKernelDataSegmentSelector(void)
{
    return g_sslData;
}
