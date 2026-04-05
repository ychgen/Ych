#include "Init/KrKernelStart.h"

#include "Init/KrInitGDT.h"
#include "Init/KrInitInt.h"

#include "Core/Krnlmeltdown.h"
#include "Core/KernelState.h"

#include "CPU/APIC.h"
#include "CPU/Halt.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"
#include "Earlyvideo/Dwtpfonts.h"

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
    g_KernelState.SystemInfoPack = *pSystemInfoPack; // Copy over, otherwise will be lost when we unmap the 0MiB to 2MiB area.

    // Init DisplaywideTextProtocol
    {
        KrGraphicsInfo* pGraphicsInfo = &g_KernelState.SystemInfoPack.GraphicsInfo;

        KrdwtpInitializeDefaultFonts();
        if (pGraphicsInfo->FramebufferWidth >= 1920 && pGraphicsInfo->FramebufferHeight >= 1080)
        {
            // 4K... But why would you, anyway...
            if (pGraphicsInfo->FramebufferWidth >= 3840 || pGraphicsInfo->FramebufferHeight >= 2160)
            {
                g_KrdwtpDefaultFont_8x16.ScaleFactor = 4;
            }
            else
            {
                g_KrdwtpDefaultFont_8x16.ScaleFactor = 2;
            }
        }

        KrdwtpInitialize(g_KrdwtpDefaultFont_8x16, FRAMEBUFFER_VIRTUAL_ADDR, pGraphicsInfo->FramebufferWidth, pGraphicsInfo->FramebufferHeight, pGraphicsInfo->PixelsPerScanLine);
        g_KernelState.VideoOutputProtocol = KR_VIDEO_OUTPUT_PROTOCOL_DISPLAYWIDE_TEXT_PROTOCOL;
        g_KernelState.VideoOutputContext  = (void*) KrdwtpGetProtocolState();

        KrdwtpResetState(KRDWTP_COLOR_BLACK);
        KrdwtpOutColoredText("Initialized Display-Wide Text Protocol\n", KRDWTP_COLOR_GREEN);
    }

    // Print some useful information
    KrdwtpOutFormatText("[KRNLYCH] Kernel Post-Load Self Information:\n"
        " -> Kernel Binary Size       = %u KiB\n"
        " -> Load Address (PHYS)      = %p\n"
        " -> Reserved Area End (PHYS) = %p\n"
        " -> Total Reserved           = %u MiB\n",
        (uint32_t) g_KernelState.SystemInfoPack.KernelBinarySize / 1024,
        (void*) g_KernelState.SystemInfoPack.AddrKernelLoad,
        (void*) g_KernelState.SystemInfoPack.AddrKernelSpaceEnd,
        (uint32_t)(g_KernelState.SystemInfoPack.AddrKernelSpaceEnd - g_KernelState.SystemInfoPack.AddrKernelLoad) / 1024 / 1024);
    KrdwtpOutFormatText("UEFI GOP Frame Buffer lives at physical %p\n", (const void*) g_KernelState.SystemInfoPack.GraphicsInfo.PhysicalFramebufferAddress);
    KrdwtpOutFormatText("GOP Framebuffer is resolution %ux%u.\n", g_KernelState.SystemInfoPack.GraphicsInfo.FramebufferWidth, g_KernelState.SystemInfoPack.GraphicsInfo.FramebufferHeight);

    // Initialize flat Global Descriptor Table.
    KrInitGDT();
    KrdwtpOutColoredText("Initialized and loaded the Global Descriptor Table.\n", KRDWTP_COLOR_GREEN);
    
    // Initialize IDT and ISRs. Overall initializing interrupt handling.
    KrInitInt();
    KrdwtpOutColoredText("Initialized the interrupt subsystem.\n", KRDWTP_COLOR_GREEN);
    
    // Enable APIC (best safe to do this after interrupt setup as it might fire interrupts before we fully initialize the interrupt subsystem)
    KrEnableAPIC();
    {
        g_KernelState.StateLocalAPIC.BaseAddrPhysical = KrGetAPICPhysicalBase();
        //Cant use, unmapped. need to set StateLocalAPIC.BaseAddrVirtual
        //g_KernelState.StateLocalAPIC.BaseAddr = g_KernelState.StateLocalAPIC.BaseAddrPhysical;
    }
    KrdwtpOutColoredText("Initialized local APIC.\n", KRDWTP_COLOR_GREEN);
    KrdwtpOutFormatText("Local APIC Physical Base Address = %p\n", (void*) g_KernelState.StateLocalAPIC.BaseAddrPhysical);

    // ======= STOP HERE =========== //
    KrProcessorHalt();

#include "krstend.h" /* ! Always keep this at the very end of the function ! */
}
