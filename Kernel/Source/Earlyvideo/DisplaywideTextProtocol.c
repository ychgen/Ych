#include "Earlyvideo/DisplaywideTextProtocol.h"

// BIG HACK. REMOVE LATER..
#include "Core/KernelState.h"

#include "CPU/Spinlock.h"

#include "Memory/BootstrapArena.h"

#include "KRTL/Krnlstring.h"
#include "KRTL/Krnlmem.h"

static KrDisplaywideTextProtocolState sg_ProtocolState;
static Spinlock sg_DwtpSpinlock = {0};

VOID DwtpReplicateRamFrbufToVram(VOID)
{
    KrtlContiguousCopyBuffer((VOID*) sg_ProtocolState.AddrFrameBuffer, (VOID*) sg_ProtocolState.AddrRamFrameBuffer, sg_ProtocolState.FrameBufferSize);
    __asm__ __volatile__ ("sfence\n\t" ::: "memory");
}

KrDisplaywideTextProtocolFont KrdwtpScaleFont(const KrDisplaywideTextProtocolFont* pFont, BYTE ScaleFactor)
{
    KrDisplaywideTextProtocolFont Result = *pFont;
    Result.ScaleFactor *= ScaleFactor;
    return Result;
}

VOID KrdwtpInitialize(KrDisplaywideTextProtocolFont Font, UINTPTR AddrFrameBuffer, UINT FrameBufferSize, UINT FramebufferWidth, UINT FramebufferHeight, UINT PixelsPerScanLine)
{
    AcquireOrWaitSpinlock(&sg_DwtpSpinlock);

    sg_ProtocolState.FrameBufferSize   = FrameBufferSize;
    sg_ProtocolState.FrameBufferWidth  = FramebufferWidth;
    sg_ProtocolState.FrameBufferHeight = FramebufferHeight;
    sg_ProtocolState.PixelsPerScanLine = PixelsPerScanLine;
    sg_ProtocolState.CursorX           = 0;
    sg_ProtocolState.CursorY           = 0;
    sg_ProtocolState.AddrFrameBuffer   = AddrFrameBuffer;
    sg_ProtocolState.AddrRamFrameBuffer = (UINTPTR) KrBootstrapArenaAcquire(sg_ProtocolState.FrameBufferSize * 2); // FIXME: Shady!
    sg_ProtocolState.BytesPerPixel     = 4; // We have 4 fields per pixel: red green blue reserved
    sg_ProtocolState.Font              = Font;
    sg_ProtocolState.bUppercaseMode    = FALSE;

    KrtlContiguousZeroBuffer((VOID*) sg_ProtocolState.AddrRamFrameBuffer, sg_ProtocolState.FrameBufferSize * 2);
    RelinquishSpinlock(&sg_DwtpSpinlock);
}

VOID KrdwtpReloadFrameBuffer(UINTPTR AddrFrameBuffer)
{
    AcquireOrWaitSpinlock(&sg_DwtpSpinlock);
    sg_ProtocolState.AddrFrameBuffer = AddrFrameBuffer;
    RelinquishSpinlock(&sg_DwtpSpinlock);
}

VOID KrdwtpResetState(DWORD ClearColor)
{
    AcquireOrWaitSpinlock(&sg_DwtpSpinlock);
    sg_ProtocolState.CursorX = 0;
    sg_ProtocolState.CursorY = 0;

    for (DWORD y = 0; y < sg_ProtocolState.FrameBufferHeight; y++)
    {
        for (DWORD x = 0; x < sg_ProtocolState.FrameBufferWidth; x++)
        {
            ((DWORD*) sg_ProtocolState.AddrRamFrameBuffer)[y * sg_ProtocolState.PixelsPerScanLine + x] = ClearColor;
            ((DWORD*) sg_ProtocolState.AddrFrameBuffer)[y * sg_ProtocolState.PixelsPerScanLine + x] = ClearColor;
        }
    }
    RelinquishSpinlock(&sg_DwtpSpinlock);
}

VOID KrdwtpScroll_NoSpin(VOID)
{
    // Cannot scroll while CursorY=0 i.e. still on 0th row.
    if (!sg_ProtocolState.CursorY)
    {
        return;
    }

    // No bytes skip to get to the next physical row (1px down)
    UINT dwPhysStride = sg_ProtocolState.PixelsPerScanLine * sg_ProtocolState.BytesPerPixel;
    // Size that each entry occupies
    UINT dwEntrySize     = sg_ProtocolState.Font.RowsPerEntry * sg_ProtocolState.Font.ScaleFactor;
    // Logical row stride
    UINT dwLogicalStride = dwEntrySize * dwPhysStride;

    KrtlContiguousMoveBuffer
    (
        // 0th row
        (VOID*) sg_ProtocolState.AddrRamFrameBuffer,
        // 1st row
        (VOID*)(sg_ProtocolState.AddrRamFrameBuffer + dwLogicalStride),
        // no. rows
        (sg_ProtocolState.CursorY) * dwLogicalStride
    );
    KrtlContiguousZeroBuffer((VOID*) sg_ProtocolState.AddrRamFrameBuffer + ((sg_ProtocolState.CursorY) * dwLogicalStride), dwLogicalStride);

    DwtpReplicateRamFrbufToVram();

    sg_ProtocolState.CursorY--;
    sg_ProtocolState.CursorX = 0;
}

