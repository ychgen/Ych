#include "KRTL/Krnlstring.h"

#define KRTL_HEX_CASE_PREFERENCE_DEFAULT KRTL_HEX_CASE_PREFERENCE_LOWER
int g_KRTLHexCasePreference = KRTL_HEX_CASE_PREFERENCE_DEFAULT;

void KrtlPushHexCasePreference(int pref)
{
    g_KRTLHexCasePreference = pref;
}

void KrtlReverseString(char* pStr, char* pStrEnd)
{
    char tmp;
    while (pStr < pStrEnd)
    {
        tmp = *pStr;
        *pStr++ = *pStrEnd;
        *pStrEnd-- = tmp;
    }
}

char KrtlIntegerToString_Chval(QWORD r);
void KrtlIntegerToString(char* pDest, SQWORD value, int radix)
{
    char* pDestInitial = pDest;
    if (value == 0)
    {
        *pDest++ = '0';
        *pDest   =  0; // null terminate
        return;
    }

    int sign = 0;
    if (value < 0)
    {
        sign = 1;
        value = -value;
    }

    while (value)
    {
        *pDest++ = KrtlIntegerToString_Chval((QWORD)(value % radix));
        value /= radix;
    }
    if (sign)
    {
        *pDest++ = '-';
    }
    *pDest = 0; // null terminate
    KrtlReverseString(pDestInitial, pDest - 1);

    // pop preference
    if (radix == KRTL_RADIX_HEXADECIMAL)
    {
        g_KRTLHexCasePreference = KRTL_HEX_CASE_PREFERENCE_DEFAULT;
    }
}
void KrtlUnsignedIntegerToString(char* pDest, QWORD value, int radix)
{
    char* pDestInitial = pDest;
    if (value == 0)
    {
        *pDest++ = '0';
        *pDest   =  0; // null terminate
        return;
    }

    while (value)
    {
        *pDest++ = KrtlIntegerToString_Chval(value % radix);
        value /= radix;
    }
    *pDest = 0; // null terminate
    KrtlReverseString(pDestInitial, pDest - 1);

    // pop preference
    if (radix == KRTL_RADIX_HEXADECIMAL)
    {
        g_KRTLHexCasePreference = KRTL_HEX_CASE_PREFERENCE_DEFAULT;
    }
}

char KrtlIntegerToString_Chval(QWORD r)
{
    return r < KRTL_RADIX_DECIMAL ? r + '0' : (r-KRTL_RADIX_DECIMAL) + (g_KRTLHexCasePreference == KRTL_HEX_CASE_PREFERENCE_UPPER ? 'A' : 'a');
}

USIZE KrtlStringLength(const char* pString)
{
    const char* pIt = pString;
    while (*++pIt);
    return (USIZE)(pIt - pString);
}

