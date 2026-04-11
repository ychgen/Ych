#ifndef YCH_KERNEL_KRTL_KRNLSTRING_H
#define YCH_KERNEL_KRTL_KRNLSTRING_H

#include "Core/Fundtypes.h"

#define KRTL_HEX_CASE_PREFERENCE_LOWER 0
#define KRTL_HEX_CASE_PREFERENCE_UPPER 1

#define KRTL_RADIX_OCTAL        8
#define KRTL_RADIX_DECIMAL     10
#define KRTL_RADIX_HEXADECIMAL 16

void KrtlPushHexCasePreference(int pref);

void KrtlReverseString(char* pStr, char* pStrEnd);
void KrtlIntegerToString(char* pDest, SQWORD value, int radix);
void KrtlUnsignedIntegerToString(char* pDest, QWORD value, int radix);

USIZE KrtlStringLength(const char* pString);

#endif // !YCH_KERNEL_KRTL_KRNLSTRING_H
