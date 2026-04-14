#include "CPU/APIC.h"

#include "CPU/CPUID.h"
#include "CPU/MSR.h"

BOOL KrProcessorSupportsAPIC(VOID)
{
    DWORD EAX, EBX, ECX, EDX;
    KrCPUID(1, EAX, EBX, ECX, EDX);
    return EDX & KR_CPUID_FEAT_EDX_APIC;
}

VOID KrEnableAPIC(VOID)
{
    QWORD MSR = KrReadModelSpecificRegister(KR_APIC_BASE_MSR);
    MSR |= (1 << KR_APIC_BASE_MSR_ENABLE);
    KrWriteModelSpecificRegister(KR_APIC_BASE_MSR, MSR);
}

QWORD KrGetAPICPhysicalBase(VOID)
{
    QWORD  MSR = KrReadModelSpecificRegister(KR_APIC_BASE_MSR);
    return MSR & 0xFFFFF000;
}
