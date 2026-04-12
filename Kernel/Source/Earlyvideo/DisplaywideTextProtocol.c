#include "Earlyvideo/DisplaywideTextProtocol.h"

#include "KRTL/Krnlstring.h"
#include "KRTL/Krnlmem.h"

KrDisplaywideTextProtocolState g_ProtocolState;

KrDisplaywideTextProtocolFont KrdwtpScaleFont(const KrDisplaywideTextProtocolFont* pFont, BYTE factor)
{
    KrDisplaywideTextProtocolFont result = *pFont;
    result.ScaleFactor *= factor;
    return result;
}

void KrdwtpInitialize(KrDisplaywideTextProtocolFont Font, UINTPTR AddrFrameBuffer, DWORD FramebufferWidth, DWORD FramebufferHeight, DWORD PixelsPerScanLine)
{
    g_ProtocolState.bIsActive         = TRUE;
    g_ProtocolState.FrameBufferWidth  = FramebufferWidth;
    g_ProtocolState.FrameBufferHeight = FramebufferHeight;
    g_ProtocolState.PixelsPerScanLine = PixelsPerScanLine;
    g_ProtocolState.CursorX           = 0;
    g_ProtocolState.CursorY           = 0;
    g_ProtocolState.AddrFrameBuffer   = AddrFrameBuffer;
    g_ProtocolState.BytesPerPixel     = 4; // We have 4 fields per pixel: red green blue reserved
    g_ProtocolState.Font              = Font;
    g_ProtocolState.bUppercaseMode    = FALSE;
}

void KrdwtpReloadFrameBuffer(UINTPTR AddrFrameBuffer)
{
    g_ProtocolState.AddrFrameBuffer = AddrFrameBuffer;
}

void KrdwtpResetState(DWORD ClearColor)
{
    g_ProtocolState.CursorX = 0;
    g_ProtocolState.CursorY = 0;

    for (DWORD y = 0; y < g_ProtocolState.FrameBufferHeight; y++)
    {
        for (DWORD x = 0; x < g_ProtocolState.FrameBufferWidth; x++)
        {
            ((DWORD*) g_ProtocolState.AddrFrameBuffer)[y * g_ProtocolState.PixelsPerScanLine + x] = ClearColor;
        }
    }
}

void KrdwtpScroll(void)
{
    // Cannot scroll while CursorY=0 i.e. still on 0th row.
    if (!g_ProtocolState.CursorY)
    {
        return;
    }

    // No bytes skip to get to the next physical row (1px down)
    DWORD dwPhysStride = g_ProtocolState.PixelsPerScanLine * g_ProtocolState.BytesPerPixel;
    // Size that each entry occupies
    DWORD dwEntrySize     = g_ProtocolState.Font.RowsPerEntry * g_ProtocolState.Font.ScaleFactor;
    // Logical row stride
    DWORD dwLogicalStride = dwEntrySize * dwPhysStride;

    KrtlContiguousMoveBuffer
    (
        // 0th row
        (void*) g_ProtocolState.AddrFrameBuffer,
        // 1st row
        (void*)(g_ProtocolState.AddrFrameBuffer + dwLogicalStride),
        // no. rows
        (g_ProtocolState.CursorY) * dwLogicalStride
    );
    KrtlContiguousZeroBuffer((void*) g_ProtocolState.AddrFrameBuffer + ((g_ProtocolState.CursorY) * dwLogicalStride), dwLogicalStride);

    g_ProtocolState.CursorY--;
    g_ProtocolState.CursorX = 0;
}

