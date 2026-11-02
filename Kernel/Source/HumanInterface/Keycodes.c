#include "HumanInterface/Keycodes.h"

CSTR KeyStateToString(KeyState State)
{
    switch (State)
    {
    case KS_RELEASED:  return "Released";
    case KS_PRESSED:   return "Pressed";
    default: break;
    }
    return "INVALID";
}

CSTR KeycodeToString(KeyCode Keycode)
{
    switch (Keycode)
    {
    #define CASE(X) case KC_##X: return #X
        CASE(BACKSPACE);
        CASE(TAB);
        CASE(ENTER);
        CASE(CAPS_LOCK);
        CASE(ESCAPE);
        CASE(SPACE);
        CASE(APOSTROPHE);
        CASE(COMMA);
        CASE(MINUS);
        CASE(PERIOD);
        CASE(SLASH);
        CASE(0);
        CASE(1);
        CASE(2);
        CASE(3);
        CASE(4);
        CASE(5);
        CASE(6);
        CASE(7);
        CASE(8);
        CASE(9);
        CASE(SEMICOLON);
        CASE(EQUAL);
        CASE(A);
        CASE(B);
        CASE(C);
        CASE(D);
        CASE(E);
        CASE(F);
        CASE(G);
        CASE(H);
        CASE(I);
        CASE(J);
        CASE(K);
        CASE(L);
        CASE(M);
        CASE(N);
        CASE(O);
        CASE(P);
        CASE(Q);
        CASE(R);
        CASE(S);
        CASE(T);
        CASE(U);
        CASE(V);
        CASE(W);
        CASE(X);
        CASE(Y);
        CASE(Z);
        CASE(LBRACKET);
        CASE(BACKSLASH);
        CASE(RBRACKET);
        CASE(BACKTICK);
        CASE(KP_0);
        CASE(KP_1);
        CASE(KP_2);
        CASE(KP_3);
        CASE(KP_4);
        CASE(KP_5);
        CASE(KP_6);
        CASE(KP_7);
        CASE(KP_8);
        CASE(KP_9);
        CASE(KP_ENTER);
        CASE(KP_ASTERISK);
        CASE(KP_PLUS);
        CASE(KP_MINUS);
        CASE(KP_PERIOD);
        CASE(KP_SLASH);
        CASE(LSHIFT);
        CASE(RSHIFT);
        CASE(LCTRL);
        CASE(RCTRL);
        CASE(LALT);
        CASE(RALT);
        CASE(LSUPER);
        CASE(RSUPER);
        CASE(APPS);
        CASE(NUM_LOCK);
        CASE(SCROLL_LOCK);
        CASE(HOME);
        CASE(END);
        CASE(PRINT_SCREEN);
        CASE(PAUSE);
        CASE(INSERT);
        CASE(PAGE_UP);
        CASE(PAGE_DOWN);
        CASE(DELETE);
        CASE(LARROW);
        CASE(RARROW);
        CASE(UARROW);
        CASE(DARROW);
        CASE(VOLMUTE);
        CASE(VOLUP);
        CASE(F1);
        CASE(F2);
        CASE(F3);
        CASE(F4);
        CASE(F5);
        CASE(F7);
        CASE(F8);
        CASE(F9);
        CASE(F10);
        CASE(F11);
        CASE(F12);
    #undef  CASE
    default: break;
    }
    return "INVALID";
}
