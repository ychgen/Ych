#include "KRTL/Krnlchar.h"

BOOL KrtlIsCharDigit(CHAR Ch)
{
    return Ch >= '0' && Ch <= '9';
}

BOOL KrtlIsCharUpper(CHAR Ch)
{
    return Ch >= 'A' && Ch <= 'Z';
}

BOOL KrtlIsCharLower(CHAR Ch)
{
    return Ch >= 'a' && Ch <= 'z';
}

BOOL KrtlIsCharAlpha(CHAR Ch)
{
    return KrtlIsCharLower(Ch) || KrtlIsCharUpper(Ch);
}

BOOL KrtlIsCharPunct(CHAR Ch)
{
    return  (Ch >= '!' && Ch <= '/') ||
            (Ch >= ':' && Ch <= '@') ||
            (Ch >= '[' && Ch <= '`') ||
            (Ch >= '{' && Ch <= '~');
}

BOOL KrtlIsCharPrint(CHAR Ch)
{
    return KrtlIsCharAlpha(Ch) || KrtlIsCharDigit(Ch) || KrtlIsCharPunct(Ch) || Ch == ' ';
}
