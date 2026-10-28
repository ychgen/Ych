// NOTE: Implemented in PortIO.asm
#ifndef YCH_KERNEL_CPU_PORT_IO_H
#define YCH_KERNEL_CPU_PORT_IO_H

#include "Krnlych.h"

BYTE ByteInputFromPort(WORD wPort);
WORD WordInputFromPort(WORD wPort);
DWORD DwordInputFromPort(WORD wPort);

VOID ByteOutputToPort(WORD wPort, BYTE Byte);
VOID WordOutputToPort(WORD wPort, WORD Word);
VOID DwordOutputToPort(WORD wPort, DWORD Doubleword);

#endif // !YCH_KERNEL_CPU_PORT_IO_H