void KrdwtpOutColoredCharacter(char Char, DWORD ForegroundColor, DWORD BackgroundColor)
{
    if (!g_ProtocolState.AddrFrameBuffer || !g_ProtocolState.Font.CharacterSet || Char >= 128)
    {
        return;
    }
    if (Char == '\n')
    {
        g_ProtocolState.CursorX = 0;
        if (++g_ProtocolState.CursorY > g_ProtocolState.FrameBufferHeight / (g_ProtocolState.Font.RowsPerEntry * g_ProtocolState.Font.ScaleFactor))
        {
            KrdwtpScroll();
        }
        return;
    }
    if (g_ProtocolState.bUppercaseMode && (Char >= 'a' && Char <= 'z'))
    {
        Char -= ('a' - 'A');
    }

    const KrDisplaywideTextProtocolFont* pFont = &g_ProtocolState.Font;
    DWORD dwBytesPerGlyph = (pFont->ColumnsPerEntry + 7) / 8;
    DWORD dwGlyphSize = pFont->RowsPerEntry * dwBytesPerGlyph;
    BYTE* pGlyph = &pFont->CharacterSet[Char * dwGlyphSize];

    DWORD dwStartX = g_ProtocolState.CursorX * pFont->ColumnsPerEntry * pFont->ScaleFactor;
    DWORD dwStartY = g_ProtocolState.CursorY * pFont->RowsPerEntry    * pFont->ScaleFactor;

    for (DWORD dwRow = 0; dwRow < pFont->RowsPerEntry; dwRow++)
    {
        BYTE* pRow = &pGlyph[dwRow * dwBytesPerGlyph];

        for (BYTE bit = 0; bit < pFont->ColumnsPerEntry; bit++)
        {
            BYTE iBit = bit % 8;
            if (pFont->bDirectionBit)
            {
                iBit = 7 - bit;
            }
            BOOL pxlit = pRow[bit / 8] & (1 << iBit);

            for (DWORD dwScalarY = 0; dwScalarY < pFont->ScaleFactor; dwScalarY++)
            {
                for (DWORD dwScalarX = 0; dwScalarX < pFont->ScaleFactor; dwScalarX++)
                {
                    DWORD dwFramebufferOffsetY = dwStartY + dwRow * pFont->ScaleFactor + dwScalarY;
                    DWORD dwFramebufferOffsetX = dwStartX + bit   * pFont->ScaleFactor + dwScalarX;
                    ((DWORD*) g_ProtocolState.AddrFrameBuffer)[dwFramebufferOffsetY * g_ProtocolState.PixelsPerScanLine + dwFramebufferOffsetX]
                        = pxlit ? (ForegroundColor) : BackgroundColor;
                }
            }
        }
    }

    g_ProtocolState.CursorX++;
    if (g_ProtocolState.CursorX >= g_ProtocolState.FrameBufferWidth / (pFont->ColumnsPerEntry * pFont->ScaleFactor))
    {
        g_ProtocolState.CursorX = 0;
        if (++g_ProtocolState.CursorY > g_ProtocolState.FrameBufferHeight / (pFont->RowsPerEntry * pFont->ScaleFactor))
        {
            KrdwtpScroll();
        }
    }
}

void KrdwtpOutCharacter(char Char)
{
    KrdwtpOutColoredCharacter(Char, KRDWTP_COLOR_WHITE, KRDWTP_FOREGROUND);
}

int KrdwtpOutColoredText(const char* pText, DWORD ForegroundColor, DWORD BackgroundColor)
{
    int nWritten = 0;
    while (*pText)
    {
        KrdwtpOutColoredCharacter(*pText++, ForegroundColor, BackgroundColor);
        nWritten++;
    }
    return nWritten;
}

int KrdwtpOutText(const char* pText)
{
    return KrdwtpOutColoredText(pText, KRDWTP_COLOR_WHITE, KRDWTP_FOREGROUND);
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
            KrtlPushHexCasePreference(KRTL_HEX_CASE_PREFERENCE_UPPER);
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
            KrtlPushHexCasePreference(KRTL_HEX_CASE_PREFERENCE_UPPER);
            KrtlUnsignedIntegerToString(buf, (QWORD) val, KRTL_RADIX_HEXADECIMAL);
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
                QWORD val = va_arg(args, QWORD);
                char buf[128];
                KrtlUnsignedIntegerToString(buf, val, KRTL_RADIX_DECIMAL);
                nwritten += KrdwtpOutText(buf);
                break;
            }
            case 'X':
            {
                KrtlPushHexCasePreference(KRTL_HEX_CASE_PREFERENCE_UPPER);
            }
            // falls through...
            case 'x':
            {
                QWORD val = va_arg(args, QWORD);
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

void KrdwtpSetUppercaseMode(BOOL bUppercaseModeEnabled)
{
    g_ProtocolState.bUppercaseMode = bUppercaseModeEnabled;
}

const KrDisplaywideTextProtocolState* KrdwtpGetProtocolState(void)
{
    return &g_ProtocolState;
}
