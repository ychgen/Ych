#ifndef YCH_KERNEL_EARLYVIDEO_DISPLAYWIDE_TEXT_PROTOCOL
#define YCH_KERNEL_EARLYVIDEO_DISPLAYWIDE_TEXT_PROTOCOL

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>

// 8x8. bit 0 = px off ; bit 1 = px on
#define KR_DISPLAYWIDE_TEXT_PROTOCOL_FONT_CHAR_ROW_COUNT 8
#define KR_DISPLAYWIDE_TEXT_PROTOCOL_FONT_CHARSET_SIZE   128

#define KRDWTP_MAKE_COLOR(r,g,b,a) ( ((((uint32_t)a) & 0xFF) << 24) | ((((uint32_t)r) & 0xFF) << 16) | ((((uint32_t)g) & 0xFF) << 8) | ((((uint32_t)b) & 0xFF)))

#define KRDWTP_COLOR_BLACK KRDWTP_MAKE_COLOR(0x00,0x00,0x00,0xFF)
#define KRDWTP_COLOR_WHITE KRDWTP_MAKE_COLOR(0xFF,0xFF,0xFF,0xFF)
#define KRDWTP_COLOR_RED   KRDWTP_MAKE_COLOR(0xFF,0x00,0x00,0xFF)
#define KRDWTP_COLOR_GREEN KRDWTP_MAKE_COLOR(0x00,0xFF,0x00,0xFF)
#define KRDWTP_COLOR_BLUE  KRDWTP_MAKE_COLOR(0x00,0xFF,0x00,0xFF)

typedef uint8_t KrDisplaywideTextProtocolFontCharacter[KR_DISPLAYWIDE_TEXT_PROTOCOL_FONT_CHAR_ROW_COUNT];

typedef struct
{
    KrDisplaywideTextProtocolFontCharacter* CharacterSet;
    uint8_t ScaleFactor;
} KrDisplaywideTextProtocolFont;

typedef struct
{
    uint32_t  FrameBufferWidth;
    uint32_t  FrameBufferHeight;
    uint32_t  RowStride;
    uint32_t  CursorX;
    uint32_t  CursorY;
    uintptr_t AddrFrameBuffer;
    
    KrDisplaywideTextProtocolFont Font;
    bool bUppercaseMode; // If true, each call to KrdwtpOutCharacter with a lowercase character will automatically be uppercased.
} KrDisplaywideTextProtocolState;

void KrdwtpInitialize(KrDisplaywideTextProtocolFont font, uintptr_t addrFrameBuffer, uint32_t xszFrameBuffer, uint32_t yszFrameBuffer, uint32_t rowStride);
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
