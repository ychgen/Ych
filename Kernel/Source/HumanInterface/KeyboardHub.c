#include "HumanInterface/KeyboardHub.h"

#define MAX_KEYBOARD_EVENTS 128

STATIC HiKeyboardEvent KeyboardEventQueue[MAX_KEYBOARD_EVENTS] = { 0 };
STATIC UINT KeyevHeadidx = 0, KeyevTailidx = 0; // Pops from head, pushes from tail
STATIC UINT KeyevCount = 0;

VOID HiPushKeyboardEvent(HiKeyboardEvent Data)
{
    KeyboardEventQueue[KeyevTailidx++] = Data;
    if (KeyevTailidx >= MAX_KEYBOARD_EVENTS)
    {
        KeyevTailidx = 0;
    }
    KeyevCount = MACROMIN(MAX_KEYBOARD_EVENTS, KeyevCount + 1);
}

HiKeyboardEvent HiPopKeyboardEvent(VOID)
{
    HiKeyboardEvent Result = KeyboardEventQueue[KeyevHeadidx++];
    if (KeyevHeadidx >= MAX_KEYBOARD_EVENTS)
    {
        KeyevHeadidx = 0;
    }
    KeyevCount--;
    return Result;
}

UINT HiGetKeyboardEventQueueNum(VOID)
{
    return KeyevCount;
}
