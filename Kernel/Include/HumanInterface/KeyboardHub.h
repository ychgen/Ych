#ifndef YCH_KERNEL_HUMAN_INTERFACE_KEYBOARD_HUB_H
#define YCH_KERNEL_HUMAN_INTERFACE_KEYBOARD_HUB_H

#include "HumanInterface/Keycodes.h"

struct DeviceNode;

typedef struct
{
    struct DeviceNode* Source;
    KeyState OldState, NewState;
    KeyCode  Keycode;
    KeyMod   Keymod;
} HiKeyboardEvent; // Human Interface Keyboard Event

VOID HiPushKeyboardEvent(HiKeyboardEvent Data);
HiKeyboardEvent HiPopKeyboardEvent(VOID);
UINT HiGetKeyboardEventQueueNum(VOID);

#endif // !YCH_KERNEL_HUMAN_INTERFACE_KEYBOARD_HUB_H
