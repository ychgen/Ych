#ifndef YCH_KERNEL_EARLYVIDEO_DISPLAYWIDE_TEXT_PROTOCOL
#define YCH_KERNEL_EARLYVIDEO_DISPLAYWIDE_TEXT_PROTOCOL

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>

#define KR_DISPLAYWIDE_TEXT_PROTOCOL_FONT_CHARSET_SIZE   128

#define KRDWTP_MAKE_COLOR(r,g,b,a) ( ((((uint32_t)a) & 0xFF) << 24) | ((((uint32_t)r) & 0xFF) << 16) | ((((uint32_t)g) & 0xFF) << 8) | ((((uint32_t)b) & 0xFF)))

#define KRDWTP_COLOR_BLACK KRDWTP_MAKE_COLOR(0x00,0x00,0x00,0xFF)
#define KRDWTP_COLOR_WHITE KRDWTP_MAKE_COLOR(0xFF,0xFF,0xFF,0xFF)
#define KRDWTP_COLOR_RED   KRDWTP_MAKE_COLOR(0xFF,0x00,0x00,0xFF)
#define KRDWTP_COLOR_GREEN KRDWTP_MAKE_COLOR(0x00,0xFF,0x00,0xFF)
#define KRDWTP_COLOR_BLUE  KRDWTP_MAKE_COLOR(0x00,0xFF,0x00,0xFF)

typedef struct
{
    uint8_t* CharacterSet;    // Pointer to font bitset. In form: `uint8_t CharSet[NumberOfEntries][ColumnsPerEntry]`
    uint64_t NumberOfEntries; // No. character entries in CharacterSet.
    uint8_t  ColumnsPerEntry; // Columns per each character.
    uint8_t  RowsPerEntry;    // Rows per each character.
    uint8_t  ScaleFactor;     // Scaling factor, leave as 1 nor NOP.
    bool     bDirectionBit;   // If false, uses MSB, if true, uses LSB.
} KrDisplaywideTextProtocolFont;
KrDisplaywideTextProtocolFont KrdwtpScaleFont(const KrDisplaywideTextProtocolFont* pFont, uint8_t factor);

typedef struct
{
    uint32_t  FrameBufferWidth;
    uint32_t  FrameBufferHeight;
    uint32_t  PixelsPerScanLine;
    uint32_t  CursorX;
    uint32_t  CursorY;
    uintptr_t AddrFrameBuffer;
    
    KrDisplaywideTextProtocolFont Font;
    bool bUppercaseMode; // If true, each call to KrdwtpOutCharacter with a lowercase character will automatically be uppercased.
} KrDisplaywideTextProtocolState;

void KrdwtpInitialize(KrDisplaywideTextProtocolFont font, uintptr_t addrFrameBuffer, uint32_t xszFrameBuffer, uint32_t yszFrameBuffer, uint32_t pxlsPerScanline);
void KrdwtpReloadFrameBuffer(uintptr_t addrFrameBuffer);
// Clears the framebuffer with given color and moves the cursor to its initial default position.
void KrdwtpResetState(uint32_t fbcolor);

void KrdwtpOutColoredCharacter(char character, uint32_t color);
void KrdwtpOutCharacter(char character);
int  KrdwtpOutColoredText(const char* pText, uint32_t color);
int  KrdwtpOutText(const char* pText);

int  KrdwtpOutFormatTextVariadic(const char* pFmt, va_list args);
int  KrdwtpOutFormatText(const char* pFmt, ...);

void KrdwtpSetUppercaseMode(bool bUppercaseModeEnabled);
const KrDisplaywideTextProtocolState* KrdwtpGetProtocolState(void);

#endif // !YCH_KERNEL_EARLYVIDEO_DISPLAYWIDE_TEXT_PROTOCOL
