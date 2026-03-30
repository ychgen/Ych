#include "Init/KrKernelStart.h"
#include "Init/KrInitGDT.h"
#include "Init/KrInitIDT.h"

#include "Core/KrProcessorHalt.h"
#include "Core/Krnlmeltdown.h"

KrSystemInfoPack* g_pSystemInfoPack;

__attribute__((section(".text.KrKernelStart"), noreturn))
/** Entry point of the kernel. The bootloader will jump to this function upon control transfer. */
void KrKernelStart(const KrSystemInfoPack* pSystemInfoPack)
{    
    __asm__ __volatile__("cli"); // Just to be safe, clear interrupts (YchBoot does this anyway, but, still.)
    *g_pSystemInfoPack = *pSystemInfoPack; // Immediately store copy of system info

    if (g_pSystemInfoPack->Magic != SYSTEM_INFO_PACK_MAGIC)
    {
        // We can't really report errors as the integrity of the struct is compromised.
        // Framebuffer can also very well be invalid.
        // So we just halt... quietly.
        KrProcessorHalt();
    }

    for (uint64_t i = 0; i < g_pSystemInfoPack->GraphicsInfo.FramebufferSize; i++)
    {
        ((uint8_t*) g_pSystemInfoPack->GraphicsInfo.PhysicalFramebufferAddress)[i] = 0x40;
    }

    // Initialize flat Global Descriptor Table.
    KrInitGDT();
    // Initialize IDT and ISRs.
    KrInitIDT();

    // breakpoint
    __asm__ __volatile__ ("int $3");
    
    KrProcessorHalt();
}
