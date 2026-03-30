#include "CPU/MSR.h"

uint64_t KrReadModelSpecificRegister(uint32_t idRegister)
{
    uint32_t lodword, hidword;
    __asm__ __volatile__ ("rdmsr" : "=a"(lodword), "=d"(hidword) : "c"(idRegister) :);
    return ((uint64_t) hidword << 32) | lodword;
}

void KrWriteModelSpecificRegister(uint32_t idRegister, uint64_t data)
{
    uint32_t lodword = (uint32_t) data;
    uint32_t hidword = (uint32_t)(data >> 32);
    __asm__ __volatile__ ("wrmsr" : : "c"(idRegister), "a"(lodword), "d"(hidword) : "memory");
}
