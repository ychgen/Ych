CHAR16* StrStr(const CHAR16* haystack, const CHAR16* needle) {
    if (!*needle) return (CHAR16*)haystack;

    for (const CHAR16* h = haystack; *h; h++) {
        const CHAR16 *h_sub = h, *n = needle;
        while (*h_sub && *n && *h_sub == *n) {
            h_sub++;
            n++;
        }
        if (!*n) return (CHAR16*)h;
    }
    return 0;
}

void* MemCpy(void* pDest, const void* pSrc, UINTN N)
{
    unsigned char* pDestBytes = (unsigned char*) pDest;
    const unsigned char* pSrcBytes = (unsigned char*) pSrc;

    while (N--)
    {
        *pDestBytes++ = *pSrcBytes++;
    }

    return pDest;
}
