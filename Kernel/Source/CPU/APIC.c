#include "CPU/APIC.h"

#include "CPU/CPUID.h"
#include "CPU/MSR.h"

bool KrProcessorSupportsAPIC(void)
{
    uint32_t EAX, EBX, ECX, EDX;
    KrCPUID(1, EAX, EBX, ECX, EDX);
    return EDX & KR_CPUID_FEAT_EDX_APIC;
}

void KrEnableAPIC(void)
{
    uint64_t MSR = KrReadModelSpecificRegister(KR_APIC_BASE_MSR);
    MSR |= (1 << KR_APIC_BASE_MSR_ENABLE);
    KrWriteModelSpecificRegister(KR_APIC_BASE_MSR, MSR);
}

uint64_t KrGetAPICPhysicalBase(void)
{
    uint64_t msr = KrReadModelSpecificRegister(KR_APIC_BASE_MSR);
    return msr & 0xFFFFF000;
}
