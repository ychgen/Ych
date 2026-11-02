#ifndef YCH_KERNEL_KRTL_KRNL_CHAR_H
#define YCH_KERNEL_KRTL_KRNL_CHAR_H

#include "Krnlych.h"

BOOL KrtlIsCharDigit(CHAR Ch); // Decimal digits!
BOOL KrtlIsCharUpper(CHAR Ch);
BOOL KrtlIsCharLower(CHAR Ch);
BOOL KrtlIsCharAlpha(CHAR Ch);
BOOL KrtlIsCharPunct(CHAR Ch);
BOOL KrtlIsCharPrint(CHAR Ch);

#endif // !YCH_KERNEL_KRTL_KRNL_CHAR_H
