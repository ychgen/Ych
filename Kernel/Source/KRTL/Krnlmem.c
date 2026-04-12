#include "KRTL/Krnlmem.h"

void* KrtlContiguousSetBuffer(void* pDest, BYTE uVal, USIZE N)
{
    BYTE* pDestBytes = (BYTE*) pDest;
    while (N--)
    {
        *pDestBytes++ = uVal;
    }
    return pDest;
}

void* KrtlContiguousZeroBuffer(void* pDest, USIZE N)
{
    return KrtlContiguousSetBuffer(pDest, 0x00, N);
}

void* KrtlContiguousCopyBuffer(void* restrict pDest, const void* restrict pSrc, USIZE N)
{
    BYTE* restrict pDestBytes = (BYTE* restrict) pDest;
    const BYTE* restrict pSrcBytes = (const BYTE* restrict) pSrc;

    while (N--)
    {
        *pDestBytes++ = *pSrcBytes++;
    }

    return pDest;
}

void* KrtlContiguousMoveBuffer(void* pDest, const void* pSrc, USIZE N)
{
    BYTE* pDestBytes = (BYTE*) pDest;
    const BYTE* pSrcBytes = (const BYTE*)pSrc;

    if (pDestBytes == pSrcBytes || N == 0)
        return pDest;

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
