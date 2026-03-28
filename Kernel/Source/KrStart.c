#include <stdint.h>

// Address populated by the bootloader.
#define KR_PARAMETER_ADDRESS 0x4000000

extern void KrHalt(void);

typedef struct __attribute__((packed))
{
    uint64_t PhysicalFrameBufferAddress;
    uint64_t FramebufferSize;
    uint32_t FramebufferWidth;
    uint32_t FramebufferHeight;
    uint32_t PixelsPerScanLine;
} KrGraphicsInfo;

typedef struct __attribute__((packed))
{
    KrGraphicsInfo GraphicsInfo;
} KrBootInfo;

__attribute__((section(".text.KrStart")))
void KrStart(void)
{
    KrBootInfo bootInfo = *(KrBootInfo*) KR_PARAMETER_ADDRESS;

    uint8_t* pFrameBuffer = (uint8_t*) bootInfo.GraphicsInfo.PhysicalFrameBufferAddress;
    for (int i = 0; i < 500 * 500; i++)
    {
        pFrameBuffer[i] = 0xFF;
    }

    KrHalt();
}
