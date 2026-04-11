#ifndef YCH_KERNEL_MEMORY_BOOTSTRAP_ARENA_H
#define YCH_KERNEL_MEMORY_BOOTSTRAP_ARENA_H

#include "Core/Fundtypes.h"

void KrInitBootstrapArena(void* pBase, USIZE szArena);

/// @brief Acquires N amount of bytes in the Bootstrap Kernel Arena.
/// @param N Number of bytes to allocate.
/// @return Address to the acquired memory, NULLPTR if failure.
void* KrBootstrapArenaAcquire(USIZE N);

/// @brief Releases memory previously allocated by KrBootstrapArenaAcquire.
/// @param pPtr Pointer to the memory to release, must be allocated by KrBootstrapArenaAcquire.
void KrBootstrapArenaRelease(void* pPtr);

#endif // !YCH_KERNEL_MEMORY_BOOTSTRAP_ARENA_H
