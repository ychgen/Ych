#include "CPU/APIC.h"

#include "Core/Krnlmeltdown.h"
#include "Core/KernelState.h"

#include "CPU/PortIO.h"
#include "CPU/CPUID.h"
#include "CPU/Halt.h"
#include "CPU/MSR.h"

#include "Memory/MMIO.h"

static VOID KrMufflePIC(VOID);

KrIpiConfigStructValidationResult KrValidateIpiConfigStruct(const KrIpiConfig* pConfig)
{
    if (pConfig->dwDestApic >= MAX_SMP_PROCESSORS)
    {
        return IPI_CONFIG_STRUCT_VALIDATION_RESULT_DEST_APIC_TOO_BIG;
    }
    switch (pConfig->eDeliveryMode)
    {
    case IPI_DELIVERY_FIXED:
    case IPI_DELIVERY_SMI:
    case IPI_DELIVERY_NMI:
    case IPI_DELIVERY_INIT:
    case IPI_DELIVERY_SIPI:
    {
        break;
    }
    default:
    {
        return IPI_CONFIG_STRUCT_VALIDATION_RESULT_DELIVERY_MODE_NV;
    }
    }
    if (pConfig->eDestMode > 1)
    {
        return IPI_CONFIG_STRUCT_VALIDATION_RESULT_DESTMODE_NV;
    }
    if (pConfig->eLevel > 1)
    {
        return IPI_CONFIG_STRUCT_VALIDATION_RESULT_LEVEL_NV;
    }
    if (pConfig->eTrigMode > 1)
    {
        return IPI_CONFIG_STRUCT_VALIDATION_RESULT_TRIGMODE_NV;
    }
    if (pConfig->eDestShorthand > 3)
    {
        return IPI_CONFIG_STRUCT_VALIDATION_RESULT_DEST_SHORTHAND_NV;
    }
    if (pConfig->eDestShorthand != IPI_SHORTHAND_NONE && pConfig->dwDestApic)
    {
        return IPI_CONFIG_STRUCT_VALIDATION_RESULT_SHORTHAND_WITH_TARGET_ID;
    }
    return IPI_CONFIG_STRUCT_VALIDATION_RESULT_SUCCESS;
}

UINTPTR KrApicGetPhysicalBase(VOID)
{
    QWORD ApicBase = KrReadMSR(IA32_APIC_BASE);
    return ApicBase & IA32_APIC_BASE_ADDR_MASK;
}

VOID KrApicSetPhysicalBase(UINTPTR PhysAddr)
{
    QWORD ApicBase = KrReadMSR(IA32_APIC_BASE);

    PhysAddr &= 0x0000FFFFFFFFFFFFUL; // Keep 48-bits only
    ApicBase |= PhysAddr << 12;
    
    KrWriteMSR(IA32_APIC_BASE, ApicBase);
}

BOOL KrApicCheckSupport(VOID)
{
    DWORD EAX, EBX, ECX, EDX;
    KrCPUID(KR_CPUID_LEAF_PRC_INF_FEAT_BITS, EAX, EBX, ECX, EDX);
    return EDX & KR_CPUID_FEAT_EDX_APIC;
}

BOOL KrApicInit(VOID)
{
    if (!KrApicCheckSupport())
    {
        return FALSE;
    }
    KrMufflePIC(); // Kill that boy chop his balls off

    QWORD msrApicBase = KrReadMSR(IA32_APIC_BASE);
    msrApicBase |= IA32_APIC_BASE_EN;
    KrWriteMSR(IA32_APIC_BASE, msrApicBase);

    //KrApicSetPhysicalBase(LOCAL_APIC_BASE_ADDRESS); // we like this address
    if (KrApicGetPhysicalBase() != LOCAL_APIC_BASE_ADDRESS)
    {
        return FALSE;
    }
    // Map Local APIC into virtual address space, 1 page is enough to cover all registers
    g_KernelState.VaddrApicBase = MmioMapDevice(LOCAL_APIC_BASE_ADDRESS, 4096, 0);
    if (!g_KernelState.VaddrApicBase)
    {
        return FALSE;
    }
    
    return TRUE;
}

VOID ApicIssueEndOfInt(VOID)
{
    *(volatile DWORD*)(g_KernelState.VaddrApicBase + APIC_REG_EOI) = 0;
}

VOID ApicWaitForIcrIdle(VOID)
{
    while ( (*(volatile DWORD*)(g_KernelState.VaddrApicBase + APIC_REG_ICR_LO)) & APIC_ICR_DELIVERY_STATUS )
    {
        KrProcessorPause();
    }
}

VOID ApicIssueIpi(const KrIpiConfig* pConfig)
{
#ifndef YCH_DIST_BUILD
    if (KrValidateIpiConfigStruct(pConfig) != IPI_CONFIG_STRUCT_VALIDATION_RESULT_SUCCESS)
    {
        MDCODE mdCode = KR_MDCODE_ISSUE_IPI_DEVCHECK;
        CSTR   strMdCode = "IPI issued with architecturally nonsensical configuration parameters.";
        Krnlmeltdownimm(mdCode, strMdCode);
    }
#endif

    ApicWaitForIcrIdle();

    DWORD dwLoword = 0;
    dwLoword |= pConfig->IntVector;
    dwLoword |= (((DWORD) pConfig->eDeliveryMode) << 8);
    dwLoword |= (((DWORD) pConfig->eDestMode) << 11);
    dwLoword |= (((DWORD) pConfig->eLevel) << 14);
    dwLoword |= (((DWORD) pConfig->eTrigMode) << 15);
    dwLoword |= (((DWORD) pConfig->eDestShorthand) << 18);

    *(volatile DWORD*)(g_KernelState.VaddrApicBase + APIC_REG_ICR_HI) = (pConfig->dwDestApic << 24);
    *(volatile DWORD*)(g_KernelState.VaddrApicBase + APIC_REG_ICR_LO) = dwLoword;

    ApicWaitForIcrIdle();
}

// This basically makes the old PIC suffer in agony as it deserves to do so
static VOID KrMufflePIC(VOID)
{
    KrOutByteToPort(0x21, 0xFF); // Master PIC Data Port = 0x21, 0xFF to mask it off
    KrOutByteToPort(0xA1, 0xFF); // Slave  PIC Data Port = 0xA1, 0xFF to mask it off
}
