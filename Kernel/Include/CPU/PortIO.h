// NOTE: Implemented in PortIO.asm
#ifndef YCH_KERNEL_CPU_PORT_IO_H
#define YCH_KERNEL_CPU_PORT_IO_H

#include "Krnlych.h"

BYTE KrInByteFromPort(WORD wPort);
WORD KrInWordFromPort(WORD wPort);

VOID KrOutByteToPort(WORD wPort, BYTE Byte);
VOID KrOutWordToPort(WORD wPort, WORD Word);

#endif // !YCH_KERNEL_CPU_PORT_IO_H
