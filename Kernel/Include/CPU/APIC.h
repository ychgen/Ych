#ifndef YCH_KERNEL_CPU_APIC_H
#define YCH_KERNEL_CPU_APIC_H

#include "Krnlych.h"

#define KRX2APIC_IPI_DELIVERY_FIXED       0
#define KRX2APIC_IPI_DELIVERY_SMI         2
#define KRX2APIC_IPI_DELIVERY_NMI         4
#define KRX2APIC_IPI_DELIVERY_INIT        5
#define KRX2APIC_IPI_DELIVERY_SIPI        6

#define KRX2APIC_IPI_DESTINATION_PHYSICAL 0
#define KRX2APIC_IPI_DESTINATION_LOGICAL  1

#define KRX2APIC_IPI_LEVEL_DEASSERT       0
#define KRX2APIC_IPI_LEVEL_ASSERT         1

#define KRX2APIC_IPI_TRIGGER_EDGE         0
#define KRX2APIC_IPI_TRIGGER_LEVEL        1

#define KRX2APIC_IPI_SHORTHAND_NONE       0 // Issues IPI to dwDestApic
#define KRX2APIC_IPI_SHORTHAND_SELF       1 // Self
#define KRX2APIC_IPI_SHORTHAND_ALL_ISLF   2 // All including self
#define KRX2APIC_IPI_SHORTHAND_ALL_XSLF   3 // All excluding self

typedef struct
{
    DWORD dwDestApic;
    UCHAR IntVector;
    UCHAR eDeliveryMode;
    UCHAR eDestMode;
    UCHAR eLevel;
    UCHAR eTrigMode;
    UCHAR eDestShorthand;
} Krx2ApicIpiConfig;

typedef enum
{
    KRX2APIC_IPI_CONFIG_STRUCT_VALIDATION_RESULT_SUCCESS,
    KRX2APIC_IPI_CONFIG_STRUCT_VALIDATION_RESULT_DEST_APIC_TOO_BIG,
    KRX2APIC_IPI_CONFIG_STRUCT_VALIDATION_RESULT_DELIVERY_MODE_NV,
    KRX2APIC_IPI_CONFIG_STRUCT_VALIDATION_RESULT_DESTMODE_NV,
    KRX2APIC_IPI_CONFIG_STRUCT_VALIDATION_RESULT_LEVEL_NV,
    KRX2APIC_IPI_CONFIG_STRUCT_VALIDATION_RESULT_TRIGMODE_NV,
    KRX2APIC_IPI_CONFIG_STRUCT_VALIDATION_RESULT_DEST_SHORTHAND_NV,
    KRX2APIC_IPI_CONFIG_STRUCT_VALIDATION_RESULT_SHORTHAND_WITH_TARGET_ID
} Krx2ApicIpiConfigStructValidationResult;
Krx2ApicIpiConfigStructValidationResult KrValidatex2ApicIpiConfigStruct(const Krx2ApicIpiConfig* pConfig);

/**
 * @brief Checks whether or not the processor is capable of x2APIC.
 * 
 * @return TRUE if capable, FALSE otherwise.
 */
BOOL Krx2CheckSupport(VOID);

/**
 * @brief Enables APIC (x2APIC).
 * 
 * @return TRUE if enabled successfully, false if the processor is incapable of x2APIC.
 */
BOOL Krx2Enable(VOID);

/**
 * @brief Sends an EOI signal to the APIC.
 */
VOID Krx2SignalEndOfInterrupt(VOID);

VOID Krx2ApicIssueIPI(const Krx2ApicIpiConfig* pConfig);

#endif // !YCH_KERNEL_CPU_APIC_H
