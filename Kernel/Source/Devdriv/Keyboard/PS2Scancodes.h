#ifndef YCH_KERNEL_DEVDRIV_KEYBOARD_PS2_SCANCODES_H
#define YCH_KERNEL_DEVDRIV_KEYBOARD_PS2_SCANCODES_H

#include "Krnlych.h"

#include "HumanInterface/Keycodes.h"

#include "Earlyvideo/DisplaywideTextProtocol.h"

// Scancodes for Scancode Set 2
// XSCANCODE comes first (if any), BREAK_PREFIX comes second (if any), followed by actual key scancode

// These three fuckers are special:
// PRINT_SCREEN_PRESS   = 0xE0, 0x12, 0xE0, 0x7C
// PRINT_SCREEN_RELEASE = 0xE0, 0xF0, 0x7C, 0xE0, 0xF0, 0x12
// PAUSE_PRESSED        = 0xE1, 0x14, 0x77, 0xE1, 0xF0, 0x14, 0xF0, 0x77

#define PS2_XSCANCODE            0xE0
#define PS2_BREAK_PREFIX         0xF0

#define PS2_SCANCODE_F9          0x01
#define PS2_SCANCODE_F5          0x03
#define PS2_SCANCODE_F3          0x04
#define PS2_SCANCODE_F1          0x05
#define PS2_SCANCODE_F2          0x06
#define PS2_SCANCODE_F12         0x07
#define PS2_SCANCODE_F10         0x09
#define PS2_SCANCODE_F8          0x0A
#define PS2_SCANCODE_F6          0x0B
#define PS2_SCANCODE_F4          0x0C
#define PS2_SCANCODE_TAB         0x0D
#define PS2_SCANCODE_BACKTICK    0x0E
#define PS2_SCANCODE_LALT        0x11
#define PS2_SCANCODE_LSHIFT      0x12
#define PS2_SCANCODE_LCTRL       0x14
#define PS2_SCANCODE_Q           0x15
#define PS2_SCANCODE_1           0x16
#define PS2_SCANCODE_Z           0x1A
#define PS2_SCANCODE_S           0x1B
#define PS2_SCANCODE_A           0x1C
#define PS2_SCANCODE_W           0x1D
#define PS2_SCANCODE_2           0x1E
#define PS2_SCANCODE_C           0x21
#define PS2_SCANCODE_X           0x22
#define PS2_SCANCODE_D           0x23
#define PS2_SCANCODE_E           0x24
#define PS2_SCANCODE_4           0x25
#define PS2_SCANCODE_3           0x26
#define PS2_SCANCODE_SPACE       0x29
#define PS2_SCANCODE_V           0x2A
#define PS2_SCANCODE_F           0x2B
#define PS2_SCANCODE_T           0x2C
#define PS2_SCANCODE_R           0x2D
#define PS2_SCANCODE_5           0x2E
#define PS2_SCANCODE_N           0x31
#define PS2_SCANCODE_B           0x32
#define PS2_SCANCODE_H           0x33
#define PS2_SCANCODE_G           0x34
#define PS2_SCANCODE_Y           0x35
#define PS2_SCANCODE_6           0x36
#define PS2_SCANCODE_M           0x3A
#define PS2_SCANCODE_J           0x3B
#define PS2_SCANCODE_U           0x3C
#define PS2_SCANCODE_7           0x3D
#define PS2_SCANCODE_8           0x3E
#define PS2_SCANCODE_COMMA       0x41
#define PS2_SCANCODE_K           0x42
#define PS2_SCANCODE_I           0x43
#define PS2_SCANCODE_O           0x44
#define PS2_SCANCODE_0           0x45
#define PS2_SCANCODE_9           0x46
#define PS2_SCANCODE_PERIOD      0x49 // DOT
#define PS2_SCANCODE_SLASH       0x4A
#define PS2_SCANCODE_L           0x4B
#define PS2_SCANCODE_SEMICOLON   0x4C
#define PS2_SCANCODE_P           0x4D
#define PS2_SCANCODE_MINUS       0x4E
#define PS2_SCANCODE_APOSTROPHE  0x52
#define PS2_SCANCODE_LBRACKET    0x54
#define PS2_SCANCODE_EQUAL       0x55
#define PS2_SCANCODE_CAPS_LOCK   0x58
#define PS2_SCANCODE_RSHIFT      0x59
#define PS2_SCANCODE_ENTER       0x5A
#define PS2_SCANCODE_RBRACKET    0x5B
#define PS2_SCANCODE_BACKSLASH   0x5D
#define PS2_SCANCODE_BACKSPACE   0x66
#define PS2_SCANCODE_KP_1        0x69
#define PS2_SCANCODE_KP_4        0x6B
#define PS2_SCANCODE_KP_7        0x6C
#define PS2_SCANCODE_KP_0        0x70
#define PS2_SCANCODE_KP_PERIOD   0x71
#define PS2_SCANCODE_KP_2        0x72
#define PS2_SCANCODE_KP_5        0x73
#define PS2_SCANCODE_KP_6        0x74
#define PS2_SCANCODE_KP_8        0x75
#define PS2_SCANCODE_ESCAPE      0x76
#define PS2_SCANCODE_NUM_LOCK    0x77
#define PS2_SCANCODE_F11         0x78
#define PS2_SCANCODE_KP_PLUS     0x79
#define PS2_SCANCODE_KP_3        0x7A
#define PS2_SCANCODE_KP_MINUS    0x7B
#define PS2_SCANCODE_KP_ASTERISK 0x7C
#define PS2_SCANCODE_KP_9        0x7D
#define PS2_SCANCODE_SCROLL_LOCK 0x7E
#define PS2_SCANCODE_F7          0x83

