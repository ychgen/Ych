#include "Earlyvideo/DisplaywideTextProtocol.h"

// BIG HACK. REMOVE LATER..
#include "Core/KernelState.h"

#include "KRTL/Krnlstring.h"
#include "KRTL/Krnlmem.h"

KrDisplaywideTextProtocolState g_ProtocolState;

KrDisplaywideTextProtocolFont KrdwtpScaleFont(const KrDisplaywideTextProtocolFont* pFont, BYTE ScaleFactor)
{
    KrDisplaywideTextProtocolFont Result = *pFont;
    Result.ScaleFactor *= ScaleFactor;
    return Result;
}

VOID KrdwtpInitialize(KrDisplaywideTextProtocolFont Font, UINTPTR AddrFrameBuffer, UINT FrameBufferSize, UINT FramebufferWidth, UINT FramebufferHeight, UINT PixelsPerScanLine)
{
    g_ProtocolState.FrameBufferSize   = FrameBufferSize;
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

VOID KrdwtpReloadFrameBuffer(UINTPTR AddrFrameBuffer)
{
    g_ProtocolState.AddrFrameBuffer = AddrFrameBuffer;
}

VOID KrdwtpResetState(DWORD ClearColor)
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

VOID KrdwtpScroll(VOID)
{
    // Cannot scroll while CursorY=0 i.e. still on 0th row.
    if (!g_ProtocolState.CursorY)
    {
        return;
    }

    // No bytes skip to get to the next physical row (1px down)
    UINT dwPhysStride = g_ProtocolState.PixelsPerScanLine * g_ProtocolState.BytesPerPixel;
    // Size that each entry occupies
    UINT dwEntrySize     = g_ProtocolState.Font.RowsPerEntry * g_ProtocolState.Font.ScaleFactor;
    // Logical row stride
    UINT dwLogicalStride = dwEntrySize * dwPhysStride;

    KrtlContiguousMoveBuffer
    (
        // 0th row
        (VOID*) g_ProtocolState.AddrFrameBuffer,
        // 1st row
        (VOID*)(g_ProtocolState.AddrFrameBuffer + dwLogicalStride),
        // no. rows
        (g_ProtocolState.CursorY) * dwLogicalStride
    );
    KrtlContiguousZeroBuffer((VOID*) g_ProtocolState.AddrFrameBuffer + ((g_ProtocolState.CursorY) * dwLogicalStride), dwLogicalStride);

    g_ProtocolState.CursorY--;
    g_ProtocolState.CursorX = 0;

    __asm__ __volatile__ ("sfence\n\t");
}

VOID KrdwtpOutColoredCharacter(CHAR Char, DWORD ForegroundColor, DWORD BackgroundColor)
{
    const KrDisplaywideTextProtocolFont* pFont = &g_ProtocolState.Font;
    if (!g_ProtocolState.AddrFrameBuffer || !g_ProtocolState.Font.CharacterSet || pFont->NumberOfEntries >= (UINT) Char)
    {
        return;
    }
    if (Char == '\n')
    {
        g_ProtocolState.CursorX = 0;
        if (++g_ProtocolState.CursorY > g_ProtocolState.FrameBufferHeight / (pFont->RowsPerEntry * pFont->ScaleFactor))
        {
            KrdwtpScroll();
        }
        return;
    }
    if (g_ProtocolState.bUppercaseMode && (Char >= 'a' && Char <= 'z'))
    {
        Char -= ('a' - 'A');
    }

    UINT dwBytesPerGlyph = (pFont->ColumnsPerEntry + 7) / 8;
    UINT dwGlyphSize = pFont->RowsPerEntry * dwBytesPerGlyph;
    BYTE* pGlyph = &pFont->CharacterSet[Char * dwGlyphSize];

    UINT dwStartX = g_ProtocolState.CursorX * pFont->ColumnsPerEntry * pFont->ScaleFactor;
    UINT dwStartY = g_ProtocolState.CursorY * pFont->RowsPerEntry    * pFont->ScaleFactor;

    for (DWORD Row = 0; Row < pFont->RowsPerEntry; Row++)
    {
        BYTE* pRow = &pGlyph[Row * dwBytesPerGlyph];

        for (BYTE bit = 0; bit < pFont->ColumnsPerEntry; bit++)
        {
            BYTE iBit = bit % 8;
            if (pFont->bDirectionBit)
            {
                iBit = 7 - bit;
            }
            BOOL pxlit = pRow[bit / 8] & (1 << iBit);

            for (DWORD ScalarY = 0; ScalarY < pFont->ScaleFactor; ScalarY++)
            {
                for (DWORD ScalarX = 0; ScalarX < pFont->ScaleFactor; ScalarX++)
                {
                    UINT FramebufferOffsetY = dwStartY + Row * pFont->ScaleFactor + ScalarY;
                    UINT FramebufferOffsetX = dwStartX + bit   * pFont->ScaleFactor + ScalarX;
                    ((DWORD*) g_ProtocolState.AddrFrameBuffer)[FramebufferOffsetY * g_ProtocolState.PixelsPerScanLine + FramebufferOffsetX]
                        = pxlit ? (ForegroundColor) : (g_KernelState.bMeltdown ? KRDWTP_COLOR_DARK_RED : BackgroundColor); // BIG HACK!
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

VOID KrdwtpOutCharacter(CHAR Char)
{
    KrdwtpOutColoredCharacter(Char, KRDWTP_COLOR_WHITE, KRDWTP_BACKGROUND);
}

INT KrdwtpOutColoredText(CSTR pText, DWORD ForegroundColor, DWORD BackgroundColor)
{
    INT nWritten = 0;
    while (*pText)
    {
        KrdwtpOutColoredCharacter(*pText++, ForegroundColor, BackgroundColor);
        nWritten++;
    }
    __asm__ __volatile__ ("sfence\n\t");
    return nWritten;
}

INT KrdwtpOutText(CSTR pText)
{
    return KrdwtpOutColoredText(pText, KRDWTP_COLOR_WHITE, KRDWTP_BACKGROUND);
}

INT KrdwtpOutFormatTextVariadic(CSTR pFmt, va_list args)
{
    const INT HexCasePreference = KRTL_HEX_UPPERCASE; // No matter %x or %X, we will use uppercase.
    INT nwritten = 0;

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
            KrtlSignedToString(buf, val, KRTL_RADIX_DECIMAL, HexCasePreference);
            nwritten += KrdwtpOutText(buf);
            break;
        }
        case 'u':
        {
            unsigned int val = va_arg(args, unsigned int);
            char buf[128];
            KrtlSignedToString(buf, val, KRTL_RADIX_DECIMAL, HexCasePreference);
            nwritten += KrdwtpOutText(buf);
            break;
        }
        case 'o':
        {
            unsigned int val = va_arg(args, unsigned int);
            char buf[128];
            KrtlSignedToString(buf, val, KRTL_RADIX_OCTAL, HexCasePreference);
            nwritten += KrdwtpOutText(buf);
            break;
        }
        case 'X':
        case 'x':
        {
            unsigned int val = va_arg(args, unsigned int);
            char buf[128];
            KrtlSignedToString(buf, val, KRTL_RADIX_HEXADECIMAL, HexCasePreference);
            nwritten += KrdwtpOutText(buf);
            break;
        }
        case 'p':
        {
            VOID* val = va_arg(args, VOID*);
            char buf[128];
            KrtlUnsignedToString(buf, (QWORD) val, KRTL_RADIX_HEXADECIMAL, HexCasePreference);
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
                KrtlUnsignedToString(buf, val, KRTL_RADIX_DECIMAL, HexCasePreference);
                nwritten += KrdwtpOutText(buf);
                break;
            }
            case 'X':
            case 'x':
            {
                QWORD val = va_arg(args, QWORD);
                char buf[128];
                KrtlUnsignedToString(buf, val, KRTL_RADIX_HEXADECIMAL, HexCasePreference);
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
    __asm__ __volatile__ ("sfence\n\t");
    return nwritten;
}

INT KrdwtpOutFormatText(CSTR pFmt, ...)
{
    va_list args;
    va_start(args, pFmt);
    INT nWritten = KrdwtpOutFormatTextVariadic(pFmt, args);
    va_end(args);

    return nWritten;
}

VOID KrdwtpSetUppercaseMode(BOOL bUppercaseModeEnabled)
{
    g_ProtocolState.bUppercaseMode = bUppercaseModeEnabled;
}

const KrDisplaywideTextProtocolState* KrdwtpGetProtocolState(VOID)
{
    return &g_ProtocolState;
}
