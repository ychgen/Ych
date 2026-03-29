#ifndef YCH_KERNEL_INIT_KR_INIT_GDT_H
#define YCH_KERNEL_INIT_KR_INIT_GDT_H

#include "CPU/GDT.h"

// Standard kernel GDT storage and information
#define KR_STANDARD_GPT_NUMBER_OF_DESCRIPTOR_ENTRIES 3
#define KR_STANDARD_GPT_SIZE                         ( KR_STANDARD_GPT_NUMBER_OF_DESCRIPTOR_ENTRIES * GDT_SEGMENT_DESCRIPTOR_ENTRY_SIZE )
extern __attribute__((aligned(16))) uint8_t g_KrSegmentDescriptorsGDT[KR_STANDARD_GPT_SIZE];

void KrInitGDT(void);

#endif // !YCH_KERNEL_INIT_KR_INIT_GDT_H
