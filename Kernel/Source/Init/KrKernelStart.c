#include "Init/KrKernelStart.h"

#include "Init/KrInitMemmap.h"
#include "Init/KrInitGDT.h"
#include "Init/KrInitInt.h"
#include "Init/KrInitMem.h"

#include "Core/Krnlmeltdown.h"
#include "Core/KernelState.h"

#include "CPU/Identify.h"
#include "CPU/APIC.h"
#include "CPU/Halt.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"
#include "Earlyvideo/Dwtpfonts.h"

#include "Memory/BootstrapArena.h"
#include "Memory/Physmemmgmt.h"
#include "Memory/Virtmemmgmt.h"

#include "KRTL/Krnlmem.h"

/** Defined by the linker (see `Kernel.ld` linker script used to link the kernel). */
extern CHAR __KR_LINK_BSS_START[];
extern CHAR __KR_LINK_BSS_END[];

/** Entry point of the kernel. The bootloader will jump to this function upon control transfer. */
KR_SECTION(".text.KrKernelStart")
KR_NORETURN VOID KrKernelStart(const KrSystemInfoPack* pSystemInfoPack)
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

    g_KernelState.LoadInfo.BinarySize       = SysInfoPack.KernelBinarySize;
    g_KernelState.LoadInfo.ReserveSize      = SysInfoPack.KernelReserveSize;
    g_KernelState.LoadInfo.AddrPhysicalBase = SysInfoPack.KernelPhysicalBase;
    g_KernelState.LoadInfo.AddrVirtualBase  = SysInfoPack.KernelVirtualBase;

    // Initialize Bootstrap Arena
    KrInitBootstrapArena((VOID*) SysInfoPack.KernelBootstrapArenaBase, SysInfoPack.KernelBootstrapArenaSize);

    // Does sum crazy shit
    KrInitMemmap(&SysInfoPack.MemoryMapInfo);

    // Init FrameBufferInfo & DisplaywideTextProtocol
    {
        KrGraphicsInfo* pGraphicsInfo = &SysInfoPack.GraphicsInfo;

        // Init these too
        g_KernelState.FrameBufferInfo.PhysicalAddress = pGraphicsInfo->PhysicalFramebufferAddress;
        g_KernelState.FrameBufferInfo.VirtualAddress  = FRAMEBUFFER_VIRTUAL_ADDR;
        g_KernelState.FrameBufferInfo.Size            = pGraphicsInfo->FramebufferSize;

        KrdwtpInitializeDefaultFonts();
        if (pGraphicsInfo->FramebufferWidth >= 2560 && pGraphicsInfo->FramebufferHeight >= 1440)
        {
            // 4K... But why would you, anyway...
            if (pGraphicsInfo->FramebufferWidth >= 3840 && pGraphicsInfo->FramebufferHeight >= 2160)
            {
                g_KrdwtpDefaultFont_8x16.ScaleFactor = 4;
            }
            else
            {
                g_KrdwtpDefaultFont_8x16.ScaleFactor = 2;
            }
        }

        KrdwtpInitialize(g_KrdwtpDefaultFont_8x16, FRAMEBUFFER_VIRTUAL_ADDR, pGraphicsInfo->FramebufferSize, pGraphicsInfo->FramebufferWidth, pGraphicsInfo->FramebufferHeight, pGraphicsInfo->PixelsPerScanLine);
        g_KernelState.VideoOutputProtocol = KR_VIDEO_OUTPUT_PROTOCOL_DISPLAYWIDE_TEXT;
        g_KernelState.VideoOutputContext  = KrdwtpGetProtocolState();

        KrdwtpResetState(KRDWTP_COLOR_BLACK);
        KrdwtpOutColoredText("Initialized Displaywide Text Protocol\n", KRDWTP_COLOR_GREEN, KRDWTP_BACKGROUND);
    }

    // Print some useful information
    KrdwtpOutFormatText("[KRNLYCH] Kernel Post-Load Self Information:\n"
        " -> Kernel Binary Size  : %u~%u KiB (%u bytes)\n"
        " -> Load Address (PHYS) : %p\n"
        " -> Load Address (VIRT) : %p\n"
        " -> Total Reserved      : %u MiB\n",
        (UINT)  g_KernelState.LoadInfo.BinarySize / 1024, (UINT) KR_CEILDIV(g_KernelState.LoadInfo.BinarySize, 1024), (UINT) g_KernelState.LoadInfo.BinarySize,
        (void*) g_KernelState.LoadInfo.AddrPhysicalBase,
        (void*) g_KernelState.LoadInfo.AddrVirtualBase,
        (UINT)(g_KernelState.LoadInfo.ReserveSize / 1024 / 1024));
    KrdwtpOutFormatText("UEFI GOP Frame Buffer lives at physical %p\n", (const void*) SysInfoPack.GraphicsInfo.PhysicalFramebufferAddress);
    KrdwtpOutFormatText("Framebuffer resolution is %ux%u.\n", SysInfoPack.GraphicsInfo.FramebufferWidth, SysInfoPack.GraphicsInfo.FramebufferHeight);

    {
        CHAR ManufacturerID[KR_PROCESSOR_MANUFACTURER_ID_SIZE + 1];
        KrGetProcessorManufacturerID(ManufacturerID);
        ManufacturerID[KR_PROCESSOR_MANUFACTURER_ID_SIZE] = 0;

        CHAR BrandStr[KR_PROCESSOR_BRAND_STRING_SIZE + 1];
        KrGetProcessorBrandString(BrandStr);
        BrandStr[KR_PROCESSOR_BRAND_STRING_SIZE] = 0;

        KrdwtpOutFormatText
        (
            "Processor Information:\n"
            " -> Manufacturer : %s\n"
            " -> Brand String : %s\n",

            ManufacturerID, BrandStr
        );
    }

    // Initialize flat Global Descriptor Table.
    KrInitGDT();
    KrdwtpOutColoredText("Initialized and loaded the Global Descriptor Table.\n", KRDWTP_COLOR_GREEN, KRDWTP_BACKGROUND);
    
    // Initialize IDT and ISRs. Overall initializing interrupt handling.
    // Bye bye Triple Fault!
    KrInitInt();
    KrdwtpOutColoredText("Initialized the interrupt subsystem.\n", KRDWTP_COLOR_GREEN, KRDWTP_BACKGROUND);

    KrdwtpOutColoredText("Initialized local APIC.\n", KRDWTP_COLOR_GREEN, KRDWTP_BACKGROUND);
    KrdwtpOutFormatText("Local APIC Physical Base Address = 0x%RX\n", g_KernelState.LAPIC.PhysAddr);

    // Init Physmemmgmt & Virtmemmgmt
    KrInitMem();

    // LAPIC stuff.
    g_KernelState.LAPIC.PhysAddr = KrGetAPICPhysicalBase();
    g_KernelState.LAPIC.VirtAddr = KR_MAKE_VIRTUAL(KR_KERNEL_RESERVED_PML4_INDEX, 0, 0, 0, 0);
    if
    (
        KrMapVirt
        (
            KR_VMM_PROCID_KERNEL,
            g_KernelState.LAPIC.VirtAddr,
            g_KernelState.LAPIC.PhysAddr,
            1, // will be rounded to page boundary so 4KiB
            KR_ACQUIRE_STATIC,
            KR_PAGE_FLAG_WRITE | KR_PAGE_FLAG_UNCACHEABLE
        ) != KR_MAP_RESULT_SUCCESS
    )
    {
        MDCODE mdCode = KR_MDCODE_LOCAL_APIC_MAP_FAILURE;
        CSTR szDesc = "Failed to map the local APIC to the kernel virtual address space!";
        Krnlmeltdownimm(mdCode, szDesc);
    }
    KrdwtpOutFormatText("Successfully mapped the Local APIC at virtual 0x%RX.\n", g_KernelState.LAPIC.VirtAddr);

    // Enable APIC (best safe to do this after interrupt setup as it might fire interrupts before we fully initialize the interrupt subsystem)
    // Also we doing this after mapping it with MMIO yk so we can send EOI.
    KrEnableAPIC();

    const KrVirtualMemoryRegion* pRegion = KrGetRootVMR();
    while (pRegion)
    {
        KrdwtpOutFormatText("VMR -> Base : 0x%RX , PageCount : 0x%RX , Acq : 0x%X , Flg : 0x%X , PID : %u\n", pRegion->VirtAddrBase, pRegion->szPageCount, pRegion->wAcquisitionType, pRegion->wFlags, pRegion->uProcID);
        pRegion = pRegion->pNext;
    }

    // ======= STOP HERE =========== //
    KrProcessorHalt();

#include "Krnlstend.h" /* ! Always keep this at the very end of the function ! Invokes a special Kernel Meltdown. */
}
