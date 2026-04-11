#ifndef YCH_KERNEL_EARLYVIDEO_DISPLAYWIDE_TEXT_PROTOCOL
#define YCH_KERNEL_EARLYVIDEO_DISPLAYWIDE_TEXT_PROTOCOL

#include "Core/Fundtypes.h"
#include <stdarg.h>

#define KR_DISPLAYWIDE_TEXT_PROTOCOL_FONT_CHARSET_SIZE   128

#define KRDWTP_MAKE_COLOR(r,g,b,a) ( ((((DWORD)a) & 0xFF) << 24) | ((((DWORD)r) & 0xFF) << 16) | ((((DWORD)g) & 0xFF) << 8) | ((((DWORD)b) & 0xFF)))
// Default foreground color
#define KRDWTP_FOREGROUND KRDWTP_MAKE_COLOR(0x00,0x00,0x00,0xFF)

#define KRDWTP_COLOR_BLACK  KRDWTP_MAKE_COLOR(0x00,0x00,0x00,0xFF)
#define KRDWTP_COLOR_WHITE  KRDWTP_MAKE_COLOR(0xFF,0xFF,0xFF,0xFF)
#define KRDWTP_COLOR_RED    KRDWTP_MAKE_COLOR(0xFF,0x00,0x00,0xFF)
#define KRDWTP_COLOR_GREEN  KRDWTP_MAKE_COLOR(0x00,0xFF,0x00,0xFF)
#define KRDWTP_COLOR_BLUE   KRDWTP_MAKE_COLOR(0x00,0x00,0xFF,0xFF)
#define KRDWTP_COLOR_YELLOW KRDWTP_MAKE_COLOR(0xFF,0xFF,0x00,0xFF)
#define KRDWTP_COLOR_CYAN   KRDWTP_MAKE_COLOR(0x00,0xFF,0xFF,0xFF)

#define KRDWTP_COLOR_DARK_RED KRDWTP_MAKE_COLOR(0x80,0x00,0x00,0xFF)

typedef struct
{
    BYTE* CharacterSet;    // Pointer to font bitset. In form: `uint8_t CharSet[NumberOfEntries][ColumnsPerEntry]`
    QWORD NumberOfEntries; // No. character entries in CharacterSet.
    BYTE  ColumnsPerEntry; // Columns per each character.
    BYTE  RowsPerEntry;    // Rows per each character.
    BYTE  ScaleFactor;     // Scaling factor, leave as 1 nor NOP.
    BOOL  bDirectionBit;   // If false, uses MSB, if true, uses LSB.
} KrDisplaywideTextProtocolFont;
KrDisplaywideTextProtocolFont KrdwtpScaleFont(const KrDisplaywideTextProtocolFont* pFont, BYTE factor);

typedef struct
{
    DWORD   FrameBufferWidth;
    DWORD   FrameBufferHeight;
    DWORD   PixelsPerScanLine;
    DWORD   CursorX;
    DWORD   CursorY;
    UINTPTR AddrFrameBuffer;
    DWORD   BytesPerPixel;
    
    KrDisplaywideTextProtocolFont Font;
    BOOL bUppercaseMode; // If true, each call to KrdwtpOutCharacter with a lowercase character will automatically be uppercased.
} KrDisplaywideTextProtocolState;

void KrdwtpInitialize(KrDisplaywideTextProtocolFont Font, UINTPTR AddrFrameBuffer, DWORD FramebufferWidth, DWORD FramebufferHeight, DWORD PixelsPerScanLine);
void KrdwtpReloadFrameBuffer(UINTPTR AddrFrameBuffer);
// Clears the framebuffer with given color and moves the cursor to its initial default position.
void KrdwtpResetState(DWORD ClearColor);
void KrdwtpScroll(void);

void KrdwtpOutColoredCharacter(char Char, DWORD ForegroundColor, DWORD BackgroundColor);
void KrdwtpOutCharacter(char Char);
int  KrdwtpOutColoredText(const char* pText, DWORD ForegroundColor, DWORD BackgroundColor);
int  KrdwtpOutText(const char* pText);

int  KrdwtpOutFormatTextVariadic(const char* pFmt, va_list args);
int  KrdwtpOutFormatText(const char* pFmt, ...);

void KrdwtpSetUppercaseMode(BOOL bUppercaseModeEnabled);
const KrDisplaywideTextProtocolState* KrdwtpGetProtocolState(void);

#endif // !YCH_KERNEL_EARLYVIDEO_DISPLAYWIDE_TEXT_PROTOCOL
