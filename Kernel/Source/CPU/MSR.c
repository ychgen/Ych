#include "CPU/MSR.h"

QWORD KrReadModelSpecificRegister(DWORD idRegister)
{
    DWORD lodword, hidword;
    __asm__ __volatile__ ("rdmsr" : "=a"(lodword), "=d"(hidword) : "c"(idRegister) :);
    return ((QWORD) hidword << 32) | lodword;
}

void KrWriteModelSpecificRegister(DWORD idRegister, DWORD data)
{
    DWORD lodword = (DWORD) data;
    DWORD hidword = (DWORD)(data >> 32);
    __asm__ __volatile__ ("wrmsr" : : "c"(idRegister), "a"(lodword), "d"(hidword) : "memory");
}
