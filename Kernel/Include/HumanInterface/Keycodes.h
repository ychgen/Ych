#ifndef YCH_KERNEL_INPUT_KEYCODES_H
#define YCH_KERNEL_INPUT_KEYCODES_H

#include "Krnlych.h"

typedef DWORD KeyCode;
typedef DWORD KeyState;
typedef DWORD KeyMod;

#define KM_CTRL  (1U << 0)
#define KM_ALT   (1U << 1)
#define KM_SHIFT (1U << 2)

#define KS_RELEASED  0U
#define KS_PRESSED   1U

#define KC_INVALID     0x00

#define KC_BACKSPACE   0x08
#define KC_TAB         0x09
#define KC_ENTER       0x0A
#define KC_CAPS_LOCK   0x14
#define KC_ESCAPE      0x1B
#define KC_SPACE       0x20
#define KC_APOSTROPHE  0x27
#define KC_COMMA       0x2C
#define KC_MINUS       0x2D
#define KC_PERIOD      0x2E
#define KC_SLASH       0x2F

#define KC_0           0x30
#define KC_1           0x31
#define KC_2           0x32
#define KC_3           0x33
#define KC_4           0x34
#define KC_5           0x35
#define KC_6           0x36
#define KC_7           0x37
#define KC_8           0x38
#define KC_9           0x39
#define KC_SEMICOLON   0x3B
#define KC_EQUAL       0x3D

#define KC_A           0x41
#define KC_B           0x42
#define KC_C           0x43
#define KC_D           0x44
#define KC_E           0x45
#define KC_F           0x46
#define KC_G           0x47
#define KC_H           0x48
#define KC_I           0x49
#define KC_J           0x4A
#define KC_K           0x4B
#define KC_L           0x4C
#define KC_M           0x4D
#define KC_N           0x4E
#define KC_O           0x4F
#define KC_P           0x50
#define KC_Q           0x51
#define KC_R           0x52
#define KC_S           0x53
#define KC_T           0x54
#define KC_U           0x55
#define KC_V           0x56
#define KC_W           0x57
#define KC_X           0x58
#define KC_Y           0x59
#define KC_Z           0x5A

#define KC_LBRACKET    0x5B
#define KC_BACKSLASH   0x5C
#define KC_RBRACKET    0x5D

#define KC_BACKTICK    0x60

#define KC_KP_0        0x70
#define KC_KP_1        0x71
#define KC_KP_2        0x72
#define KC_KP_3        0x73
#define KC_KP_4        0x74
#define KC_KP_5        0x75
#define KC_KP_6        0x76
#define KC_KP_7        0x77
#define KC_KP_8        0x78
#define KC_KP_9        0x79
#define KC_KP_ENTER    0x7A
#define KC_KP_ASTERISK 0x7B
#define KC_KP_PLUS     0x7C
#define KC_KP_MINUS    0x7D
#define KC_KP_PERIOD   0x7E
#define KC_KP_SLASH    0x7F

#define KC_LSHIFT      0x80
#define KC_RSHIFT      0x81
#define KC_LCTRL       0x82
#define KC_RCTRL       0x83
#define KC_LALT        0x84
#define KC_RALT        0x85
#define KC_LSUPER      0x86
#define KC_RSUPER      0x87
#define KC_APPS        0x88

#define KC_NUM_LOCK    0x90
#define KC_SCROLL_LOCK 0x91
#define KC_HOME        0x93
#define KC_END         0x94
#define KC_PRINT_SCREEN 0x95 // Print Screen
#define KC_PAUSE       0x96
#define KC_INSERT      0x97
#define KC_PAGE_UP     0x98
#define KC_PAGE_DOWN   0x99
#define KC_DELETE      0x9A

#define KC_LARROW      0xA1 // LEFT
#define KC_RARROW      0xA2 // RIGHT
#define KC_UARROW      0xA4 // UP
#define KC_DARROW      0xA8 // DOWN

#define KC_VOLMUTE     0xB0
#define KC_VOLDOWN     0xB1
#define KC_VOLUP       0xB2

#define KC_F0          0xF0
#define KC_F1          0xF1
#define KC_F2          0xF2
#define KC_F3          0xF3
#define KC_F4          0xF4
#define KC_F5          0xF5
#define KC_F6          0xF6
#define KC_F7          0xF7
#define KC_F8          0xF8
#define KC_F9          0xF9
#define KC_F10         0xFA
#define KC_F11         0xFB
#define KC_F12         0xFC

CSTR KeyStateToString(KeyState State);
CSTR KeycodeToString(KeyCode Keycode);

#endif // !YCH_KERNEL_INPUT_KEYCODES_H
