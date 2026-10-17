#ifndef KRNLYCH_CORE_ASSERT_H
#define KRNLYCH_CORE_ASSERT_H

#include "Core/Krnlmeltdown.h"

// Do not call directly. Use the macros instead.
VOID ImplKernelAssert(BOOL Expr, MDCODE MdCode, CSTR MdDesc, ...);

// Only on debug builds (currently all)
#define RUNTIME_ASSERT(Expr, MdCode, MdDesc, ...) ImplKernelAssert((Expr), MdCode, MdDesc, ##__VA_ARGS__)
// Always executed on all build types
#define RUNTIME_ENSURE(Expr, MdCode, MdDesc, ...) ImplKernelAssert((Expr), MdCode, MdDesc, ##__VA_ARGS__)

#endif // !KRNLYCH_CORE_ASSERT_H
