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
 * @return TRUE if relinquished, FALSE if no-op.
 */
BOOL KrBootstrapArenaRelinquish(VOID* pPtr);

/**
 * @brief Calculates the remaining amount of free bytes within the bootstrap arena.
 * 
 * @return Space left within the bootstrap arena.
 */
SIZE KrBootstrapArenaGetSpaceLeft(VOID);

const VOID* KrBootstrapArenaGetBase(VOID);
const VOID* KrBootstrapArenaGetPtr(VOID);
const VOID* KrBootstrapArenaGetEnd(VOID);

#endif // !YCH_KERNEL_MEMORY_BOOTSTRAP_ARENA_H
