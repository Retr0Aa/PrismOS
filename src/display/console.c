#include <stdint.h>

#include "console.h"
#include "platform/io.h"

#define VGA_MEMORY ((volatile uint8_t*)0xB8000)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

typedef struct {
    int x;
    int y;
    VGA_Color fg_color;
    VGA_Color bg_color;
} Console;

static Console console = {0, 0, COLOR_WHITE, COLOR_BLACK};

static inline uint8_t make_color_attr(VGA_Color fg, VGA_Color bg) {
    return (uint8_t)((bg << 4) | fg);
}

static inline int get_offset(int x, int y) {
    return (y * VGA_WIDTH + x) * 2;
}

// VGA uses a hardware cursor, so the visible caret needs to be synchronized explicitly.
static void console_update_hardware_cursor(void) {
    uint16_t position = (uint16_t)(console.y * VGA_WIDTH + console.x);

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(position & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((position >> 8) & 0xFF));
}

static void console_scroll(void) {
    const uint8_t attribute = make_color_attr(console.fg_color, console.bg_color);

    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            const int from = get_offset(x, y);
            const int to = get_offset(x, y - 1);
            VGA_MEMORY[to] = VGA_MEMORY[from];
            VGA_MEMORY[to + 1] = VGA_MEMORY[from + 1];
        }
    }

    for (int x = 0; x < VGA_WIDTH; x++) {
        const int offset = get_offset(x, VGA_HEIGHT - 1);
        VGA_MEMORY[offset] = ' ';
        VGA_MEMORY[offset + 1] = attribute;
    }

    if (console.y > 0) {
        console.y--;
    }
}

static void console_newline(void) {
    console.x = 0;
    console.y++;
    if (console.y >= VGA_HEIGHT) {
        console_scroll();
        console.y = VGA_HEIGHT - 1;
    }
}

void console_clear_row(int row) {
    const uint8_t attribute = make_color_attr(console.fg_color, console.bg_color);

    if (row < 0 || row >= VGA_HEIGHT) {
        return;
    }

    for (int x = 0; x < VGA_WIDTH; x++) {
        const int offset = get_offset(x, row);
        VGA_MEMORY[offset] = ' ';
        VGA_MEMORY[offset + 1] = attribute;
    }
}

void console_clear(void) {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        console_clear_row(y);
    }

    console.x = 0;
    console.y = 0;
    console_update_hardware_cursor();
}

void console_write_char(char c) {
    const uint8_t attribute = make_color_attr(console.fg_color, console.bg_color);

    if (c == '\n') {
        console_newline();
        console_update_hardware_cursor();
        return;
    }

    if (console.x >= VGA_WIDTH) {
        console_newline();
    }

    const int offset = get_offset(console.x, console.y);
    VGA_MEMORY[offset] = (uint8_t)c;
    VGA_MEMORY[offset + 1] = attribute;

    console.x++;
    if (console.x >= VGA_WIDTH) {
        console_newline();
    }

    console_update_hardware_cursor();
}

void console_write(const char* str) {
    while (*str) {
        console_write_char(*str++);
    }
}

void console_writeln(const char* str) {
    console_write(str);
    console_write_char('\n');
}

void console_write_uint(unsigned int value) {
    char buffer[11];
    int index = 0;

    if (value == 0) {
        console_write_char('0');
        return;
    }

    while (value > 0 && index < 10) {
        buffer[index++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (index > 0) {
        console_write_char(buffer[--index]);
    }
}

void console_set_fg_color(VGA_Color color) {
    console.fg_color = color;
}

void console_set_bg_color(VGA_Color color) {
    console.bg_color = color;
}

void console_set_color(VGA_Color fg, VGA_Color bg) {
    console.fg_color = fg;
    console.bg_color = bg;
}

void console_set_cursor(int x, int y) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        console.x = x;
        console.y = y;
        console_update_hardware_cursor();
    }
}

int console_get_x(void) {
    return console.x;
}

int console_get_y(void) {
    return console.y;
}