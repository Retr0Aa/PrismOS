#ifndef PRISMOS_SYSTEM_H
#define PRISMOS_SYSTEM_H

#include <stdint.h>

// The bootloader writes detected RAM here in kilobytes before entering the kernel.
#define PRISMOS_TOTAL_MEMORY_KB (*((volatile uint32_t*)0x9000))

#endif