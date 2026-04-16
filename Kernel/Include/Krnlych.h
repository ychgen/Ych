/**
 * 
 * Krnlych.h is a header defining fundamental units of processing and some convenient macro values for them.
 * NOTE: We specifically rely on GCC/Clang specific types internally because the genius C committee cannot fucking
 * provide us with builtin native standard sized integer types without making us include stdint.h are we fucking serious?   
 * 
 * This header assumes classic GCC/Clang convention of:
 * char  ->  8-bit
 * short -> 16-bit
 * int   -> 32-bit
 * long  -> 64-bit (this is the biggest potential blowing your brains out case. MSVC would be disappointed, but I am already disappointed in her.)
 * 
 * Kernelych uses the LP64 model, unlike some other OS that uses LLP64.
 * In our case, a long is 64-bit while char, short, and int meet the sizes every normal modern C programmer expects; 8, 16 and 32 respectively.
 * 
 */
#ifndef YCH_KERNEL_KRNLYCH_H
#define YCH_KERNEL_KRNLYCH_H

#define KR_CEILDIV(Dividend, Divisor) (((Dividend) + (Divisor) - 1) / Divisor)

/** Defined as macros just in case in the future we change compilers, we are not completely GCC dependent. */

#define KR_NORETURN   __attribute__((noreturn))
#define KR_PACKED     __attribute__((packed))
#define KR_ALIGNED(N) __attribute__((aligned(N)))
#define KR_SECTION(S) __attribute__((section(S)))

#define VOID      void
#define NULLPTR ((VOID*) 0)
#define KR_UNUSED(X) ((VOID)(X))

#define FALSE            0
#define TRUE             1

/** Types to be used for hardware/memory oriented and size-specific operations, therefore named to keep consistent and close with x86-64 ISA. */

typedef unsigned char  BYTE;    //  8 bit unsigned, physical.
typedef unsigned short WORD;    // 16 bit unsigned, physical.
typedef unsigned int   DWORD;   // 32 bit unsigned, physical.
typedef unsigned long  QWORD;   // 64 bit unsigned, physical.

/** Types to be used with normal logical operations. Such as Type IDs for example, something not hardware or memory oriented. */

typedef          char  CHAR;    //  8 bit   signed, logical.
typedef          short SHORT;   // 16 bit   signed, logical.
typedef          int   INT;     // 32 bit   signed, logical.
typedef          long  LONG;    // 64 bit   signed, logical.
/**
 * @brief 32 bit  signed, logical. 32-bit for better CPU mapping.
 * Type capable of storing at least 0 and 1. 0 means FALSE, anything else means TRUE, 1 is preferred.
 * This is mainly for function returns, if you are storing a lot of BOOLs in a struct, you are doing it wrong. Prefer using bit fields or BYTE.
 */
typedef          int   BOOL;

typedef unsigned char  UCHAR;   //  8 bit unsigned, logical.
typedef unsigned short USHORT;  // 16 bit unsigned, logical.
typedef unsigned int   UINT;    // 32 bit unsigned, logical.
typedef unsigned long  ULONG;   // 64 bit unsigned, logical.

// It gets tiring typing `const char*` everytime a static string is needed.
// Therefore we add CSTR. If a non-const version is needed, use CHAR* as it makes more sense (I need a sequence of characters, not a static string).

typedef const char*    CSTR;    // Pointer to a C-string. Sequences of characters that are null-terminated. Always treat as UTF-8 encoded, if someone doesn't listen, it's on them.

/** Target architecture word size specific */
// NOTE: Since kernel is 64-bit only, we use LONG. No need for #ifdef X86_64 or stuff.

typedef          long  INTMAX;  //   Signed integer of maximum processing size.
typedef unsigned long  UINTMAX; // Unsigned integer of maximum processing size.

typedef          long  ARITH;   // `Arithmetic` Unit, the native arithmetic type for target architecture.      Signed. Comparable to `ssize_t`.
typedef unsigned long  SIZE;    // `Size`       Unit, the native counting/size type for target architecture. Unsigned. Comparable to `size_t`.

typedef          long  INTPTR;  //   Signed integer value capable of storing the value of a relative pointer. Since this is   signed, it's almost always used as displacement.
typedef unsigned long  UINTPTR; // Unsigned integer value capable of storing the value of a pointer. Since this is unsigned, it's almost always used to store an absolute address.
typedef          long  PTRDIFF; // The result of subtracting two pointers, i.e. `difference` of pointers. Signed integer.

// Useful quick macros

#define HIDWORD(QW) ((DWORD)(((QWORD)(QW)) >> 32))
#define LODWORD(QW) ((DWORD)(((QWORD)(QW))      ))

#define HIWORD(XW)  ((WORD)((LODWORD(XW))  >> 16))
#define LOWORD(XW)  ((WORD)((LODWORD(XW))       ))

#define HIBYTE(XW)  ((BYTE)((LOWORD(XW))   >>  8))
#define LOBYTE(XW)  ((BYTE)((LOWORD(XW))        ))

#endif // !YCH_KERNEL_KRNLYCH_H
