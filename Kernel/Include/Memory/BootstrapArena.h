#ifndef YCH_KERNEL_MEMORY_BOOTSTRAP_ARENA_H
#define YCH_KERNEL_MEMORY_BOOTSTRAP_ARENA_H

#include "Krnlych.h"

/**
 * @brief Initializes the Bootstrap Kernel Arena acquisition system.
 * 
 * @param pBase The base address of the bootstrap arena.
 * @param szArena The size of the bootstrap arena in bytes.
 */
BOOL KrInitBootstrapArena(VOID* pBase, SIZE szArena);

/**
 * @brief Acquires N amount of bytes in the Bootstrap Kernel Arena.
 * 
 * @param N Number of bytes to allocate.
 * @return Address to the acquired memory, NULLPTR if failure.
 */
VOID* KrBootstrapArenaAcquire(SIZE N);

/**
 * @brief Releases memory previously allocated by KrBootstrapArenaAcquire.
 * 
 * @param pPtr Pointer to the memory to release, must be allocated by KrBootstrapArenaAcquire.
 */
BOOL KrBootstrapArenaRelinquish(VOID* pPtr);

#endif // !YCH_KERNEL_MEMORY_BOOTSTRAP_ARENA_H
