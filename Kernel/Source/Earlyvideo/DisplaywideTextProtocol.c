#include "Earlyvideo/DisplaywideTextProtocol.h"

#include "KRTL/Krnlstring.h"

// TODO: Add scrolling!

KrDisplaywideTextProtocolState g_ProtocolState;

void KrdwtpInitialize(KrDisplaywideTextProtocolFont font, uintptr_t addrFrameBuffer, uint32_t xszFrameBuffer, uint32_t yszFrameBuffer, uint32_t rowStride)
{
    g_ProtocolState.FrameBufferWidth  = xszFrameBuffer;
    g_ProtocolState.FrameBufferHeight = yszFrameBuffer;
    g_ProtocolState.RowStride         = rowStride;
    g_ProtocolState.CursorX           = 0;
    g_ProtocolState.CursorY           = 0;
    g_ProtocolState.AddrFrameBuffer   = addrFrameBuffer;
    g_ProtocolState.Font              = font;
    g_ProtocolState.bUppercaseMode    = false;
}

void KrdwtpReloadFrameBuffer(uintptr_t addrFrameBuffer)
{
    g_ProtocolState.AddrFrameBuffer = addrFrameBuffer;
}

void KrdwtpResetState(uint32_t fbcolor)
{
    g_ProtocolState.CursorX = 0;
    g_ProtocolState.CursorY = 0;

    for (uint32_t y = 0; y < g_ProtocolState.FrameBufferHeight; y++)
    {
        for (uint32_t x = 0; x < g_ProtocolState.FrameBufferWidth; x++)
        {
            ((uint32_t*) g_ProtocolState.AddrFrameBuffer)[y * g_ProtocolState.RowStride + x] = fbcolor;
        }
    }
}

void KrdwtpOutColoredCharacter(char character, uint32_t color)
{
    if (!g_ProtocolState.AddrFrameBuffer || !g_ProtocolState.Font.CharacterSet)
    {
        return;
    }
    if (character == '\n')
    {
        g_ProtocolState.CursorX = 0;
        g_ProtocolState.CursorY++;
        return;
    }
    if (g_ProtocolState.bUppercaseMode && (character >= 'a' && character <= 'z'))
    {
        character -= ('a' - 'A');
    }
    KrDisplaywideTextProtocolFontCharacter* pCharFontBitmap = &g_ProtocolState.Font.CharacterSet[(uint8_t) character];
    uint8_t scale = g_ProtocolState.Font.ScaleFactor;
    uint32_t stx = g_ProtocolState.CursorX * 8 * scale;
    uint32_t sty = g_ProtocolState.CursorY * KR_DISPLAYWIDE_TEXT_PROTOCOL_FONT_CHAR_ROW_COUNT * scale;

    for (uint32_t row = 0; row < KR_DISPLAYWIDE_TEXT_PROTOCOL_FONT_CHAR_ROW_COUNT; row++)
    {
        uint8_t rowBits = (*pCharFontBitmap)[row];
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            bool pxlit = (rowBits & (1 << bit));
            if (!pxlit)
            {
                continue;
            }

            for (uint32_t dy = 0; dy < scale; dy++)
            {
                for (uint32_t dx = 0; dx < scale; dx++)
                {
                    uint32_t xFB = stx + bit * scale + dx;
                    uint32_t yFB = sty + row * scale + dy;
                    ((uint32_t*) g_ProtocolState.AddrFrameBuffer)[yFB * g_ProtocolState.RowStride + xFB] = color;
                }
            }
        }
    }

    g_ProtocolState.CursorX++;
    if (g_ProtocolState.CursorX >= g_ProtocolState.FrameBufferWidth / (8 * scale))
    {
        g_ProtocolState.CursorX = 0;
        g_ProtocolState.CursorY++;
    }
}

void KrdwtpOutCharacter(char character)
{
    KrdwtpOutColoredCharacter(character, KRDWTP_COLOR_WHITE);
}

int KrdwtpOutColoredText(const char* pText, uint32_t color)
{
    int nwritten = 0;
    while (*pText)
    {
        KrdwtpOutColoredCharacter(*pText++, color);
        nwritten++;
    }
    return nwritten;
}

