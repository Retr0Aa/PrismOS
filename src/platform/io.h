#ifndef PRISMOS_IO_H
#define PRISMOS_IO_H

#include <stdint.h>

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void cpu_relax(void) {
    __asm__ volatile ("pause");
}

static inline void reboot_system(void) {
    __asm__ volatile ("cli");
    outb(0x64, 0xFE);

    for (;;) {
        __asm__ volatile ("hlt");
    }
}

static inline void shutdown_system(void) {
    __asm__ volatile ("cli");
    outw(0x604, 0x2000);
    outw(0xB004, 0x2000);

    for (;;) {
        __asm__ volatile ("hlt");
    }
}

#endif