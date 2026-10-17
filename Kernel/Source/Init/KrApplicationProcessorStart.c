#include "Init/KrApplicationProcessorStart.h"

#include "Core/KernelState.h"
#include "CPU/Halt.h"
#include "CPU/GDT.h"

VOID KrApplicationProcessorStart(VOID)
{
    KrGlobalDescriptorTableRegister GDTR;
    GDTR.Base = g_KernelState.StateGDT.AddrDescriptorTable;
    GDTR.Limit = g_KernelState.StateGDT.AddrDescriptorTable + g_KernelState.StateGDT.NumberOfDescriptorEntries * GDT_SEGMENT_DESCRIPTOR_ENTRY_SIZE - 1;
    KrLoadGlobalDescriptorTable(&GDTR, g_KernelState.StateGDT.KernelCodeSegmentSelector, g_KernelState.StateGDT.KernelDataSegmentSelector);
    
    *(BYTE*) 0x2000 = 'H';
    *(BYTE*) 0x2001 = 'O';
    *(BYTE*) 0x2002 = 'I';
    *(BYTE*) 0x2003 = '!';
    KrProcessorHalt();
}
