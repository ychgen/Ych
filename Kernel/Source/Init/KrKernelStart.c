#include "Init/KrKernelStart.h"

#include "Init/KrInitGDT.h"
#include "Init/KrInitInt.h"

#include "Core/Krnlmeltdown.h"
#include "Core/KernelState.h"

#include "CPU/APIC.h"
#include "CPU/Halt.h"

#include "Memory/Krnlmem.h"

__attribute__((section(".text.KrKernelStart"), noreturn))
/** Entry point of the kernel. The bootloader will jump to this function upon control transfer. */
void KrKernelStart(const KrSystemInfoPack* pSystemInfoPack)
{    
    __asm__ __volatile__("cli"); // Just to be safe, clear interrupts (YchBoot does this anyway, but, still.)

    if (pSystemInfoPack->Magic != SYSTEM_INFO_PACK_MAGIC)
    {
        // We can't really report errors as the integrity of the struct is compromised.
        // Framebuffer can also very well be invalid.
        // So we just halt... quietly.
        KrProcessorHalt();
    }

    // Initialize g_KernelState
    KrContiguousZeroBuffer(&g_KernelState, sizeof(KrKernelState));
    g_KernelState.SystemInfoPack = *pSystemInfoPack; // Copy over

    // Initialize flat Global Descriptor Table.
    KrInitGDT();
    // Initialize IDT and ISRs. Overall initializing interrupt handling.
    KrInitInt();

    // Enable APIC (best safe to do this after interrupt setup as it might fire interrupts before we fully initialize the interrupt subsystem)
    KrEnableAPIC();

    // breakpoint
    __asm__ __volatile__ ("int $3");

    // test meltdown
    meltdowncode_t code = KR_MELTDOWN_CODE_GENERAL_DEBUG;
    const char* desc = NULL;
    Krnlmeltdownimm(code, desc);
    
    KrProcessorHalt();
}
