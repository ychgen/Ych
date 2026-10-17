#include "Core/Assert.h"

#include "KRTL/Krnlstring.h"

#define MAX_MDDESC_SIZE 8192

VOID ImplKernelAssert(BOOL Expr, MDCODE MdCode, CSTR MdDesc, ...)
{
    if (Expr)
    {
        return;
    }
    CHAR MdFmtDesc[MAX_MDDESC_SIZE];

    if (MdDesc)
    {
        va_list Args;
        va_start(Args, MdDesc);
        {
            KrtlFormatStringVariadic(MAX_MDDESC_SIZE, MdFmtDesc, MdDesc, Args);
        }
        va_end(Args);
        
        MdDesc = MdFmtDesc;
    }

    Krnlmeltdownimm(MdCode, MdDesc);
}
