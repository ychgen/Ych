#include "Memory/BootstrapArena.h"

#define KR_BOOTSTRAP_ARENA_ALIGNMENT 16

BYTE* g_pBootstrapArenaBase;
BYTE* g_pBootstrapArenaPtr;
BYTE* g_pBootstrapArenaEnd;

BOOL KrInitBootstrapArena(VOID* pBase, SIZE szArena)
{
    if (!szArena || (g_pBootstrapArenaBase || g_pBootstrapArenaPtr || g_pBootstrapArenaEnd))
    {
        return FALSE;
    }
    g_pBootstrapArenaBase = g_pBootstrapArenaPtr = (BYTE*) pBase;
    g_pBootstrapArenaEnd = g_pBootstrapArenaBase + szArena;
    return TRUE;
}

BOOL KrBootstrapArenaAlign(UINT Alignment)
{
    BYTE* pAligned = (BYTE*)(((UINTPTR) g_pBootstrapArenaPtr + Alignment - 1) & ~((UINTPTR) Alignment - 1));
    if (pAligned > g_pBootstrapArenaEnd)
    {
        return FALSE;
    }
    g_pBootstrapArenaPtr = pAligned;
    return TRUE;
}

VOID* KrBootstrapArenaAcquire(SIZE N)
{
    N = (N + KR_BOOTSTRAP_ARENA_ALIGNMENT - 1) & ~(KR_BOOTSTRAP_ARENA_ALIGNMENT - 1);
    if (!N || (UINTPTR)(g_pBootstrapArenaPtr + N) > (UINTPTR) g_pBootstrapArenaEnd)
    {
        return NULLPTR;
    }
    BYTE* pAddr = g_pBootstrapArenaPtr;
    g_pBootstrapArenaPtr += N;
    return pAddr;
}

VOID KrBootstrapArenaRelease(VOID* pPtr)
{
    (VOID)(pPtr);
    return;
}

SIZE KrBootstrapArenaGetSpaceLeft(VOID)
{
    return g_pBootstrapArenaPtr ? (SIZE)(g_pBootstrapArenaEnd - g_pBootstrapArenaPtr) : 0;
}

const VOID* KrBootstrapArenaGetBase(VOID)
{
    return g_pBootstrapArenaBase;
}

const VOID* KrBootstrapArenaGetPtr(VOID)
{
    return g_pBootstrapArenaPtr;
}

const VOID* KrBootstrapArenaGetEnd(VOID)
{
    return g_pBootstrapArenaEnd;
}
