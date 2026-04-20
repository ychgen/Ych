#include "CPU/APIC.h"

#include "CPU/CPUID.h"
#include "CPU/MSR.h"

BOOL KrProcessorSupportsAPIC(VOID)
{
    DWORD EAX, EBX, ECX, EDX;
    KrCPUID(KR_CPUID_LEAF_PRC_INF_FEAT_BITS, EAX, EBX, ECX, EDX);
    return EDX & KR_CPUID_FEAT_EDX_APIC;
}

VOID KrEnableAPIC(VOID)
{
    QWORD MSR = KrReadModelSpecificRegister(KR_MSR_IA32_APIC_BASE);
    MSR |= KR_MSR_IA32_APIC_BASE_EN;
    KrWriteModelSpecificRegister(KR_MSR_IA32_APIC_BASE, MSR);
}

QWORD KrGetAPICPhysicalBase(VOID)
{
    QWORD  MSR = KrReadModelSpecificRegister(KR_MSR_IA32_APIC_BASE);
    return MSR & 0xFFFFF000;
}