int KrdwtpOutText(const char* pText)
{
    return KrdwtpOutColoredText(pText, KRDWTP_COLOR_WHITE);
}

int KrdwtpOutFormatTextVariadic(const char* pFmt, va_list args)
{
    int nwritten = 0;
    while (*pFmt)
    {
        if (*pFmt != '%')
        {
            KrdwtpOutCharacter(*pFmt++);
            nwritten++;
            continue;
        }
        // specified `%` but string abruptly ended
        if (!*++pFmt)
        {
            return -1;
        }

        switch (*pFmt++)
        {
        case '%': // %%, print format specifier
        {
            KrdwtpOutCharacter('%');
            nwritten++;
            break;
        }
        case 'c':
        {
            int ch = va_arg(args, int);
            KrdwtpOutCharacter((char) ch);
            nwritten++;
            break;
        }
        case 's':
        {
            char* str = va_arg(args, char*);
            if (!str)
            {
                str = "(NULL)";
            }
            nwritten += KrdwtpOutText(str);
            break;
        }
        case 'd':
        // falls through...
        case 'i':
        {
            int val = va_arg(args, int);
            char buf[128];
            KrtlIntegerToString(buf, val, KRTL_RADIX_DECIMAL);
            nwritten += KrdwtpOutText(buf);
            break;
        }
        case 'u':
        {
            unsigned int val = va_arg(args, unsigned int);
            char buf[128];
            KrtlIntegerToString(buf, val, KRTL_RADIX_DECIMAL);
            nwritten += KrdwtpOutText(buf);
            break;
        }
        case 'o':
        {
            unsigned int val = va_arg(args, unsigned int);
            char buf[128];
            KrtlIntegerToString(buf, val, KRTL_RADIX_OCTAL);
            nwritten += KrdwtpOutText(buf);
            break;
        }
        case 'X':
        {
            KrtlPushHexCasePreference(KRTL_CASE_PREFERENCE_UPPER);
        }
        // falls through...
        case 'x':
        {
            unsigned int val = va_arg(args, unsigned int);
            char buf[128];
            KrtlIntegerToString(buf, val, KRTL_RADIX_HEXADECIMAL);
            nwritten += KrdwtpOutText(buf);
            break;
        }
        case 'p':
        {
            void* val = va_arg(args, void*);
            char buf[128];
            KrtlPushHexCasePreference(KRTL_CASE_PREFERENCE_UPPER);
            KrtlUnsignedIntegerToString(buf, (uint64_t) val, KRTL_RADIX_HEXADECIMAL);
            nwritten += KrdwtpOutText("0x");
            nwritten += KrdwtpOutText(buf);
            break;
        }
        case 'R': // Extended (64-bit)
        {
            if (!*pFmt)
            {
                return -1;
            }

            switch (*pFmt++)
            {
            case 'u':
            {
                uint64_t val = va_arg(args, uint64_t);
                char buf[128];
                KrtlUnsignedIntegerToString(buf, val, KRTL_RADIX_DECIMAL);
                nwritten += KrdwtpOutText(buf);
                break;
            }
            case 'X':
            {
                KrtlPushHexCasePreference(KRTL_CASE_PREFERENCE_UPPER);
            }
            // falls through...
            case 'x':
            {
                uint64_t val = va_arg(args, uint64_t);
                char buf[128];
                KrtlUnsignedIntegerToString(buf, val, KRTL_RADIX_HEXADECIMAL);
                nwritten += KrdwtpOutText(buf);
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
    return nwritten;
}

int KrdwtpOutFormatText(const char* pFmt, ...)
{
    va_list args;
    va_start(args, pFmt);
    int result = KrdwtpOutFormatTextVariadic(pFmt, args);
    va_end(args);

    return result;
}

void KrdwtpSetUppercaseMode(bool bUppercaseModeEnabled)
{
    g_ProtocolState.bUppercaseMode = bUppercaseModeEnabled;
}

const KrDisplaywideTextProtocolState* KrdwtpGetProtocolState(void)
{
    return &g_ProtocolState;
}
