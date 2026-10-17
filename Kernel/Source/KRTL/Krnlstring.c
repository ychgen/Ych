#include "KRTL/Krnlstring.h"

#include "KRTL/Krnlmem.h"

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

// TODO: This function can very well overshoot Limit because limit check is only done on format str, not format elements
// Fix this later it's an important issue
INT KrtlFormatStringVariadic(INT Limit, VOID* DstBuffer, CSTR Format, va_list Arguments)
{
    CHAR NullStrStr[] = "(NULL)";

    CHAR* OutHead = (CHAR*) DstBuffer;
    CHAR* OutTail = OutHead;

    BOOL HexOrBoolCase = FALSE; // false = locase, true = upcase

    while (*Format)
    {
        if (Limit != -1 && (INT)(OutTail - OutHead) >= Limit)
        {
            return (INT)(OutTail - OutHead);
        }

        if (*Format != '%')
        {
            *OutTail++ = *Format++;
            continue;
        }
        // specified `%` but string abruptly ended
        if (!*++Format)
        {
            return -1;
        }

        switch (*Format++)
        {
        case '%': // %%, print format specifier
        {
            *OutTail++ = *Format;
            break;
        }
        case 'c':
        {
            *OutTail++ = (CHAR) va_arg(Arguments, INT); // Take `INT` per integer promotion rules.
            break;
        }
        case 'B':
            HexOrBoolCase = TRUE;
            // falls through...
        case 'b':
        {
            CSTR StringOfChoice = NULLPTR;
            UINT SizeOfStringOfChoice = 0;
            if (va_arg(Arguments, UINT))
            {
                StringOfChoice = HexOrBoolCase ? "TRUE" : "true";
                SizeOfStringOfChoice = 4;
            }
            else
            {
                StringOfChoice = HexOrBoolCase ? "FALSE" : "false";
                SizeOfStringOfChoice = 5;
            }
            KrtlContiguousCopyBuffer(OutTail, StringOfChoice, SizeOfStringOfChoice);
            OutTail += SizeOfStringOfChoice;
            HexOrBoolCase = FALSE; // do not forget to reset
            break;
        }
        case 's':
        {
            CHAR* Str = va_arg(Arguments, CHAR*); // read ptr
            UINT Strlen = 0;
            if (Str)
            {
                Strlen = KrtlStringSize(Str);
            }
            else
            {
                Str = NullStrStr;
                Strlen = sizeof(NullStrStr);
            }

            KrtlContiguousCopyBuffer(OutTail, Str, Strlen);
            OutTail += Strlen;

            break;
        }
        case 'd':
        // falls through...
        case 'i':
        {
            INT Int = va_arg(Arguments, INT);
            
            CHAR Buf[12];
            KrtlSignedToString(Buf, Int, KRTL_RADIX_DECIMAL, 0);
            UINT Size = KrtlStringSize(Buf);

            KrtlContiguousCopyBuffer(OutTail, Buf, Size);
            OutTail += Size;
            
            break;
        }
        case 'u':
        {
            UINT UInt = va_arg(Arguments, UINT);

            CHAR Buf[11];
            KrtlUnsignedToString(Buf, UInt, KRTL_RADIX_DECIMAL, 0);
            UINT Size = KrtlStringSize(Buf);
            
            KrtlContiguousCopyBuffer(OutTail, Buf, Size);
            OutTail += Size;

            break;
        }
        case 'o':
        {
            UINT UInt = va_arg(Arguments, UINT);

            CHAR Buf[128];
            KrtlUnsignedToString(Buf, UInt, KRTL_RADIX_OCTAL, 0);
            UINT Size = KrtlStringSize(Buf);

            KrtlContiguousCopyBuffer(OutTail, Buf, Size);
            OutTail += Size;

            break;
        }
        case 'X':
            HexOrBoolCase = TRUE;
            // falls through...
        case 'x':
        {
            UINT UInt = va_arg(Arguments, UINT);

            CHAR Buf[128];
            KrtlUnsignedToString(Buf, UInt, KRTL_RADIX_HEXADECIMAL, HexOrBoolCase ? KRTL_HEX_UPPERCASE : KRTL_HEX_LOWERCASE);
            UINT Size = KrtlStringSize(Buf);

            KrtlContiguousCopyBuffer(OutTail, Buf, Size);
            OutTail += Size;

            HexOrBoolCase = FALSE;
            break;
        }
        case 'p':
        {
            VOID* Ptr = va_arg(Arguments, VOID*);

            CHAR Buf[128];
            KrtlUnsignedToString(Buf, (UINTMAX) Ptr, KRTL_RADIX_HEXADECIMAL, KRTL_HEX_UPPERCASE);
            UINT Size = KrtlStringSize(Buf);

            *OutTail++ = '0';
            *OutTail++ = 'x';

            KrtlContiguousCopyBuffer(OutTail, Buf, Size);
            OutTail += Size;

            break;
        }
        case 'R': // Extended (64-bit)
        {
            if (!*Format)
            {
                return -1;
            }

            switch (*Format++)
            {
            case 'u':
            {
                QWORD Quadword = va_arg(Arguments, QWORD);
                
                CHAR Buf[128];
                KrtlUnsignedToString(Buf, Quadword, KRTL_RADIX_DECIMAL, 0);
                UINT Size = KrtlStringSize(Buf);

                KrtlContiguousCopyBuffer(OutTail, Buf, Size);
                OutTail += Size;

                break;
            }
            case 'X':
                HexOrBoolCase = TRUE;
                // falls through...
            case 'x':
            {
                QWORD Quadword = va_arg(Arguments, QWORD);
                
                CHAR Buf[128];
                KrtlUnsignedToString(Buf, Quadword, KRTL_RADIX_HEXADECIMAL, HexOrBoolCase ? KRTL_HEX_UPPERCASE : KRTL_HEX_LOWERCASE);
                UINT Size = KrtlStringSize(Buf);

                KrtlContiguousCopyBuffer(OutTail, Buf, Size);
                OutTail += Size;

                HexOrBoolCase = FALSE;
                break;
            }
            default:
            {
                return -1;
            }
            }
            break;
        }
        default:
        {
            return -1;
        }
        }
    }

    return (INT)(OutTail - OutHead);
}

INT KrtlFormatString(INT Limit, VOID* DstBuffer, CSTR Format, ...)
{
    va_list Arguments;
    va_start(Arguments, Format);

    INT Result = KrtlFormatStringVariadic(Limit, DstBuffer, Format, Arguments);
    va_end(Arguments);

    return Result;
}
