#include "display/serial.h"
#include "platform/io.h"

#define SERIAL_PORT 0x3F8

void serial_init(void) {
    /* Disable interrupts */
    outb(SERIAL_PORT + 1, 0x00);
    /* Enable DLAB */
    outb(SERIAL_PORT + 3, 0x80);
    /* Set baud divisor to 115200 (assuming 115200 base) */
    outb(SERIAL_PORT + 0, 0x01);
    outb(SERIAL_PORT + 1, 0x00);
    /* 8 bits, no parity, one stop bit */
    outb(SERIAL_PORT + 3, 0x03);
    /* Enable FIFO, clear them */
    outb(SERIAL_PORT + 2, 0xC7);
    /* IRQs enabled, RTS/DSR set */
    outb(SERIAL_PORT + 4, 0x0B);
}

static int serial_can_write(void) {
    return (inb(SERIAL_PORT + 5) & 0x20) != 0;
}

void serial_write_char(char c) {
    while (!serial_can_write()) {
        __asm__ volatile ("pause");
    }
    outb(SERIAL_PORT + 0, (uint8_t)c);
}

void serial_write(const char* s) {
    while (s && *s) {
        if (*s == '\n') {
            serial_write_char('\r');
        }
        serial_write_char(*s++);
    }
}

void serial_write_hex(uint32_t value) {
    const char* hex = "0123456789ABCDEF";
    char buf[11];
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 0; i < 8; i++) {
        buf[2 + i] = hex[(value >> ((7 - i) * 4)) & 0xF];
    }
    buf[10] = '\0';
    serial_write(buf);
}

