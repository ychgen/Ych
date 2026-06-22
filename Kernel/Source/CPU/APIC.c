#include "CPU/APIC.h"

#include "Core/Krnlmeltdown.h"
#include "Core/KernelState.h"

#include "CPU/PortIO.h"
#include "CPU/CPUID.h"
#include "CPU/MSR.h"

Krx2ApicIpiConfigStructValidationResult KrValidatex2ApicIpiConfigStruct(const Krx2ApicIpiConfig* pConfig)
{
    if (pConfig->dwDestApic >= MAX_SMP_PROCESSORS)
    {
        return KRX2APIC_IPI_CONFIG_STRUCT_VALIDATION_RESULT_DEST_APIC_TOO_BIG;
    }
    switch (pConfig->eDeliveryMode)
    {
    case KRX2APIC_IPI_DELIVERY_FIXED:
    case KRX2APIC_IPI_DELIVERY_SMI:
    case KRX2APIC_IPI_DELIVERY_NMI:
    case KRX2APIC_IPI_DELIVERY_INIT:
    case KRX2APIC_IPI_DELIVERY_SIPI:
    {
        break;
    }
    default:
    {
        return KRX2APIC_IPI_CONFIG_STRUCT_VALIDATION_RESULT_DELIVERY_MODE_NV;
    }
    }
    if (pConfig->eDestMode > 1)
    {
        return KRX2APIC_IPI_CONFIG_STRUCT_VALIDATION_RESULT_DESTMODE_NV;
    }
    if (pConfig->eLevel > 1)
    {
        return KRX2APIC_IPI_CONFIG_STRUCT_VALIDATION_RESULT_LEVEL_NV;
    }
    if (pConfig->eTrigMode > 1)
    {
        return KRX2APIC_IPI_CONFIG_STRUCT_VALIDATION_RESULT_TRIGMODE_NV;
    }
    if (pConfig->eDestShorthand > 3)
    {
        return KRX2APIC_IPI_CONFIG_STRUCT_VALIDATION_RESULT_DEST_SHORTHAND_NV;
    }
    if (pConfig->eDestShorthand != KRX2APIC_IPI_SHORTHAND_NONE && pConfig->dwDestApic)
    {
        return KRX2APIC_IPI_CONFIG_STRUCT_VALIDATION_RESULT_SHORTHAND_WITH_TARGET_ID;
    }
    return KRX2APIC_IPI_CONFIG_STRUCT_VALIDATION_RESULT_SUCCESS;
}

BOOL Krx2CheckSupport(VOID)
{
    DWORD EAX, EBX, ECX, EDX;
    KrCPUID(KR_CPUID_LEAF_PRC_INF_FEAT_BITS, EAX, EBX, ECX, EDX);
    return ECX & KR_CPUID_FEAT_ECX_X2APIC;
}

// This basically makes the old PIC suffer in agony as it deserves to do so
VOID KrMufflePIC(VOID)
{
    KrOutByteToPort(0x21, 0xFF); // Master PIC Data Port = 0x21, 0xFF to mask it off
    KrOutByteToPort(0xA1, 0xFF); // Slave  PIC Data Port = 0xA1, 0xFF to mask it off
}

BOOL Krx2Enable(VOID)
{
    if (!Krx2CheckSupport())
    {
        return FALSE;
    }
    KrMufflePIC(); // Kill that boy chop his balls off

    QWORD MSR = KrReadModelSpecificRegister(KR_MSR_IA32_APIC_BASE);
    MSR |= KR_MSR_IA32_APIC_BASE_EN | KR_MSR_IA32_APIC_BASE_EXTD; // Enable APIC and enable x2APIC mode
    KrWriteModelSpecificRegister(KR_MSR_IA32_APIC_BASE, MSR);

    return TRUE;
}

VOID Krx2SignalEndOfInterrupt(VOID)
{
    KrWriteModelSpecificRegister(KR_MSR_IA32_X2APIC_EOI, 0);
}

VOID Krx2ApicIssueIPI(const Krx2ApicIpiConfig* pConfig)
{
#ifndef YCH_DIST_BUILD
    if (KrValidatex2ApicIpiConfigStruct(pConfig) != KRX2APIC_IPI_CONFIG_STRUCT_VALIDATION_RESULT_SUCCESS)
    {
        MDCODE mdCode = KR_MDCODE_ISSUE_IPI_DEVCHECK;
        CSTR   strMdCode = "IPI issued with architecturally nonsensical configuration parameters.";
        Krnlmeltdownimm(mdCode, strMdCode);
    }
#endif

    QWORD CmdVal = 0;
    CmdVal |= pConfig->IntVector;
    CmdVal |= (((QWORD) pConfig->eDeliveryMode) << 8);
    CmdVal |= (((QWORD) pConfig->eDestMode) << 11);
    CmdVal |= (((QWORD) pConfig->eLevel) << 14);
    CmdVal |= (((QWORD) pConfig->eTrigMode) << 15);
    CmdVal |= (((QWORD) pConfig->eDestShorthand) << 18);
    CmdVal |= (((QWORD) pConfig->dwDestApic) << 32);

    KrWriteModelSpecificRegister(KR_MSR_IA32_X2APIC_ICR, CmdVal);
}
