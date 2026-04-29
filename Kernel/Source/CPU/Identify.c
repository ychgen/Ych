#include "CPU/Identify.h"

#include "CPU/CPUID.h"

BOOL KrGetProcessorInfoAndFeatures(KrProcessorInfoAndFeatures* pOutStruct)
{
    if (!pOutStruct)
    {
        return FALSE;
    }

    DWORD EAX, EBX, ECX, EDX;
    KrCPUID(KR_CPUID_LEAF_PRC_INF_FEAT_BITS, EAX, EBX, ECX, EDX);

    // EAX
    {
        pOutStruct->Stepping = EAX & 0xF;
        
        WORD BaseModel  = (EAX >>  4) & 0xF;
        WORD BaseFamily = (EAX >>  8) & 0xF;
        WORD ExtdModel  = (EAX >> 16) & 0xF;
        WORD ExtdFamily = (EAX >> 20) & 0xFF;

        pOutStruct->Model  = (BaseFamily == 6 || BaseFamily == 15) ? ((ExtdModel << 4) + BaseModel) : (BaseModel);
        pOutStruct->Family = (BaseFamily == 15) ? (ExtdFamily + BaseFamily) : (BaseFamily);
    }
    
    return TRUE;
}

VOID KrGetProcessorManufacturerID(CHAR* pOut)
{
    DWORD EAX, EBX, ECX, EDX;
    KrCPUID(KR_CPUID_LEAF_HFP_MANUFACID, EAX, EBX, ECX, EDX);
    *((DWORD*) pOut) = EBX; pOut += 4;
    *((DWORD*) pOut) = EDX; pOut += 4; // Yes order is EBX EDX ECX don't ask me why ask Intel
    *((DWORD*) pOut) = ECX; pOut += 4;
}

VOID KrGetProcessorBrandString(CHAR* pOut)
{
    DWORD EAX, EBX, ECX, EDX;
    for (UINT i = 0; i < 3; i++)
    {
        KrCPUID(KR_CPUID_LEAF_BRAND_STR_1ST + i, EAX, EBX, ECX, EDX);
        *((DWORD*) pOut) = EAX; pOut += 4;
        *((DWORD*) pOut) = EBX; pOut += 4;
        *((DWORD*) pOut) = ECX; pOut += 4;
        *((DWORD*) pOut) = EDX; pOut += 4;
    }
}
