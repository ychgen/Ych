#include "CPU/APIC.h"

#include "CPU/PortIO.h"
#include "CPU/CPUID.h"
#include "CPU/MSR.h"

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
