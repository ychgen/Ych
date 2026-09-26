#include "CPU/APIC.h"

#include "Core/Krnlmeltdown.h"
#include "Core/KernelState.h"

#include "CPU/PortIO.h"
#include "CPU/CPUID.h"
#include "CPU/MSR.h"

VOID KrMufflePIC(VOID);

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
    QWORD msrApicBase = KrReadModelSpecificRegister(KR_MSR_IA32_APIC_BASE);
    return msrApicBase & KR_MSR_IA32_APIC_BASE_ADDR_MASK;
}

VOID KrApicSetPhysicalBase(UINTPTR PhysAddr)
{
    QWORD msrApicBase = KrReadModelSpecificRegister(KR_MSR_IA32_APIC_BASE);
    PhysAddr &= 0x0000FFFFFFFFFFFFUL; // Kepp 48-bits only
    msrApicBase = PhysAddr << 12;
    KrWriteModelSpecificRegister(KR_MSR_IA32_APIC_BASE, msrApicBase);
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

    DWORD EAX, EBX, ECX, EDX;
    QWORD msrApicBase = KrReadModelSpecificRegister(KR_MSR_IA32_APIC_BASE);
    msrApicBase |= KR_MSR_IA32_APIC_BASE_EN;
    KrWriteModelSpecificRegister(KR_MSR_IA32_APIC_BASE, msrApicBase);

    return TRUE;
}

VOID KrApicIssueEndOfInt(VOID)
{
    
}

VOID KrApicIssueIpi(const KrIpiConfig* pConfig)
{
#ifndef YCH_DIST_BUILD
    if (KrValidateIpiConfigStruct(pConfig) != IPI_CONFIG_STRUCT_VALIDATION_RESULT_SUCCESS)
    {
        MDCODE mdCode = KR_MDCODE_ISSUE_IPI_DEVCHECK;
        CSTR   strMdCode = "IPI issued with architecturally nonsensical configuration parameters.";
        Krnlmeltdownimm(mdCode, strMdCode);
    }
#endif

}

// This basically makes the old PIC suffer in agony as it deserves to do so
VOID KrMufflePIC(VOID)
{
    KrOutByteToPort(0x21, 0xFF); // Master PIC Data Port = 0x21, 0xFF to mask it off
    KrOutByteToPort(0xA1, 0xFF); // Slave  PIC Data Port = 0xA1, 0xFF to mask it off
}
