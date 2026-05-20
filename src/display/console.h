#ifndef PRISMOS_CONSOLE_H
#define PRISMOS_CONSOLE_H

#include <stdint.h>

typedef enum {
    COLOR_BLACK = 0,
    COLOR_BLUE = 1,
    COLOR_GREEN = 2,
    COLOR_CYAN = 3,
    COLOR_RED = 4,
    COLOR_MAGENTA = 5,
    COLOR_BROWN = 6,
    COLOR_LIGHT_GRAY = 7,
    COLOR_DARK_GRAY = 8,
    COLOR_LIGHT_BLUE = 9,
    COLOR_LIGHT_GREEN = 10,
    COLOR_LIGHT_CYAN = 11,
    COLOR_LIGHT_RED = 12,
    COLOR_LIGHT_MAGENTA = 13,
    COLOR_YELLOW = 14,
    COLOR_WHITE = 15,
} VGA_Color;

typedef struct {
    uint64_t address;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t bytes_per_pixel;
    uint8_t red_position;
    uint8_t red_mask_size;
    uint8_t green_position;
    uint8_t green_mask_size;
    uint8_t blue_position;
    uint8_t blue_mask_size;
} FramebufferInfo;

void console_init(const FramebufferInfo* framebuffer);

void console_clear(void);
void console_clear_row(int row);
void console_write_char(char c);
void console_write(const char* str);
void console_writeln(const char* str);
void console_write_uint(unsigned int value);
void console_set_fg_color(VGA_Color color);
void console_set_bg_color(VGA_Color color);
void console_set_color(VGA_Color fg, VGA_Color bg);
void console_set_cursor(int x, int y);
void console_tick(void);
int console_get_x(void);
int console_get_y(void);
uint32_t console_get_framebuffer_width(void);
uint32_t console_get_framebuffer_height(void);

#endif