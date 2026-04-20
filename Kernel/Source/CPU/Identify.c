#include "CPU/Identify.h"

#include "CPU/CPUID.h"

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
