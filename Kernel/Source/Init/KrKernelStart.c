#include "Init/KrKernelStart.h"

#include "Init/KrInitGDT.h"
#include "Init/KrInitInt.h"

#include "Core/Krnlmeltdown.h"
#include "Core/KernelState.h"

#include "CPU/APIC.h"
#include "CPU/Halt.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"
#include "Earlyvideo/Dwtpfonts.h"

#include "Memory/BootstrapArena.h"
#include "Memory/Physmemmgmt.h"
#include "Memory/Virtmemmgmt.h"
#include "KRTL/Krnlmem.h"

/** Defined by the linker (see `Kernel.ld` linker script used to link the kernel). */
extern char __KR_LINK_BSS_START[];
extern char __KR_LINK_BSS_END[];

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

    // Zero the BSS section.
    KrtlContiguousZeroBuffer(__KR_LINK_BSS_START, (SIZE)(__KR_LINK_BSS_END - __KR_LINK_BSS_START));

    // Initialize g_KernelState
    KrtlContiguousZeroBuffer(&g_KernelState, sizeof(KrKernelState));
    // Copy pSystemInfoPack so we don't lose it when we unmap the ID-mapped lower 2MiB.
    KrSystemInfoPack SysInfoPack;
    KrtlContiguousCopyBuffer(&SysInfoPack, pSystemInfoPack, sizeof(KrSystemInfoPack));

    g_KernelState.LoadInfo.BinarySize = SysInfoPack.KernelBinarySize;
    g_KernelState.LoadInfo.ReserveSize = SysInfoPack.KernelReserveSize;
    g_KernelState.LoadInfo.AddrPhysicalBase = SysInfoPack.KernelPhysicalBase;
    g_KernelState.LoadInfo.AddrVirtualBase = SysInfoPack.KernelVirtualBase;

    // Initialize Bootstrap Arena
    KrInitBootstrapArena((void*) SysInfoPack.KernelBootstrapArenaBase, SysInfoPack.KernelBootstrapArenaSize);

    // Copy over the memory map
    {
        g_KernelState.MemoryMapInfo = SysInfoPack.MemoryMapInfo;
        g_KernelState.MemoryMap = KrBootstrapArenaAcquire(g_KernelState.MemoryMapInfo.MemoryMapSize);
        KrtlContiguousCopyBuffer(g_KernelState.MemoryMap, (void*) SysInfoPack.MemoryMapInfo.PhysicalAddress, g_KernelState.MemoryMapInfo.MemoryMapSize);
    }

    // Init DisplaywideTextProtocol
    {
        KrGraphicsInfo* pGraphicsInfo = &SysInfoPack.GraphicsInfo;

        KrdwtpInitializeDefaultFonts();
        if (pGraphicsInfo->FramebufferWidth >= 2560 && pGraphicsInfo->FramebufferHeight >= 1440)
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
        g_KernelState.VideoOutputProtocol = KR_VIDEO_OUTPUT_PROTOCOL_DISPLAYWIDE_TEXT;
        g_KernelState.VideoOutputContext  = (void*) KrdwtpGetProtocolState();

        KrdwtpResetState(KRDWTP_COLOR_BLACK);
        KrdwtpOutColoredText("Initialized Displaywide Text Protocol\n", KRDWTP_COLOR_GREEN, KRDWTP_BACKGROUND);
    }

    // Print some useful information
    KrdwtpOutFormatText("[KRNLYCH] Kernel Post-Load Self Information:\n"
        " -> Kernel Binary Size       = %u KiB\n"
        " -> Load Address (PHYS)      = %p\n"
        " -> Load Address (VIRT)      = %p\n"
        " -> Total Reserved           = %Ru MiB\n",
        (DWORD) g_KernelState.LoadInfo.BinarySize / 1024,
        (void*) g_KernelState.LoadInfo.AddrPhysicalBase,
        (void*) g_KernelState.LoadInfo.AddrVirtualBase,
        (QWORD)(g_KernelState.LoadInfo.ReserveSize / 1024 / 1024));
    KrdwtpOutFormatText("UEFI GOP Frame Buffer lives at physical %p\n", (const void*) SysInfoPack.GraphicsInfo.PhysicalFramebufferAddress);
    KrdwtpOutFormatText("GOP Framebuffer is resolution %ux%u.\n", SysInfoPack.GraphicsInfo.FramebufferWidth, SysInfoPack.GraphicsInfo.FramebufferHeight);

    // Initialize flat Global Descriptor Table.
    KrInitGDT();
    KrdwtpOutColoredText("Initialized and loaded the Global Descriptor Table.\n", KRDWTP_COLOR_GREEN, KRDWTP_BACKGROUND);
    
    // Initialize IDT and ISRs. Overall initializing interrupt handling.
    KrInitInt();
    KrdwtpOutColoredText("Initialized the interrupt subsystem.\n", KRDWTP_COLOR_GREEN, KRDWTP_BACKGROUND);
    
    // Enable APIC (best safe to do this after interrupt setup as it might fire interrupts before we fully initialize the interrupt subsystem)
    KrEnableAPIC();
    {
        g_KernelState.StateLocalAPIC.BaseAddrPhysical = KrGetAPICPhysicalBase();
        //Cant use, unmapped. need to set StateLocalAPIC.BaseAddrVirtual. Will be done when kernel sets up proper paging!
        //g_KernelState.StateLocalAPIC.BaseAddr = g_KernelState.StateLocalAPIC.BaseAddrPhysical;
    }
    KrdwtpOutColoredText("Initialized local APIC.\n", KRDWTP_COLOR_GREEN, KRDWTP_BACKGROUND);
    KrdwtpOutFormatText("Local APIC Physical Base Address = %p\n", (void*) g_KernelState.StateLocalAPIC.BaseAddrPhysical);

    // Initialize Physical Memory Management
    if (!KrInitPhysmemmgmt())
    {
        MDCODE code = KR_MDCODE_PHYSMEMMGMT_INIT_FAILURE;
        const char* pDesc = "Failed to initialize Physmemmgmt.";
        Krnlmeltdownimm(code, pDesc);
    }
    KrdwtpOutColoredText("Initialized Physmemmgmt (Physical Memory Management).\n", KRDWTP_COLOR_GREEN, KRDWTP_BACKGROUND);
    KrdwtpOutFormatText
    (
        "Physical Memory Information:\n"
        " -> Total Pages: %Ru (%Ru MiB) \n"
        " -> Unusable Pages: %Ru (%Ru MiB)\n"
        " -> Total Usable Pages: %Ru (%Ru MiB)\n",
        g_KernelState.StatePMM.TotalPages, g_KernelState.StatePMM.TotalPages * g_KernelState.StatePMM.PageSize / 1024 / 1024,
        g_KernelState.StatePMM.UnusablePages, g_KernelState.StatePMM.UnusablePages * g_KernelState.StatePMM.PageSize / 1024 / 1024,
        (g_KernelState.StatePMM.TotalPages - g_KernelState.StatePMM.UnusablePages), (g_KernelState.StatePMM.TotalPages - g_KernelState.StatePMM.UnusablePages) * g_KernelState.StatePMM.PageSize / 1024 / 1024
    );

    // Test acquisition/relinquishment
    {
        PAGEID TestPageID = KrAcquirePhysicalPage(KR_INVALID_PAGEID);
        if (TestPageID == KR_INVALID_PAGEID)
        {
            MDCODE code = KR_MDCODE_PHYSMEMMGMT_TEST_FAILURE;
            const char* pDesc = "Test page acquisition from Physmemmgmt failed!";
            Krnlmeltdownimm(code, pDesc);
        }
        KrdwtpOutFormatText("Acquired test page from Physmemmgmt: ID = %Ru, Address = %p\n", TestPageID, KrGetPhysicalPageAddress(TestPageID));
        if (!KrRelinquishPhysicalPage(TestPageID))
        {
            MDCODE code = KR_MDCODE_PHYSMEMMGMT_TEST_FAILURE;
            const char* pDesc = "Test page relinquishment to Physmemmgmt failed!";
            Krnlmeltdownimm(code, pDesc);
        }
        KrdwtpOutColoredText("Relinquished test page to Physmemmgmt.\n", KRDWTP_COLOR_CYAN, KRDWTP_BACKGROUND);
    }

    // ======= STOP HERE =========== //
    KrProcessorHalt();

#include "krstend.h" /* ! Always keep this at the very end of the function ! Invokes a special Kernel Meltdown. */
}
