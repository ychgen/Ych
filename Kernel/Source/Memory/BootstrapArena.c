#include "Memory/BootstrapArena.h"

BYTE* g_pBootstrapArenaBase = NULLPTR;
BYTE* g_pBootstrapArenaPtr  = NULLPTR;
BYTE* g_pBootstrapArenaEnd  = NULLPTR;

void KrInitBootstrapArena(void* pBase, USIZE szArena)
{
    g_pBootstrapArenaBase = g_pBootstrapArenaPtr = (BYTE*) pBase;
    g_pBootstrapArenaEnd = g_pBootstrapArenaBase + szArena;
}

void* KrBootstrapArenaAcquire(USIZE N)
{
    if (!N || (UINTPTR)(g_pBootstrapArenaPtr + N) >= (UINTPTR) g_pBootstrapArenaEnd)
    {
        return NULLPTR;
    }
    BYTE* pAddr = g_pBootstrapArenaPtr;
    g_pBootstrapArenaPtr += N;
    return pAddr;
}

void KrBootstrapArenaRelease(void* pPtr)
{
    (void)(pPtr);
    return;
}
