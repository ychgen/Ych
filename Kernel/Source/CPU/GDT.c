#include "CPU/GDT.h"

KrEncodeSegmentDescriptor_Result KrEncodeSegmentDescriptor(void* pDest, const KrGlobalDescriptorTableSegmentDescriptor* pSrc)
{
    /**
     * GDT Layout:
     * 
     * |   Bits   |           Field          |
     *   [00-15]      Limit (LOW)
     *   [16-31]      Base  (LOW)
     *   [32-39]      Base  (MID)
     *   [40-47]      Access Byte
     *   [48-51]      Limit (HIGH)
     *   [52-55]      Flags  (rel0: reserved, rel1: LM Code, rel2: size flag, rel3: granularity)
     *   [55-63]      Base  (HIGH)
     */

    if (pSrc->Limit > 0xFFFFF)
    {
        return ENCODE_SEGMENT_DESCRIPTOR_LIMIT_TOO_LARGE;
    }

    uint8_t* pDestBytes = (uint8_t*) pDest;
    *pDestBytes++  = (pSrc->Limit >> 0 ) & 0xFF;
    *pDestBytes++  = (pSrc->Limit >> 8 ) & 0xFF;
    *pDestBytes++  = (pSrc->Base  >> 0 ) & 0xFF;
    *pDestBytes++  = (pSrc->Base  >> 8 ) & 0xFF;
    *pDestBytes++  = (pSrc->Base  >> 16) & 0xFF;
    
    *pDestBytes    = 0;
    *pDestBytes   |= (pSrc->Access_Present    & 0x01) << 7;
    *pDestBytes   |= (pSrc->Access_DPL        & 0x03) << 5;
    *pDestBytes   |= (pSrc->Access_DescType   & 0x01) << 4;
    *pDestBytes   |= (pSrc->Access_Executable & 0x01) << 3;
    *pDestBytes   |= (pSrc->Access_DC         & 0x01) << 2;
    *pDestBytes   |= (pSrc->Access_ReadWrite  & 0x01) << 1;
    *pDestBytes++ |= (pSrc->Access_Accessed   & 0x01) << 0;

    uint8_t flags = 0;
    flags |= (pSrc->Flags_Granularity  & 0x01) << 3;
    flags |= (pSrc->Flags_Size         & 0x01) << 2;
    flags |= (pSrc->Flags_LongModeCode & 0x01) << 1;
    flags |= (pSrc->Flags_Reserved     & 0x01) << 0;

    *pDestBytes++  = (flags << 4) | ((pSrc->Limit >> 12) & 0xF);
    *pDestBytes++  = (pSrc->Base >> 24) & 0xFF;

    return ENCODE_SEGMENT_DESCRIPTOR_SUCCESS;
}

uint16_t KrConstructSegmentSelector(uint8_t RPL, uint16_t i)
{
    if (RPL > 3 || i > 0x1FFF)
    {
        return 0;
    }
    return (uint16_t)(((i << 3) & 0xFFF8) | (0 << 2) | (RPL & 0x03));
}
