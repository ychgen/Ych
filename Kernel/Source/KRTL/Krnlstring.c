#include "KRTL/Krnlstring.h"

VOID KrtlReverseString(CHAR* pStr, CHAR* pStrEnd)
{
    CHAR Temp;
    while (pStr < pStrEnd)
    {
         Temp      = *pStr;
        *pStr++    = *pStrEnd;
        *pStrEnd-- = Temp;
    }
}

inline CHAR KrtlDigitChar(UINTMAX r, INT HexCasePreference)
{
    return r < KRTL_RADIX_DECIMAL ? r + '0' : (r - KRTL_RADIX_DECIMAL) + (HexCasePreference == KRTL_HEX_UPPERCASE ? 'A' : 'a');
}

VOID KrtlSignedToString(CHAR* pDest, INTMAX Value, INT Radix, INT HexCasePreference)
{
    CHAR* pDestInitial = pDest;

    if (!Value)
    {
        *pDest++ = '0';
        *pDest   =  0; // Null terminate
        return;
    }

    BOOL bHasSign = FALSE;
    if (Value < 0)
    {
        bHasSign =  TRUE;
        Value    = -Value; // Make positive
    }

    while (Value)
    {
        *pDest++ = KrtlDigitChar((QWORD)(Value % Radix), HexCasePreference);
        Value /= Radix;
    }
    if (bHasSign)
    {
        *pDest++ = '-';
    }
    *pDest = 0; // Null terminate
    KrtlReverseString(pDestInitial, pDest - 1);
}
VOID KrtlUnsignedToString(CHAR* pDest, UINTMAX Value, INT Radix, INT HexCasePreference)
{
    CHAR* pDestInitial = pDest;

    if (Value == 0)
    {
        *pDest++ = '0';
        *pDest   =  0; // Null terminate
        return;
    }

    while (Value)
    {
        *pDest++ = KrtlDigitChar(Value % Radix, HexCasePreference);
        Value /= Radix;
    }
    *pDest = 0; // Null terminate
    KrtlReverseString(pDestInitial, pDest - 1);
}

SIZE KrtlStringSize(CSTR pString)
{
    const CHAR* pIt = pString;
    while (*++pIt);
    return (SIZE)(pIt - pString);
}

