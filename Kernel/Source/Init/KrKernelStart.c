#include "Init/KrKernelStart.h"

#include "Init/KrInitMemmap.h"
#include "Init/KrInitACPI.h"
#include "Init/KrInitGDT.h"
#include "Init/KrInitInt.h"
#include "Init/KrInitMem.h"
#include "Init/KrInitSMP.h"

#include "Core/Krnlmeltdown.h"
#include "Core/KernelState.h"

#include "CPU/Identify.h"
#include "CPU/PerCpu.h"
#include "CPU/APIC.h"
#include "CPU/Halt.h"
#include "CPU/MSR.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"
#include "Earlyvideo/Dwtpfonts.h"

#include "Memory/BootstrapArena.h"
#include "Memory/Physmemmgmt.h"
#include "Memory/Virtmemmgmt.h"
#include "Memory/Krnlheap.h"

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
    g_KernelState.SmpInfo.ActiveProcessorCount = 1; // The Bootstrap Processor is currently active

    // Copy pSystemInfoPack so we don't lose it when we unmap the ID-mapped lower 2MiB.
    KrSystemInfoPack SysInfoPack;
    KrtlContiguousCopyBuffer(&SysInfoPack, pSystemInfoPack, sizeof(KrSystemInfoPack));

    g_KernelState.LoadInfo.BinarySize       = SysInfoPack.KernelBinarySize;
    g_KernelState.LoadInfo.ReserveSize      = SysInfoPack.KernelReserveSize;
    g_KernelState.LoadInfo.AddrPhysicalBase = SysInfoPack.KernelPhysicalBase;
    g_KernelState.LoadInfo.AddrVirtualBase  = SysInfoPack.KernelVirtualBase;

    // Initialize Bootstrap Arena Allocator & the Canonical Memory Map (which depends on the Barena, so init after that)
    KrInitBootstrapArena((VOID*) SysInfoPack.KernelBootstrapArenaBase, SysInfoPack.KernelBootstrapArenaSize);
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
            // 4K (who the fuck is running this OS on a 4K monitor bro)
            if (pGraphicsInfo->FramebufferWidth >= 3840 && pGraphicsInfo->FramebufferHeight >= 2160)
            {
                g_KrdwtpDefaultFont_8x16.ScaleFactor = 4;
            }
            else
            {
                g_KrdwtpDefaultFont_8x16.ScaleFactor = 2;
            }
        }

        KrdwtpInitialize(g_KrdwtpDefaultFont_8x16, g_KernelState.FrameBufferInfo.VirtualAddress, pGraphicsInfo->FramebufferSize, pGraphicsInfo->FramebufferWidth, pGraphicsInfo->FramebufferHeight, pGraphicsInfo->PixelsPerScanLine);
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
    KrdwtpOutFormatText("Frame Buffer lives at physical 0x%RX ; virtual 0x%RX\n", SysInfoPack.GraphicsInfo.PhysicalFramebufferAddress, FRAMEBUFFER_VIRTUAL_ADDR);
    KrdwtpOutFormatText("Framebuffer resolution is %ux%u.\n", SysInfoPack.GraphicsInfo.FramebufferWidth, SysInfoPack.GraphicsInfo.FramebufferHeight);

    // Initialize flat Global Descriptor Table.
    KrInitGDT();
    KrdwtpOutColoredText("Initialized and loaded the Global Descriptor Table.\n", KRDWTP_COLOR_GREEN, KRDWTP_BACKGROUND);

    // Per CPU for the bootstrap processor. Must be done AFTER init GDT, because `lgdt` invalidates GS base.
    KrGrabThisCpuStruct();

    // Initialize IDT and ISRs. Overall initializing interrupt handling.
    // Bye bye Triple Fault!
    KrInitInt();
    KrdwtpOutColoredText("Initialized the interrupt subsystem.\n", KRDWTP_COLOR_GREEN, KRDWTP_BACKGROUND);

    {
        KrProcessorInfoAndFeatures PrcInfnFeats;
        KrGetProcessorInfoAndFeatures(&PrcInfnFeats);

        CHAR ManufacturerID[KR_PROCESSOR_MANUFACTURER_ID_SIZE + 1];
        KrGetProcessorManufacturerID(ManufacturerID);
        ManufacturerID[KR_PROCESSOR_MANUFACTURER_ID_SIZE] = 0;

        CHAR BrandStr[KR_PROCESSOR_BRAND_STRING_SIZE + 1];
        KrGetProcessorBrandString(BrandStr);
        BrandStr[KR_PROCESSOR_BRAND_STRING_SIZE] = 0;

        KrdwtpOutFormatText
        (
            "Processor Information:\n"
            " -> Processor : %s %s\n"
            " -> Stepping  : 0x%X\n"
            " -> Model     : 0x%X\n"
            " -> Family    : 0x%X\n",

            ManufacturerID, BrandStr,
            PrcInfnFeats.Stepping, PrcInfnFeats.Model, PrcInfnFeats.Family
        );
    }

    // Init Physmemmgmt & Virtmemmgmt. Must be done before APIC init as it needs MMIO which needs the VMM being alive.
    KrInitMem();

    // Initialize APIC only AFTER initializing the interrupt subsystem via KrInitInt().
    if (!KrApicInit())
    {
        MDCODE mdCode = KR_MDCODE_LOCAL_APIC_INIT_FAILURE;
        CSTR szMdDesc = "Local APIC initialization failure.";
        Krnlmeltdownimm(mdCode, szMdDesc);
    }
    KrdwtpOutColoredText("Initialized the local APIC.\n", KRDWTP_COLOR_GREEN, KRDWTP_BACKGROUND); 

    // Init ACPI
    KrInitACPI(SysInfoPack.PhysAddrRSDP);
    KrdwtpOutColoredText("ACPI initialization sucessful.\n", KRDWTP_COLOR_GREEN, KRDWTP_BACKGROUND);
    
    KrdwtpOutColoredText("Processor topology:\n", KRDWTP_COLOR_YELLOW, KRDWTP_BACKGROUND); 
    for (UINT i = 0; i < MAX_SMP_PROCESSORS; i++)
    {
        if (!g_KernelState.SmpInfo.ProcInfo[i].E_V)
        {
            continue;
        }
        KrdwtpOutFormatText(" -> Logical CPU `APIC %u / ACPI %u` %s\n",
            g_KernelState.SmpInfo.ProcInfo[i].LocalApicID,
            g_KernelState.SmpInfo.ProcInfo[i].AcpiID,
            g_KernelState.SmpInfo.ProcInfo[i].BSP ? "BSP" : "AP"
        ); 
    }

    // Initialize & bring-up the Application Processors.
    KrInitSMP();

    KrdwtpOutColoredText("KrKernelStart() finished, the processor is now halted.\n", KRDWTP_COLOR_PURPLE, KRDWTP_BACKGROUND);
    // ======= STOP HERE =========== //
    KrProcessorHalt();

#include "Krnlstend.h" /* ! Always keep this at the very end of the function ! Invokes a special Kernel Meltdown. */
}