VOID KrdwtpScroll(VOID)
{
    AcquireOrWaitSpinlock(&sg_DwtpSpinlock);
    KrdwtpScroll_NoSpin();
    RelinquishSpinlock(&sg_DwtpSpinlock);
}

VOID KrdwtpOutColoredCharacter_NoSpin(CHAR Char, DWORD ForegroundColor, DWORD BackgroundColor)
{
    const KrDisplaywideTextProtocolFont* pFont = &sg_ProtocolState.Font;
    if (!sg_ProtocolState.AddrFrameBuffer || !sg_ProtocolState.Font.CharacterSet || pFont->NumberOfEntries >= (UINT) Char)
    {
        return;
    }
    if (Char == '\n')
    {
        sg_ProtocolState.CursorX = 0;
        if (++sg_ProtocolState.CursorY > sg_ProtocolState.FrameBufferHeight / (pFont->RowsPerEntry * pFont->ScaleFactor))
        {
            KrdwtpScroll_NoSpin();
        }
        return;
    }
    if (sg_ProtocolState.bUppercaseMode && (Char >= 'a' && Char <= 'z'))
    {
        Char -= ('a' - 'A');
    }

    UINT dwBytesPerGlyph = (pFont->ColumnsPerEntry + 7) / 8;
    UINT dwGlyphSize = pFont->RowsPerEntry * dwBytesPerGlyph;
    BYTE* pGlyph = &pFont->CharacterSet[Char * dwGlyphSize];

    UINT dwStartX = sg_ProtocolState.CursorX * pFont->ColumnsPerEntry * pFont->ScaleFactor;
    UINT dwStartY = sg_ProtocolState.CursorY * pFont->RowsPerEntry    * pFont->ScaleFactor;

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
                    ((DWORD*) sg_ProtocolState.AddrFrameBuffer)[FramebufferOffsetY * sg_ProtocolState.PixelsPerScanLine + FramebufferOffsetX]
                        = pxlit ? (ForegroundColor) : (g_KernelState.bMeltdown ? KRDWTP_COLOR_DARK_PURPLE : BackgroundColor); // BIG HACK!
                    ((DWORD*) sg_ProtocolState.AddrRamFrameBuffer)[FramebufferOffsetY * sg_ProtocolState.PixelsPerScanLine + FramebufferOffsetX]
                        = pxlit ? (ForegroundColor) : (g_KernelState.bMeltdown ? KRDWTP_COLOR_DARK_PURPLE : BackgroundColor); // BIG HACK!
                }
            }
        }
    }

    sg_ProtocolState.CursorX++;
    if (sg_ProtocolState.CursorX >= sg_ProtocolState.FrameBufferWidth / (pFont->ColumnsPerEntry * pFont->ScaleFactor))
    {
        sg_ProtocolState.CursorX = 0;
        if (++sg_ProtocolState.CursorY > sg_ProtocolState.FrameBufferHeight / (pFont->RowsPerEntry * pFont->ScaleFactor))
        {
            KrdwtpScroll();
        }
    }
}

VOID KrdwtpOutColoredCharacter(CHAR Char, DWORD ForegroundColor, DWORD BackgroundColor)
{
    AcquireOrWaitSpinlock(&sg_DwtpSpinlock);
    KrdwtpOutColoredCharacter_NoSpin(Char, ForegroundColor, BackgroundColor);
    RelinquishSpinlock(&sg_DwtpSpinlock);
}

VOID KrdwtpOutCharacter_NoSpin(CHAR Char)
{
    KrdwtpOutColoredCharacter_NoSpin(Char, KRDWTP_COLOR_WHITE, KRDWTP_BACKGROUND);
}

VOID KrdwtpOutCharacter(CHAR Char)
{
    KrdwtpOutColoredCharacter(Char, KRDWTP_COLOR_WHITE, KRDWTP_BACKGROUND);
}

INT KrdwtpOutColoredText_NoSpin(CSTR pText, DWORD ForegroundColor, DWORD BackgroundColor)
{
    INT nWritten = 0;
    while (*pText)
    {
        KrdwtpOutColoredCharacter_NoSpin(*pText++, ForegroundColor, BackgroundColor);
        nWritten++;
    }
    __asm__ __volatile__ ("sfence\n\t");
    return nWritten;
}

INT KrdwtpOutColoredText(CSTR pText, DWORD ForegroundColor, DWORD BackgroundColor)
{
    AcquireOrWaitSpinlock(&sg_DwtpSpinlock);
    INT Result = KrdwtpOutColoredText_NoSpin(pText, ForegroundColor, BackgroundColor);
    RelinquishSpinlock(&sg_DwtpSpinlock);

    return Result;
}

