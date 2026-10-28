#include "CPU/APIC.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

#include "Core/Krnlmeltdown.h"
#include "Core/KernelState.h"
#include "Core/Assert.h"

#include "Devdriv/Timer/AcpiPmTimer.h"

#include "CPU/Timestamp.h"
#include "CPU/Spinlock.h"
#include "CPU/PortIO.h"
#include "CPU/PerCpu.h"
#include "CPU/CPUID.h"
#include "CPU/Halt.h"
#include "CPU/Stall.h"
#include "CPU/MSR.h"

#include "Intmgmt/Spurious.h"
#include "Intmgmt/Timer.h"

#include "Memory/MMIO.h"

STATIC VOID  ApicCalibrateTimer(VOID);
INLINE DWORD ApicRead(SIZE RegOffset);
INLINE DWORD ApicWrite(SIZE RegOffset, DWORD dwValue);
INLINE VOID  DisableLegacyPIC(VOID);

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

BOOL KrApicInit(BOOL bIsCallerBSP)
{
    if (!KrApicCheckSupport())
    {
        return FALSE;
    }

    //KrApicSetPhysicalBase(LOCAL_APIC_BASE_ADDRESS); // we like this address
    if (KrApicGetPhysicalBase() != LOCAL_APIC_BASE_ADDRESS)
    {
        return FALSE;
    }
    if (bIsCallerBSP)
    {
        DisableLegacyPIC();

        // Map Local APIC into virtual address space, 1 page is enough to cover all registers
        g_KernelState.MmioInfo.VaddrApicBase = MmioMapDevice("Processor Local APIC", LOCAL_APIC_BASE_ADDRESS, 4096, MMIO_NF);

        if (!g_KernelState.MmioInfo.VaddrApicBase)
        {
            return FALSE;
        }
    }

    {
        QWORD ApicBase = KrReadMSR(IA32_APIC_BASE);
        ApicBase |= IA32_APIC_BASE_EN;
        KrWriteMSR(IA32_APIC_BASE, ApicBase);
    }

    ApicWrite(APIC_REG_TPR, 0);
    ApicWrite(APIC_REG_DCR, 0);
    ApicWrite(APIC_REG_LTR, (DWORD) KERNEL_TIMER_INTERRUPT_VECTOR);
    ApicWrite(APIC_REG_SIV, ((DWORD) KERNEL_SPURIOUS_INTERRUPT_VECTOR) | APIC_SIV_SOFTWARE_ENABLE);
    
    ApicCalibrateTimer();
    return TRUE;
}

VOID ApicSetEnabled(BOOL bEnabled)
{
    DWORD SIVR = ApicRead(APIC_REG_SIV);
    if (bEnabled)
    {
        SIVR |= APIC_SIV_SOFTWARE_ENABLE;
    }
    else
    {
        SIVR &= ~APIC_SIV_SOFTWARE_ENABLE;
    }
    ApicWrite(APIC_REG_SIV, SIVR);
}

VOID ApicSetTimerInterruptsEnabled(BOOL bEnabled)
{   
    DWORD LTR = MmioRead32(g_KernelState.MmioInfo.VaddrApicBase, APIC_REG_LTR);
    if (bEnabled)
    {
        LTR &= ~APIC_LTR_TIMER_INT_BLOCK;
    }
    else
    {
        LTR |= APIC_LTR_TIMER_INT_BLOCK;
    }
    ApicWrite(APIC_REG_LTR, LTR);
}

DWORD ApicGetTimerInitialCount(VOID)
{
    return ApicRead(APIC_REG_INITCR);
}

DWORD ApicGetTimerCurrentCount(VOID)
{
    return ApicRead(APIC_REG_CURRCR);
}

VOID ApicSetTimerCount(DWORD dwInitCnt)
{
    ApicWrite(APIC_REG_INITCR, dwInitCnt);
}

VOID ApicWaitForIcrIdle(VOID)
{
    while (ApicRead(APIC_REG_ICR_LO) & APIC_ICR_DELIVERY_STATUS)
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

    ApicWrite(APIC_REG_ICR_HI, pConfig->dwDestApic << 24);
    ApicWrite(APIC_REG_ICR_LO, dwLoword);

    ApicWaitForIcrIdle();
}

VOID ApicIssueEndOfInt(VOID)
{
    ApicWrite(APIC_REG_EOI, 0);
}

STATIC VOID ApicCalibrateTimer(VOID)
{
    KrDisableInterrupts();

    const UINT NrTicksFor50ms = 178977; // Number of ACPI PM Timer ticks needed to elapse 50 ms (3579545 Hz * 0.05 sec ~= 178977)
    STATIC Spinlock sl_CalibSpinlock = { 0 };
    AcquireOrWaitSpinlock(&sl_CalibSpinlock);
    {
        ApicSetTimerInterruptsEnabled(FALSE);

        DWORD TickCnt;
        QWORD DestTickCnt;
    Recalc:
        AcpiPmTimerGetTicks(&TickCnt);
        DestTickCnt = ((QWORD) TickCnt) + NrTicksFor50ms;

        if ((AcpiPmTimerGetTimerBitWidth() == 24 && DestTickCnt >= 0xFFFF00) || DestTickCnt > DWORD_MAX - 0xFF) // FF tolerance
        {
            KrProcessorStall(50);
            goto Recalc;
        }
        ApicWrite(APIC_REG_INITCR, DWORD_MAX);
        do
        {
            AcpiPmTimerGetTicks(&TickCnt);
        } while (TickCnt < (DWORD) DestTickCnt);

        DWORD ApicCurrent = ApicRead(APIC_REG_CURRCR);
        DWORD dwElapsed = DWORD_MAX - ApicCurrent;

        GetThisCpu()->TimerInf.CalibrationValue = dwElapsed / 50000; // 50 milliseconds to 1 microsecond

        ApicSetTimerCount(0);
        ApicSetTimerInterruptsEnabled(TRUE);
    }
    RelinquishSpinlock(&sl_CalibSpinlock);

    KrEnableInterrupts();
}

INLINE DWORD ApicRead(SIZE RegOffset)
{
    return MmioRead32(g_KernelState.MmioInfo.VaddrApicBase, RegOffset);
}

INLINE DWORD ApicWrite(SIZE RegOffset, DWORD dwValue)
{
    return MmioWrite32(g_KernelState.MmioInfo.VaddrApicBase, RegOffset, dwValue);
}

INLINE VOID DisableLegacyPIC(VOID)
{
    ByteOutputToPort(0x21, 0xFF); // Master PIC Data Port = 0x21, 0xFF to mask it off
    ByteOutputToPort(0xA1, 0xFF); // Slave  PIC Data Port = 0xA1, 0xFF to mask it off
}
