#include "Intmgmt/IRQAldist.h"

#include "Core/KernelState.h"
#include "Core/Assert.h"

#include "CPU/PerCpu.h"

PairOfProcessorIntVector AldistOfferSetupForIRQ(DWORD IRQ)
{
    STATIC DWORD Idx = 0;
    UINT RetryCount = 0;
    DWORD Result;
Retry:
    Result = g_KernelState.SmpInfo.pCpuIDs[Idx++];
    if (Idx >= g_KernelState.SmpInfo.ActiveProcessorCount)
    {
        Idx = 0;
    }

    PerCpu* ThatCpu = GetThatCpu(Result);
    for (UINT i = 32; i < NUM_INT_VECTORS; i++)
    {
        if (ThatCpu->Interrupt[i].Vector == 0x00) // Empty?
        {
            return (PairOfProcessorIntVector) {
                .CpuID = Result,
                .IntVector = i
            };
        }
    }
    
    RUNTIME_ENSURE(RetryCount++ < g_KernelState.SmpInfo.ActiveProcessorCount,
        KR_MDCODE_INTERRUPT_VECTORS_OVERRUN, "Could not find home for IRQ %u, all present CPUs have their interrupt vectors assigned.\n",
        IRQ);
    goto Retry;

    KR_STATIC_UNREACHABLE();
    return (PairOfProcessorIntVector) {0};
}
