#ifndef YCH_KERNEL_CPU_APIC_H
#define YCH_KERNEL_CPU_APIC_H

#include "Krnlych.h"

#define LOCAL_APIC_BASE_ADDRESS  0xFEE00000

#define IPI_DELIVERY_FIXED       0
#define IPI_DELIVERY_SMI         2
#define IPI_DELIVERY_NMI         4
#define IPI_DELIVERY_INIT        5
#define IPI_DELIVERY_SIPI        6

#define IPI_DESTINATION_PHYSICAL 0
#define IPI_DESTINATION_LOGICAL  1

#define IPI_LEVEL_DEASSERT       0
#define IPI_LEVEL_ASSERT         1

#define IPI_TRIGGER_EDGE         0
#define IPI_TRIGGER_LEVEL        1

#define IPI_SHORTHAND_NONE       0 // Issues IPI to dwDestApic
#define IPI_SHORTHAND_SELF       1 // Self
#define IPI_SHORTHAND_ALL_ISLF   2 // All including self
#define IPI_SHORTHAND_ALL_XSLF   3 // All excluding self

#define APIC_REG_ID     0x020
#define APIC_REG_VER    0x030
#define APIC_REG_TPR    0x080 // Task Priority Register
#define APIC_REG_APR    0x090 // Arbitration Priority Register
#define APIC_REG_PPR    0x0A0 // Processor Priority Register
#define APIC_REG_EOI    0x0B0 // End of Interrupt
#define APIC_REG_RRD    0x0C0 // Remote Read Register
#define APIC_REG_LDR    0x0D0 // Local Destination Register
#define APIC_REG_DFR    0x0E0 // Destination Format Register
#define APIC_REG_SIV    0x0F0 // Spurious Interrupt Vector Register
#define APIC_REG_ICR_LO 0x300 // Interrupt Command Register Lodword
#define APIC_REG_ICR_HI 0x310 // Interrupt Command Register Hidword

#define APIC_ICR_DELIVERY_STATUS (1 << 12)

typedef struct
{
    DWORD dwDestApic;
    UCHAR IntVector;
    UCHAR eDeliveryMode;
    UCHAR eDestMode;
    UCHAR eLevel;
    UCHAR eTrigMode;
    UCHAR eDestShorthand;
} KrIpiConfig;

typedef enum
{
    IPI_CONFIG_STRUCT_VALIDATION_RESULT_SUCCESS,
    IPI_CONFIG_STRUCT_VALIDATION_RESULT_DEST_APIC_TOO_BIG,
    IPI_CONFIG_STRUCT_VALIDATION_RESULT_DELIVERY_MODE_NV,
    IPI_CONFIG_STRUCT_VALIDATION_RESULT_DESTMODE_NV,
    IPI_CONFIG_STRUCT_VALIDATION_RESULT_LEVEL_NV,
    IPI_CONFIG_STRUCT_VALIDATION_RESULT_TRIGMODE_NV,
    IPI_CONFIG_STRUCT_VALIDATION_RESULT_DEST_SHORTHAND_NV,
    IPI_CONFIG_STRUCT_VALIDATION_RESULT_SHORTHAND_WITH_TARGET_ID
} KrIpiConfigStructValidationResult;
KrIpiConfigStructValidationResult KrValidateIpiConfigStruct(const KrIpiConfig* pConfig);

UINTPTR KrApicGetPhysicalBase(VOID);
VOID KrApicSetPhysicalBase(UINTPTR PhysAddr);

BOOL KrApicCheckSupport(VOID);
BOOL KrApicInit(VOID);

VOID ApicIssueEndOfInt(VOID);

VOID ApicWaitForIcrIdle(VOID);
VOID ApicIssueIpi(const KrIpiConfig* pConfig);

#endif // !YCH_KERNEL_CPU_APIC_H
