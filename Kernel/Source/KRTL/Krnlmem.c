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

VOID* KrtlContiguousCopyBuffer(VOID* restrict pDest, const VOID* restrict pSrc, SIZE N)
{
          BYTE* restrict pDestBytes = (      BYTE* restrict) pDest;
    const BYTE* restrict pSrcBytes  = (const BYTE* restrict) pSrc;

    while (N--)
    {
        *pDestBytes++ = *pSrcBytes++;
    }

    return pDest;
}

VOID* KrtlContiguousMoveBuffer(VOID* pDest, const VOID* pSrc, SIZE N)
{
          BYTE* pDestBytes = (      BYTE*) pDest;
    const BYTE* pSrcBytes  = (const BYTE*) pSrc;

    if (pDestBytes == pSrcBytes || N == 0)
    {
        return pDest;
    }

    if (pDestBytes < pSrcBytes)
    {
        while (N--)
        {
            *pDestBytes++ = *pSrcBytes++;
        }
    }
    else
    {
        pDestBytes += N;
        pSrcBytes += N;

        while (N--)
        {
            *--pDestBytes = *--pSrcBytes;
        }
    }

    return pDest;
}
