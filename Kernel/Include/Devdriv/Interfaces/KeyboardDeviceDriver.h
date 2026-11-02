#ifndef YCH_KERNEL_DEVDRIV_INTERFACES_KEYBOARD_DEVICE_DRIVER_H
#define YCH_KERNEL_DEVDRIV_INTERFACES_KEYBOARD_DEVICE_DRIVER_H

#include "Devdriv/Driver.h"

#include "HumanInterface/Keycodes.h"

typedef struct
{
    Driver Object;
    KeyState (*GetKeyStatus)(KeyCode Keycode);
} KeyboardDeviceDriver;

#endif // !YCH_KERNEL_DEVDRIV_INTERFACES_KEYBOARD_DEVICE_DRIVER_H
