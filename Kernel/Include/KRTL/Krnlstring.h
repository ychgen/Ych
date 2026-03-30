#ifndef YCH_KERNEL_KRTL_KRNLSTRING_H
#define YCH_KERNEL_KRTL_KRNLSTRING_H

#include <stdint.h>

#define KRTL_CASE_PREFERENCE_LOWER 0
#define KRTL_CASE_PREFERENCE_UPPER 1

#define KRTL_RADIX_OCTAL        8
#define KRTL_RADIX_DECIMAL     10
#define KRTL_RADIX_HEXADECIMAL 16

void KrtlPushHexCasePreference(int pref);

void KrtlReverseString(char* pStr, char* pStrEnd);
void KrtlIntegerToString(char* pDest, int64_t value, int radix);
void KrtlUnsignedIntegerToString(char* pDest, uint64_t value, int radix);

#endif // !YCH_KERNEL_KRTL_KRNLSTRING_H
