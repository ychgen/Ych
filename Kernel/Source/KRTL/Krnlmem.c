#include "KRTL/Krnlmem.h"

VOID* KrtlContiguousSetBuffer(VOID* pDest, BYTE Value, SIZE N)
{
    BYTE* pDestBytes = (BYTE*) pDest;
    while (N--)
    {
        *pDestBytes++ = Value;
    }
    return pDest;
}

VOID* KrtlContiguousZeroBuffer(VOID* pDest, SIZE N)
{
    return KrtlContiguousSetBuffer(pDest, 0x00, N);
}

BOOL KrtlBufferEqual(const VOID* pLHS, const VOID* pRHS, SIZE N)
{
    const BYTE* pLeftBytes = (const BYTE*) pLHS;
    const BYTE* pRightBytes = (const BYTE*) pRHS;

    while (*pLeftBytes++ == *pRightBytes++ && N--);
    return !N;
}

BOOL KrtlIsPowerOfTwoAligned(UINTPTR Address, UINT PowerOfTwoAlignment)
{
    return !(Address & (PowerOfTwoAlignment - 1));
}

UINTPTR KrtlAlignUpToPowerOfTwo(UINTPTR Address, UINT PowerOfTwoAlignment)
{
    return (Address + (PowerOfTwoAlignment - 1)) & ~(((UINTPTR) PowerOfTwoAlignment) - 1);
}
