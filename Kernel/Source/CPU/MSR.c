#include "CPU/MSR.h"

QWORD KrReadModelSpecificRegister(DWORD RegisterID)
{
    DWORD lodword, hidword;
    __asm__ __volatile__ ("rdmsr" : "=a"(lodword), "=d"(hidword) : "c"(RegisterID) :);
    return ((QWORD) hidword << 32) | lodword;
}

VOID KrWriteModelSpecificRegister(DWORD RegisterID, QWORD qwData)
{
    DWORD lodword = LODWORD(qwData);
    DWORD hidword = HIDWORD(qwData);
    __asm__ __volatile__ ("wrmsr" : : "c"(RegisterID), "a"(lodword), "d"(hidword) : "memory");
}