#define PS2_XSCANCODE_SEARCH     0x10
#define PS2_XSCANCODE_RALT       0x11
#define PS2_XSCANCODE_RCTRL      0x14
#define PS2_XSCANCODE_PREV_TRACK 0x15
#define PS2_XSCANCODE_FAVORITES  0x18
#define PS2_XSCANCODE_LGUI       0x1F
#define PS2_XSCANCODE_REFRESH    0x20
#define PS2_XSCANCODE_VOLDOWN    0x21
#define PS2_XSCANCODE_VOLMUTE    0x23
#define PS2_XSCANCODE_RGUI       0x27
#define PS2_XSCANCODE_STOP       0x28
#define PS2_XSCANCODE_CALC       0x2B
#define PS2_XSCANCODE_FORWARD    0x30
#define PS2_XSCANCODE_VOLUP      0x32
#define PS2_XSCANCODE_PLAYPAUSE  0x34
#define PS2_XSCANCODE_POWER      0x37
#define PS2_XSCANCODE_BACK       0x38
#define PS2_XSCANCODE_HOME       0x3A
#define PS2_XSCANCODE_STOP       0x3B
#define PS2_XSCANCODE_SLEEP      0x3F
#define PS2_XSCANCODE_MY_COMP    0x40
#define PS2_XSCANCODE_EMAIL      0x48
#define PS2_XSCANCODE_KP_SLASH   0x4A
#define PS2_XSCANCODE_NEXT_TRACK 0x4D
#define PS2_XSCANCODE_MEDIASEL   0x50
#define PS2_XSCANCODE_KP_ENTER   0x5A
#define PS2_XSCANCODE_WAKE       0x5E
#define PS2_XSCANCODE_END        0x69
#define PS2_XSCANCODE_LEFT       0x6B
#define PS2_XSCANCODE_HOME       0x6C
#define PS2_XSCANCODE_INSERT     0x70
#define PS2_XSCANCODE_DELETE     0x71
#define PS2_XSCANCODE_DOWN       0x72
#define PS2_XSCANCODE_RIGHT      0x74
#define PS2_XSCANCODE_UP         0x75
#define PS2_XSCANCODE_PAGE_DOWN  0x7A
#define PS2_XSCANCODE_PAGE_UP    0x7D

