#include "Core/Krnlmeltdown.h"
#include "Core/KrProcessorHalt.h"

#include "Init/KrKernelStart.h"

__attribute__((noreturn))
void Krnlmeltdown(uint64_t code, const char* pDesc, const KrProcessorSnapshot* pSnapshot)
{
    // TODO: Log, for now we just paint screen red
    KrGraphicsInfo* pGraphicsInfo = &g_pSystemInfoPack->GraphicsInfo;
    uint32_t* pFramebuffer = (uint32_t*) pGraphicsInfo->PhysicalFramebufferAddress;

    for (uint32_t i = 0; i < pGraphicsInfo->FramebufferSize; i++)
    {
        pFramebuffer[i] = 0x00FF0000;
    }

    KrProcessorHalt();
}
