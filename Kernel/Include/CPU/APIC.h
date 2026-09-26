#ifndef YCH_KERNEL_CPU_APIC_H
#define YCH_KERNEL_CPU_APIC_H

#include "Krnlych.h"

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

#define APIC_REG_ID  0x020
#define APIC_REG_VER 0x030
#define APIC_REG_TPR 0x080 // Task Priority Register
#define APIC_REG_APR 0x090 // Arbitration Priority Register
#define APIC_REG_PPR 0x0A0 // Processor Priority Register
#define APIC_REG_EOI 0x0B0 // End of Interrupt

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

VOID KrApicIssueEndOfInt(VOID);

VOID KrApicIssueIpi(const KrIpiConfig* pConfig);

#endif // !YCH_KERNEL_CPU_APIC_H
