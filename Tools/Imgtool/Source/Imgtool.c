#include "Imgtool.h"

#include <memory.h>

void Imgtool_LBAToCHS(uint8_t* pCHS, uint64_t LBA)
{
    const uint32_t heads = 255;
    const uint32_t sectors = 63;

    uint32_t c, h, s;

    // Compute CHS
    c = (uint32_t)(LBA / (heads * sectors));
    uint32_t temp = (uint32_t)(LBA % (heads * sectors));

    h = temp / sectors;
    s = (temp % sectors) + 1;

    if (c > 1023) {
        c = 1023;
        h = 254;
        s = 63;
    }

    pCHS[0] = (uint8_t)h;
    pCHS[1] = (uint8_t)((s & 0x3F) | ((c >> 2) & 0xC0));
    pCHS[2] = (uint8_t)(c & 0xFF);
}

void Imgtool_WriteASCIIToUNICODE16LE(uint8_t* pDest, const char* pSrc)
{
    int i;
    for (i = 0; i < GPT_PARTITION_NAME_CHAR_COUNT && *pSrc; i++)
    {
        *pDest++ = *pSrc++;
        *pDest++ =  0x00;
    }
    memset(pDest, 0x00, (GPT_PARTITION_NAME_CHAR_COUNT - i) * sizeof(uint16_t));
}

static uint32_t TableCRC32[256];

void InitTableCRC32(void)
{
    for (uint32_t i = 0; i < 256; i++)
    {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++)
        {
            if (crc & 1)
            {
                crc = (crc >> 1) ^ 0xEDB88320;
            }
            else
            {
                crc >>= 1;
            }
        }
        TableCRC32[i] = crc;
    }
}

uint32_t ChecksumCRC32(const void* pData, size_t szData)
{
    const uint8_t* buf = (const uint8_t*) pData;
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < szData; i++)
    {
        uint8_t index = (uint8_t)((crc ^ buf[i]) & 0xFF);
        crc = (crc >> 8) ^ TableCRC32[index];
    }

    return crc ^ 0xFFFFFFFF;
}
