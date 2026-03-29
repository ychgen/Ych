#include "Memory/Krnlmem.h"

void* KrContiguousSetBuffer(void* pDest, int iVal, size_t N)
{
    unsigned char* pDestBytes = (unsigned char*) pDest;
    while (N--)
    {
        *pDestBytes++ = (unsigned char) iVal;
    }
    return pDest;
}

void* KrContiguousZeroBuffer(void* pDest, size_t N)
{
    return KrContiguousSetBuffer(pDest, 0x00000000, N);
}

void* KrContiguousCopyBuffer(void* restrict pDest, const void* restrict pSrc, size_t N)
{
    unsigned char* restrict pDestBytes = (unsigned char* restrict) pDest;
    const unsigned char* restrict pSrcBytes = (const unsigned char* restrict) pSrc;

    while (N--)
    {
        *pDestBytes++ = *pSrcBytes++;
    }

    return pDest;
}
