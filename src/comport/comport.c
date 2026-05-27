#include "comport.h"

#include <stdint.h>

#include "platform/io.h"

#define PORT 0x3f8 

int comport_init(void) {
    outb(PORT + 1, 0x00);    // Disable interrupts
    outb(PORT + 3, 0x80);    // Enable DLAB

    outb(PORT + 0, 0x03);    // Baud divisor low (38400)
    outb(PORT + 1, 0x00);    // Baud divisor high

    outb(PORT + 3, 0x03);    // 8 bits, no parity, 1 stop bit
    outb(PORT + 2, 0xC7);    // Enable FIFO, clear, 14-byte threshold
    outb(PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set

    // Loopback test
    outb(PORT + 4, 0x1E);    // Enable loopback mode
    outb(PORT + 0, 0xAE);    // Send test byte

    if (inb(PORT + 0) != 0xAE) {
        return 1; // failed
    }

    // Normal operation
    outb(PORT + 4, 0x0F);
    return 0;
}

// -------------------------
// Status helpers
// -------------------------
static int serial_received() {
    return inb(PORT + 5) & 1;
}

static int serial_transmit_empty() {
    return inb(PORT + 5) & 0x20;
}

// -------------------------
// Read / Write single byte
// -------------------------
static char serial_read(void) {
    while (!serial_received());
    return inb(PORT);
}

void comport_write_char(char c) {
    while (!serial_transmit_empty());
    outb(PORT, c);
}

// -------------------------
// Write string
// -------------------------
void comport_write_string(const char* str) {
    while (*str) {
        comport_write_char(*str++);
    }
}

// -------------------------
// Read buffer (blocking, fixed length)
// -------------------------
void comport_read_buffer(char* buffer, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        buffer[i] = serial_read();
    }
}