INT KrdwtpOutText_NoSpin(CSTR pText)
{
    return KrdwtpOutColoredText_NoSpin(pText, KRDWTP_COLOR_WHITE, KRDWTP_BACKGROUND);
}

INT KrdwtpOutText(CSTR pText)
{
    return KrdwtpOutColoredText(pText, KRDWTP_COLOR_WHITE, KRDWTP_BACKGROUND);
}

INT KrdwtpOutFormatTextVariadic(CSTR pFmt, va_list args)
{
    AcquireOrWaitSpinlock(&sg_DwtpSpinlock);
    
    const INT HexCasePreference = KRTL_HEX_UPPERCASE; // No matter %x or %X, we will use uppercase.
    INT nwritten = 0;

    while (*pFmt)
    {
        if (*pFmt != '%')
        {
            KrdwtpOutCharacter_NoSpin(*pFmt++);
            nwritten++;
            continue;
        }
        // specified `%` but string abruptly ended
        if (!*++pFmt)
        {
            goto ErrorOut;
        }

        switch (*pFmt++)
        {
        case '%': // %%, print format specifier
        {
            KrdwtpOutCharacter_NoSpin('%');
            nwritten++;
            break;
        }
        case 'c':
        {
            int ch = va_arg(args, int);
            KrdwtpOutCharacter_NoSpin((char) ch);
            nwritten++;
            break;
        }
        case 'b':
        {
            int val = va_arg(args, int);
            nwritten += KrdwtpOutText_NoSpin(val ? "true" : "false");
            break;
        }
        case 's':
        {
            char* str = va_arg(args, char*);
            if (!str)
            {
                str = "(NULL)";
            }
            nwritten += KrdwtpOutText_NoSpin(str);
            break;
        }
        case 'd':
        // falls through...
        case 'i':
        {
            int val = va_arg(args, int);
            char buf[128];
            KrtlSignedToString(buf, val, KRTL_RADIX_DECIMAL, HexCasePreference);
            nwritten += KrdwtpOutText_NoSpin(buf);
            break;
        }
        case 'u':
        {
            unsigned int val = va_arg(args, unsigned int);
            char buf[128];
            KrtlUnsignedToString(buf, val, KRTL_RADIX_DECIMAL, HexCasePreference);
            nwritten += KrdwtpOutText_NoSpin(buf);
            break;
        }
        case 'o':
        {
            unsigned int val = va_arg(args, unsigned int);
            char buf[128];
            KrtlSignedToString(buf, val, KRTL_RADIX_OCTAL, HexCasePreference);
            nwritten += KrdwtpOutText_NoSpin(buf);
            break;
        }
        case 'X':
        case 'x':
        {
            unsigned int val = va_arg(args, unsigned int);
            char buf[128];
            KrtlUnsignedToString(buf, val, KRTL_RADIX_HEXADECIMAL, HexCasePreference);
            nwritten += KrdwtpOutText_NoSpin(buf);
            break;
        }
        case 'p':
        {
            VOID* val = va_arg(args, VOID*);
            char buf[128];
            KrtlUnsignedToString(buf, (QWORD) val, KRTL_RADIX_HEXADECIMAL, HexCasePreference);
            nwritten += KrdwtpOutText_NoSpin("0x");
            nwritten += KrdwtpOutText_NoSpin(buf);
            break;
        }
        case 'R': // Extended (64-bit)
        {
            if (!*pFmt)
            {
                goto ErrorOut;
            }

            switch (*pFmt++)
            {
            case 'u':
            {
                QWORD val = va_arg(args, QWORD);
                char buf[128];
                KrtlUnsignedToString(buf, val, KRTL_RADIX_DECIMAL, HexCasePreference);
                nwritten += KrdwtpOutText_NoSpin(buf);
                break;
            }
            case 'X':
            case 'x':
            {
                QWORD val = va_arg(args, QWORD);
                char buf[128];
                KrtlUnsignedToString(buf, val, KRTL_RADIX_HEXADECIMAL, HexCasePreference);
                nwritten += KrdwtpOutText_NoSpin(buf);
                break;
            }
            default:
            {
                goto ErrorOut;
            }
            }
            break;
        }
        default:
        {
            goto ErrorOut;
        }
        }
    }
    __asm__ __volatile__ ("sfence\n\t");
    RelinquishSpinlock(&sg_DwtpSpinlock);
    return nwritten;
ErrorOut:
    RelinquishSpinlock(&sg_DwtpSpinlock);
    return -1;
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
    AcquireOrWaitSpinlock(&sg_DwtpSpinlock);
    sg_ProtocolState.bUppercaseMode = bUppercaseModeEnabled;
    RelinquishSpinlock(&sg_DwtpSpinlock);
}

const KrDisplaywideTextProtocolState* KrdwtpGetProtocolState(VOID)
{
    return &sg_ProtocolState;
}
