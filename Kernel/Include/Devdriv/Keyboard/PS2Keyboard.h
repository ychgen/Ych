#ifndef YCH_KERNEL_DEVDRIV_KEYBOARD_PS2_KEYBOARD_H
#define YCH_KERNEL_DEVDRIV_KEYBOARD_PS2_KEYBOARD_H

#include "Devdriv/Interfaces/KeyboardDeviceDriver.h"

extern KeyboardDeviceDriver g_PS2_KeyboardDriver;

KeyState PS2_KeyboardGetKeyState(KeyCode Keycode);

#endif // !YCH_KERNEL_DEVDRIV_KEYBOARD_PS2_KEYBOARD_H
