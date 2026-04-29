#include "CPU/PAT.h"

#include "Core/KernelState.h"

static KrPatSelect g_PatSelectTable[8] =
{
    { .PAT = 0, .PCD = 0, .PWT = 0 },
    { .PAT = 0, .PCD = 0, .PWT = 1 },
    { .PAT = 0, .PCD = 1, .PWT = 0 },
    { .PAT = 0, .PCD = 1, .PWT = 1 },
    { .PAT = 1, .PCD = 0, .PWT = 0 },
    { .PAT = 1, .PCD = 0, .PWT = 1 },
    { .PAT = 1, .PCD = 1, .PWT = 0 },
    { .PAT = 1, .PCD = 1, .PWT = 1 }
};

KrPatSelect KrSelectPat(BYTE MemType)
{
    BYTE Target = 0;
    switch (MemType)
    {
    case KR_PAT_UNCACHEABLE:
    {
        Target = g_KernelState.PatMsrState.PA_UC;
        break;
    }
    case KR_PAT_WRITE_COMBINING:
    {
        Target = g_KernelState.PatMsrState.PA_WC;
        break;
    }
    case KR_PAT_WRITE_THROUGH:
    {
        Target = g_KernelState.PatMsrState.PA_WT;
        break;
    }
    case KR_PAT_WRITE_PROTECTED:
    {
        Target = g_KernelState.PatMsrState.PA_WP;
        break;
    }
    case KR_PAT_WRITE_BACK:
    {
        Target = g_KernelState.PatMsrState.PA_WB;
        break;
    }
    case KR_PAT_UNCACHED:
    {
        Target = g_KernelState.PatMsrState.PA_UCM;
        break;
    }
    }

    // Target cannot be larger than 7 because PA_ fields of PatMsrState are bit fields with 4 bits
    // we can safely index the array

    return g_PatSelectTable[Target];
}
