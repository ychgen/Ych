#ifndef YCH_KERNEL_DEVDRIV_INTERFACES_TIMER_DEVICE_DRIVER_H
#define YCH_KERNEL_DEVDRIV_INTERFACES_TIMER_DEVICE_DRIVER_H

#include "Devdriv/Driver.h"

typedef struct
{
    Driver Object;

    VOID (*GetTicks)(VOID* pOutData);
    UINT (*GetTimerBitWidth)(VOID);
    UINT (*GetTimerFrequency)(VOID); // in Hz
} TimerDeviceDriver;

#endif // !YCH_KERNEL_DEVDRIV_INTERFACES_TIMER_DEVICE_DRIVER_H
