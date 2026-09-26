#include "CPU/PerCpu.h"

#include "Core/KernelState.h"
#include "CPU/CPUID.h"
#include "CPU/Halt.h"
#include "CPU/MSR.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

KrPerCpu g_PerCpu[MAX_SMP_PROCESSORS] = {0};

VOID KrGrabThisCpuStruct(VOID)
{
    // Read LAPIC ID
    DWORD EAX, EBX, ECX, EDX;
    KrCPUID(KR_CPUID_LEAF_PRC_INF_FEAT_BITS, EAX, EBX, ECX, EDX);

    BYTE ApicID = EBX >> 24;
    KrPerCpu* pPerCpu = g_PerCpu + ApicID;
    
    pPerCpu->pSelf = pPerCpu;
    pPerCpu->ID = ApicID;

    KrWriteModelSpecificRegister(IA32_GS_BASE, (UINTPTR) pPerCpu); // write active base
    KrWriteModelSpecificRegister(IA32_KERNEL_GS_BASE, (UINTPTR) pPerCpu); // write inactive base
}

KrPerCpu* KrThisCpu(VOID)
{
    KrPerCpu* pStruct;
    __asm__ __volatile__ ("movq %%gs:0x0, %0\n\t" : "=r"(pStruct));
    return pStruct;
}

DWORD KrThisCpuID(VOID)
{
    return KrThisCpu()->ID;
}
