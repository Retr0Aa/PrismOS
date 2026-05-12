#ifndef PRISMOS_CONSOLE_H
#define PRISMOS_CONSOLE_H

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
int console_get_x(void);
int console_get_y(void);

#endif