KeyCode StandardScancodeToKeycodeMap[] =
{
    /*0x00*/ KC_INVALID,
    /*0x01*/ KC_F9,
    /*0x02*/ KC_INVALID,
    /*0x03*/ KC_F5,
    /*0x04*/ KC_F3,
    /*0x05*/ KC_F1,
    /*0x06*/ KC_F2,
    /*0x07*/ KC_F12,
    /*0x08*/ KC_INVALID,
    /*0x09*/ KC_F10,
    /*0x0A*/ KC_F8,
    /*0x0B*/ KC_F6,
    /*0x0C*/ KC_F4,
    /*0x0D*/ KC_TAB,
    /*0x0E*/ KC_BACKTICK,
    /*0x0F*/ KC_INVALID,
    /*0x10*/ KC_INVALID,
    /*0x11*/ KC_LALT,
    /*0x12*/ KC_LSHIFT,
    /*0x13*/ KC_INVALID,
    /*0x14*/ KC_LCTRL,
    /*0x15*/ KC_Q,
    /*0x16*/ KC_1,
    /*0x17*/ KC_INVALID,
    /*0x18*/ KC_INVALID,
    /*0x19*/ KC_INVALID,
    /*0x1A*/ KC_Z,
    /*0x1B*/ KC_S,
    /*0x1C*/ KC_A,
    /*0x1D*/ KC_W,
    /*0x1E*/ KC_2,
    /*0x1F*/ KC_INVALID,
    /*0x20*/ KC_INVALID,
    /*0x21*/ KC_C,
    /*0x22*/ KC_X,
    /*0x23*/ KC_D,
    /*0x24*/ KC_E,
    /*0x25*/ KC_4,
    /*0x26*/ KC_3,
    /*0x27*/ KC_INVALID,
    /*0x28*/ KC_INVALID,
    /*0x29*/ KC_SPACE,
    /*0x2A*/ KC_V,
    /*0x2B*/ KC_F,
    /*0x2C*/ KC_T,
    /*0x2D*/ KC_R,
    /*0x2E*/ KC_5,
    /*0x2F*/ KC_INVALID,
    /*0x30*/ KC_INVALID,
    /*0x31*/ KC_N,
    /*0x32*/ KC_B,
    /*0x33*/ KC_H,
    /*0x34*/ KC_G,
    /*0x35*/ KC_Y,
    /*0x36*/ KC_6,
    /*0x37*/ KC_INVALID,
    /*0x38*/ KC_INVALID,
    /*0x39*/ KC_INVALID,
    /*0x3A*/ KC_M,
    /*0x3B*/ KC_J,
    /*0x3C*/ KC_U,
    /*0x3D*/ KC_7,
    /*0x3E*/ KC_8,
    /*0x3F*/ KC_INVALID,
    /*0x40*/ KC_INVALID,
    /*0x41*/ KC_COMMA,
    /*0x42*/ KC_K,
    /*0x43*/ KC_I,
    /*0x44*/ KC_O,
    /*0x45*/ KC_0,
    /*0x46*/ KC_9,
    /*0x47*/ KC_INVALID,
    /*0x48*/ KC_INVALID,
    /*0x49*/ KC_PERIOD,
    /*0x4A*/ KC_SLASH,
    /*0x4B*/ KC_L,
    /*0x4C*/ KC_SEMICOLON,
    /*0x4D*/ KC_P,
    /*0x4E*/ KC_MINUS,
    /*0x4F*/ KC_INVALID,
    /*0x50*/ KC_INVALID,
    /*0x51*/ KC_INVALID,
    /*0x52*/ KC_APOSTROPHE,
    /*0x53*/ KC_INVALID,
    /*0x54*/ KC_LBRACKET,
    /*0x55*/ KC_EQUAL,
    /*0x56*/ KC_INVALID,
    /*0x57*/ KC_INVALID,
    /*0x58*/ KC_CAPS_LOCK,
    /*0x59*/ KC_RSHIFT,
    /*0x5A*/ KC_ENTER,
    /*0x5B*/ KC_RBRACKET,
    /*0x5C*/ KC_INVALID,
    /*0x5D*/ KC_BACKSLASH,
    /*0x5E*/ KC_INVALID,
    /*0x5F*/ KC_INVALID,
    /*0x60*/ KC_INVALID,
    /*0x61*/ KC_INVALID,
    /*0x62*/ KC_INVALID,
    /*0x63*/ KC_INVALID,
    /*0x64*/ KC_INVALID,
    /*0x65*/ KC_INVALID,
    /*0x66*/ KC_BACKSPACE,
    /*0x67*/ KC_INVALID,
    /*0x68*/ KC_INVALID,
    /*0x69*/ KC_KP_1,
    /*0x6A*/ KC_INVALID,
    /*0x6B*/ KC_KP_4,
    /*0x6C*/ KC_KP_7,
    /*0x6D*/ KC_INVALID,
    /*0x6E*/ KC_INVALID,
    /*0x6F*/ KC_INVALID,
    /*0x70*/ KC_KP_0,
    /*0x71*/ KC_KP_PERIOD,
    /*0x72*/ KC_KP_2,
    /*0x73*/ KC_KP_5,
    /*0x74*/ KC_KP_6,
    /*0x75*/ KC_KP_8,
    /*0x76*/ KC_ESCAPE,
    /*0x77*/ KC_NUM_LOCK,
    /*0x78*/ KC_F11,
    /*0x79*/ KC_KP_PLUS,
    /*0x7A*/ KC_KP_3,
    /*0x7B*/ KC_KP_MINUS,
    /*0x7C*/ KC_KP_ASTERISK,
    /*0x7D*/ KC_KP_9,
    /*0x7E*/ KC_SCROLL_LOCK,
    /*0x7F*/ KC_INVALID,
    /*0x80*/ KC_INVALID,
    /*0x81*/ KC_INVALID,
    /*0x82*/ KC_INVALID,
    /*0x83*/ KC_F7
};

