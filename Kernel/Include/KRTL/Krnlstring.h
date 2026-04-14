#ifndef YCH_KERNEL_KRTL_KRNLSTRING_H
#define YCH_KERNEL_KRTL_KRNLSTRING_H

#include "Krnlych.h"

#define KRTL_RADIX_OCTAL        8
#define KRTL_RADIX_DECIMAL     10
#define KRTL_RADIX_HEXADECIMAL 16

#define KRTL_HEX_LOWERCASE      0
#define KRTL_HEX_UPPERCASE      1

/**
 * @brief Reverses the given string, i.e. flips the order of characters.
 * 
 * @param pStr Start of the string to reverse.
 * @param pStr End of the string to reverse.
 */
VOID  KrtlReverseString(CHAR* pStr, CHAR* pStrEnd);

VOID  KrtlSignedToString  (CHAR* pDest, INTMAX  Value, INT Radix, INT HexCasePreference);
VOID  KrtlUnsignedToString(CHAR* pDest, UINTMAX Value, INT Radix, INT HexCasePreference);

SIZE  KrtlStringSize(CSTR pString);

#endif // !YCH_KERNEL_KRTL_KRNLSTRING_H
