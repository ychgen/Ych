/**
 * 
 * Krnlmeltdown
 * Kernel panic mechanism of the Ych OS Kernel. Dubbed "kernel meltdown".
 * 
 */

#ifndef YCH_KERNEL_CORE_KRNLMELTDOWN_H
#define YCH_KERNEL_CORE_KRNLMELTDOWN_H

#include "Core/Private/meltdowncodes.h"
#include "CPU/ProcessorSnapshot.h"

// MDCODE descriptor
CSTR Krnlmddesc(MDCODE code);

/**
 * @brief Triggers a kernel meltdown (kernel panic), ceasing all normal kernel functionality.
 * This function won't return, will do minimal logging, notify user about error and hang the system indefinitely.
 * 
 * @param code The crash code, i.e. the reason for this meltdown.
 * @param pDesc Detailed description about this crash. Leave as NULL for kernel to populate it if `code` is a standard meltdown code.
 * @param pSnapshot The snapshot of the processor at the moment of error.
 */
VOID Krnlmeltdown(MDCODE code, CSTR pDesc, const KrProcessorSnapshot* pSnapshot) __attribute__((noreturn));

/**
 * This macro takes an instant snapshot of the processor and invokes Krnlmeltdown, causing a kernel meltdown.
 * NOTE: parameters `mdCode` and `pMdDesc` must be in memory, you cannot use constants.
 */
#define Krnlmeltdownimm(mdCode, pMdDesc) (VOID)(&mdCode); (VOID)(&pMdDesc); /*if in mem, noop. if not in mem, compile error. small enforcement to catch literal and NULL passes*/ \
    __asm__ __volatile__ (                       \
    "cli\n\t"                                    \
    "call 1f\n\t"                                \
    "1:\n\t"                                     \
    "pushq %%rsp\n\t"                            \
    "pushfq\n\t"                                 \
    "pushq %%rax\n\t"                            \
    "pushq %%rbx\n\t"                            \
    "pushq %%rcx\n\t"                            \
    "pushq %%rdx\n\t"                            \
    "pushq %%rbp\n\t"                            \
    "pushq %%rsi\n\t"                            \
    "pushq %%rdi\n\t"                            \
    "pushq %%r8\n\t"                             \
    "pushq %%r9\n\t"                             \
    "pushq %%r10\n\t"                            \
    "pushq %%r11\n\t"                            \
    "pushq %%r12\n\t"                            \
    "pushq %%r13\n\t"                            \
    "pushq %%r14\n\t"                            \
    "pushq %%r15\n\t"                            \
    "movq 128(%%rsp), %%rax\n\t"                 \
    "addq $8, %%rax\n\t"                         \
    "movq %%rax, 128(%%rsp)\n\t"                 \
    "movq 136(%%rsp), %%rax\n\t"                 \
    "subq $5, %%rax\n\t"                         \
    "movq %%rax, 136(%%rsp)\n\t"                 \
    "movq %0, %%rdi\n\t"                         \
    "movq %1, %%rsi\n\t"                         \
    "movq %%rsp, %%rdx\n\t"                      \
    "jmp Krnlmeltdown\n\t"                       \
    : : "m"(mdCode), "m"(pMdDesc) :                  \
    "rax", "rbx", "rcx", "rdx",        "rsi",   \
    "rdi", "r8", "r9", "r10", "r11", "r12",      \
    "r13", "r14", "r15", "memory"                \
    );                                           \
    __builtin_unreachable()

#endif // !YCH_KERNEL_CORE_KRNLMELTDOWN_H