KeyCode ExtendedScancodeToKeycodeMap[] =
{
    /*0x00*/ KC_INVALID,
    /*0x01*/ KC_INVALID,
    /*0x02*/ KC_INVALID,
    /*0x03*/ KC_INVALID,
    /*0x04*/ KC_INVALID,
    /*0x05*/ KC_INVALID,
    /*0x06*/ KC_INVALID,
    /*0x07*/ KC_INVALID,
    /*0x08*/ KC_INVALID,
    /*0x09*/ KC_INVALID,
    /*0x0A*/ KC_INVALID,
    /*0x0B*/ KC_INVALID,
    /*0x0C*/ KC_INVALID,
    /*0x0D*/ KC_INVALID,
    /*0x0E*/ KC_INVALID,
    /*0x0F*/ KC_INVALID,
    /*0x10*/ KC_INVALID,
    /*0x11*/ KC_RALT,
    /*0x12*/ KC_INVALID,
    /*0x13*/ KC_INVALID,
    /*0x14*/ KC_RCTRL,
    /*0x15*/ KC_INVALID,
    /*0x16*/ KC_INVALID,
    /*0x17*/ KC_INVALID,
    /*0x18*/ KC_INVALID,
    /*0x19*/ KC_INVALID,
    /*0x1A*/ KC_INVALID,
    /*0x1B*/ KC_INVALID,
    /*0x1C*/ KC_INVALID,
    /*0x1D*/ KC_INVALID,
    /*0x1E*/ KC_INVALID,
    /*0x1F*/ KC_LSUPER,
    /*0x20*/ KC_INVALID,
    /*0x21*/ KC_VOLDOWN,
    /*0x22*/ KC_INVALID,
    /*0x23*/ KC_VOLMUTE,
    /*0x24*/ KC_INVALID,
    /*0x25*/ KC_INVALID,
    /*0x26*/ KC_INVALID,
    /*0x27*/ KC_RSUPER,
    /*0x28*/ KC_INVALID,
    /*0x29*/ KC_INVALID,
    /*0x2A*/ KC_INVALID,
    /*0x2B*/ KC_INVALID,
    /*0x2C*/ KC_INVALID,
    /*0x2D*/ KC_INVALID,
    /*0x2E*/ KC_INVALID,
    /*0x2F*/ KC_APPS,
    /*0x30*/ KC_INVALID,
    /*0x31*/ KC_INVALID,
    /*0x32*/ KC_VOLUP,
    /*0x33*/ KC_INVALID,
    /*0x34*/ KC_INVALID,
    /*0x35*/ KC_INVALID,
    /*0x36*/ KC_INVALID,
    /*0x37*/ KC_INVALID,
    /*0x38*/ KC_INVALID,
    /*0x39*/ KC_INVALID,
    /*0x3A*/ KC_INVALID,
    /*0x3B*/ KC_INVALID,
    /*0x3C*/ KC_INVALID,
    /*0x3D*/ KC_INVALID,
    /*0x3E*/ KC_INVALID,
    /*0x3F*/ KC_INVALID,
    /*0x40*/ KC_INVALID,
    /*0x41*/ KC_INVALID,
    /*0x42*/ KC_INVALID,
    /*0x43*/ KC_INVALID,
    /*0x44*/ KC_INVALID,
    /*0x45*/ KC_INVALID,
    /*0x46*/ KC_INVALID,
    /*0x47*/ KC_INVALID,
    /*0x48*/ KC_INVALID,
    /*0x49*/ KC_INVALID,
    /*0x4A*/ KC_KP_SLASH,
    /*0x4B*/ KC_INVALID,
    /*0x4C*/ KC_INVALID,
    /*0x4D*/ KC_INVALID,
    /*0x4E*/ KC_INVALID,
    /*0x4F*/ KC_INVALID,
    /*0x50*/ KC_INVALID,
    /*0x51*/ KC_INVALID,
    /*0x52*/ KC_INVALID,
    /*0x53*/ KC_INVALID,
    /*0x54*/ KC_INVALID,
    /*0x55*/ KC_INVALID,
    /*0x56*/ KC_INVALID,
    /*0x57*/ KC_INVALID,
    /*0x58*/ KC_INVALID,
    /*0x59*/ KC_INVALID,
    /*0x5A*/ KC_KP_ENTER,
    /*0x5B*/ KC_INVALID,
    /*0x5C*/ KC_INVALID,
    /*0x5D*/ KC_INVALID,
    /*0x5E*/ KC_INVALID,
    /*0x5F*/ KC_INVALID,
    /*0x60*/ KC_INVALID,
    /*0x61*/ KC_INVALID,
    /*0x62*/ KC_INVALID,
    /*0x63*/ KC_INVALID,
    /*0x64*/ KC_INVALID,
    /*0x65*/ KC_INVALID,
    /*0x66*/ KC_INVALID,
    /*0x67*/ KC_INVALID,
    /*0x68*/ KC_INVALID,
    /*0x69*/ KC_END,
    /*0x6A*/ KC_INVALID,
    /*0x6B*/ KC_LARROW,
    /*0x6C*/ KC_HOME,
    /*0x6D*/ KC_INVALID,
    /*0x6E*/ KC_INVALID,
    /*0x6F*/ KC_INVALID,
    /*0x70*/ KC_INSERT,
    /*0x71*/ KC_DELETE,
    /*0x72*/ KC_DARROW,
    /*0x73*/ KC_INVALID,
    /*0x74*/ KC_RARROW,
    /*0x75*/ KC_UARROW,
    /*0x76*/ KC_INVALID,
    /*0x77*/ KC_INVALID,
    /*0x78*/ KC_INVALID,
    /*0x79*/ KC_INVALID,
    /*0x7A*/ KC_PAGE_DOWN,
    /*0x7B*/ KC_INVALID,
    /*0x7C*/ KC_INVALID,
    /*0x7D*/ KC_PAGE_UP
};

