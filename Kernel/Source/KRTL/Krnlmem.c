#include "KRTL/Krnlmem.h"

void* KrtlContiguousSetBuffer(void* pDest, uint8_t uVal, size_t N)
{
    unsigned char* pDestBytes = (unsigned char*) pDest;
    while (N--)
    {
        *pDestBytes++ = uVal;
    }
    return pDest;
}

void* KrtlContiguousZeroBuffer(void* pDest, size_t N)
{
    return KrtlContiguousSetBuffer(pDest, 0x00, N);
}

void* KrtlContiguousCopyBuffer(void* restrict pDest, const void* restrict pSrc, size_t N)
{
    unsigned char* restrict pDestBytes = (unsigned char* restrict) pDest;
    const unsigned char* restrict pSrcBytes = (const unsigned char* restrict) pSrc;

    while (N--)
    {
        *pDestBytes++ = *pSrcBytes++;
    }

    return pDest;
}

void* KrtlContiguousMoveBuffer(void* pDest, const void* pSrc, size_t N)
{
    unsigned char* pDestBytes = (unsigned char*) pDest;
    const unsigned char* pSrcBytes = (const unsigned char*)pSrc;

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