UCHAR PS2_KeycodeToScancode(KeyCode Keycode, BOOL* OutExtended)
{
    if (!OutExtended)
    {
        return 0x00;
    }

    switch (Keycode)
    {
    #define Case(Keycode, Scancode, ExtendedState) case Keycode: *OutExtended = ExtendedState; return Scancode
        Case(KC_BACKSPACE, PS2_SCANCODE_BACKSPACE, FALSE);
        Case(KC_TAB, PS2_SCANCODE_TAB, FALSE);
        Case(KC_ENTER, PS2_SCANCODE_ENTER, FALSE);
        Case(KC_CAPS_LOCK, PS2_SCANCODE_CAPS_LOCK, FALSE);
        Case(KC_ESCAPE, PS2_SCANCODE_ESCAPE, FALSE);
        Case(KC_SPACE, PS2_SCANCODE_SPACE, FALSE);
        Case(KC_APOSTROPHE, PS2_SCANCODE_APOSTROPHE, FALSE);
        Case(KC_COMMA, PS2_SCANCODE_COMMA, FALSE);
        Case(KC_MINUS, PS2_SCANCODE_MINUS, FALSE);
        Case(KC_PERIOD, PS2_SCANCODE_PERIOD, FALSE);
        Case(KC_SLASH, PS2_SCANCODE_SLASH, FALSE);
        Case(KC_0, PS2_SCANCODE_0, FALSE);
        Case(KC_1, PS2_SCANCODE_1, FALSE);
        Case(KC_2, PS2_SCANCODE_2, FALSE);
        Case(KC_3, PS2_SCANCODE_3, FALSE);
        Case(KC_4, PS2_SCANCODE_4, FALSE);
        Case(KC_5, PS2_SCANCODE_5, FALSE);
        Case(KC_6, PS2_SCANCODE_6, FALSE);
        Case(KC_7, PS2_SCANCODE_7, FALSE);
        Case(KC_8, PS2_SCANCODE_8, FALSE);
        Case(KC_9, PS2_SCANCODE_9, FALSE);
        Case(KC_SEMICOLON, PS2_SCANCODE_SEMICOLON, FALSE);
        Case(KC_EQUAL, PS2_SCANCODE_EQUAL, FALSE);
        Case(KC_A, PS2_SCANCODE_A, FALSE);
        Case(KC_B, PS2_SCANCODE_B, FALSE);
        Case(KC_C, PS2_SCANCODE_C, FALSE);
        Case(KC_D, PS2_SCANCODE_D, FALSE);
        Case(KC_E, PS2_SCANCODE_E, FALSE);
        Case(KC_F, PS2_SCANCODE_F, FALSE);
        Case(KC_G, PS2_SCANCODE_G, FALSE);
        Case(KC_H, PS2_SCANCODE_H, FALSE);
        Case(KC_I, PS2_SCANCODE_I, FALSE);
        Case(KC_J, PS2_SCANCODE_J, FALSE);
        Case(KC_K, PS2_SCANCODE_K, FALSE);
        Case(KC_L, PS2_SCANCODE_L, FALSE);
        Case(KC_M, PS2_SCANCODE_M, FALSE);
        Case(KC_N, PS2_SCANCODE_N, FALSE);
        Case(KC_O, PS2_SCANCODE_O, FALSE);
        Case(KC_P, PS2_SCANCODE_P, FALSE);
        Case(KC_Q, PS2_SCANCODE_Q, FALSE);
        Case(KC_R, PS2_SCANCODE_R, FALSE);
        Case(KC_S, PS2_SCANCODE_S, FALSE);
        Case(KC_T, PS2_SCANCODE_T, FALSE);
        Case(KC_U, PS2_SCANCODE_U, FALSE);
        Case(KC_V, PS2_SCANCODE_V, FALSE);
        Case(KC_W, PS2_SCANCODE_W, FALSE);
        Case(KC_X, PS2_SCANCODE_X, FALSE);
        Case(KC_Y, PS2_SCANCODE_Y, FALSE);
        Case(KC_Z, PS2_SCANCODE_Z, FALSE);

        Case(KC_LBRACKET, PS2_SCANCODE_LBRACKET, FALSE);
        Case(KC_BACKSLASH, PS2_SCANCODE_BACKSLASH, FALSE);
        Case(KC_RBRACKET, PS2_SCANCODE_RBRACKET, FALSE);

        Case(KC_BACKTICK, PS2_SCANCODE_BACKTICK, FALSE);

        Case(KC_KP_0, PS2_SCANCODE_KP_0, FALSE);
        Case(KC_KP_1, PS2_SCANCODE_KP_1, FALSE);
        Case(KC_KP_2, PS2_SCANCODE_KP_2, FALSE);
        Case(KC_KP_3, PS2_SCANCODE_KP_3, FALSE);
        Case(KC_KP_4, PS2_SCANCODE_KP_4, FALSE);
        Case(KC_KP_5, PS2_SCANCODE_KP_5, FALSE);
        Case(KC_KP_6, PS2_SCANCODE_KP_6, FALSE);
        Case(KC_KP_7, PS2_SCANCODE_KP_7, FALSE);
        Case(KC_KP_8, PS2_SCANCODE_KP_8, FALSE);
        Case(KC_KP_9, PS2_SCANCODE_KP_9, FALSE);
        Case(KC_KP_ENTER, PS2_XSCANCODE_KP_ENTER, TRUE);
        Case(KC_KP_ASTERISK, PS2_SCANCODE_KP_ASTERISK, FALSE);
        Case(KC_KP_PLUS, PS2_SCANCODE_KP_PLUS, FALSE);
        Case(KC_KP_MINUS, PS2_SCANCODE_KP_MINUS, FALSE);
        Case(KC_KP_PERIOD, PS2_SCANCODE_KP_PERIOD, FALSE);
        Case(KC_KP_SLASH, PS2_XSCANCODE_KP_SLASH, TRUE);

        Case(KC_LSHIFT, PS2_SCANCODE_LSHIFT, FALSE);
        Case(KC_RSHIFT, PS2_SCANCODE_RSHIFT, FALSE);
        Case(KC_LCTRL, PS2_SCANCODE_LCTRL, FALSE);
        Case(KC_RCTRL, PS2_XSCANCODE_RCTRL, TRUE);
        Case(KC_LALT, PS2_SCANCODE_LALT, FALSE);
        Case(KC_RALT, PS2_XSCANCODE_RALT, TRUE);
        Case(KC_LSUPER, PS2_XSCANCODE_LGUI, TRUE);
        Case(KC_RSUPER, PS2_XSCANCODE_RGUI, TRUE);
        Case(KC_APPS, 0x2F, TRUE);
        Case(KC_NUM_LOCK, PS2_SCANCODE_NUM_LOCK, FALSE);
        Case(KC_SCROLL_LOCK, PS2_SCANCODE_SCROLL_LOCK, FALSE);
        Case(KC_HOME, PS2_XSCANCODE_HOME, TRUE);
        Case(KC_END, PS2_XSCANCODE_END, TRUE);
        Case(KC_PRINT_SCREEN, 0xF7, FALSE); // SPECIAL CASE
        Case(KC_PAUSE, 0xF8, FALSE); // SPECIAL CASE
        Case(KC_INSERT, PS2_XSCANCODE_INSERT, TRUE);
        Case(KC_PAGE_UP, PS2_XSCANCODE_PAGE_UP, TRUE);
        Case(KC_PAGE_DOWN, PS2_XSCANCODE_PAGE_DOWN, TRUE);
        Case(KC_DELETE, PS2_XSCANCODE_DELETE, TRUE);

        Case(KC_LARROW, PS2_XSCANCODE_LEFT, TRUE);
        Case(KC_RARROW, PS2_XSCANCODE_RIGHT, TRUE);
        Case(KC_UARROW, PS2_XSCANCODE_UP, TRUE);
        Case(KC_DARROW, PS2_XSCANCODE_DOWN, TRUE);

        Case(KC_VOLMUTE, PS2_XSCANCODE_VOLMUTE, TRUE);
        Case(KC_VOLDOWN, PS2_XSCANCODE_VOLDOWN, TRUE);
        Case(KC_VOLUP, PS2_XSCANCODE_VOLUP, TRUE);

        Case(KC_F1, PS2_SCANCODE_F1, FALSE);
        Case(KC_F2, PS2_SCANCODE_F2, FALSE);
        Case(KC_F3, PS2_SCANCODE_F3, FALSE);
        Case(KC_F4, PS2_SCANCODE_F4, FALSE);
        Case(KC_F5, PS2_SCANCODE_F5, FALSE);
        Case(KC_F6, PS2_SCANCODE_F6, FALSE);
        Case(KC_F7, PS2_SCANCODE_F7, FALSE);
        Case(KC_F8, PS2_SCANCODE_F8, FALSE);
        Case(KC_F9, PS2_SCANCODE_F9, FALSE);
        Case(KC_F10, PS2_SCANCODE_F10, FALSE);
        Case(KC_F11, PS2_SCANCODE_F11, FALSE);
        Case(KC_F12, PS2_SCANCODE_F12, FALSE);
    #undef Case
    default: break;
    }
    return KC_INVALID;
}

KeyCode PS2_ScancodeToKeycode(BOOL bExtend, UCHAR Scancode)
{
    if (bExtend)
    {
        return (UINT) Scancode > sizeof(ExtendedScancodeToKeycodeMap) ? KC_INVALID : ExtendedScancodeToKeycodeMap[Scancode];
    }
    return (UINT) Scancode > sizeof(StandardScancodeToKeycodeMap) ? KC_INVALID : StandardScancodeToKeycodeMap[Scancode];
}

#endif // !YCH_KERNEL_DEVDRIV_KEYBOARD_PS2_SCANCODES_H
