/* VGA Color Codes */
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

/* Console structure */
typedef struct {
    int x;
    int y;
    VGA_Color fg_color;
    VGA_Color bg_color;
} Console;

Console console = {0, 0, COLOR_WHITE, COLOR_BLACK};

#define VGA_MEMORY ((volatile char*)0xB8000)
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

/* Combine foreground and background colors into attribute byte */
static inline char make_color_attr(VGA_Color fg, VGA_Color bg) {
    return (bg << 4) | fg;
}

/* Get video memory offset for position */
static inline int get_offset(int x, int y) {
    return (y * VGA_WIDTH + x) * 2;
}

/* Clear the screen */
void console_clear(void) {
    char attr = make_color_attr(console.fg_color, console.bg_color);
    
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT * 2; i += 2) {
        VGA_MEMORY[i] = ' ';
        VGA_MEMORY[i + 1] = attr;
    }
    
    console.x = 0;
    console.y = 0;
}

/* Write a single character at current position */
void console_write_char(char c) {
    char attr = make_color_attr(console.fg_color, console.bg_color);
    int offset = get_offset(console.x, console.y);
    
    if (c == '\n') {
        console.x = 0;
        console.y++;
        if (console.y >= VGA_HEIGHT) {
            console.y = VGA_HEIGHT - 1;
        }
        return;
    }
    
    VGA_MEMORY[offset] = c;
    VGA_MEMORY[offset + 1] = attr;
    
    console.x++;
    if (console.x >= VGA_WIDTH) {
        console.x = 0;
        console.y++;
        if (console.y >= VGA_HEIGHT) {
            console.y = VGA_HEIGHT - 1;
        }
    }
}

/* Write a string */
void console_write(const char* str) {
    while (*str) {
        console_write_char(*str++);
    }
}

/* Write a string with newline */
void console_writeln(const char* str) {
    console_write(str);
    console_write_char('\n');
}

/* Set foreground color */
void console_set_fg_color(VGA_Color color) {
    console.fg_color = color;
}

/* Set background color */
void console_set_bg_color(VGA_Color color) {
    console.bg_color = color;
}

/* Set both foreground and background colors */
void console_set_color(VGA_Color fg, VGA_Color bg) {
    console.fg_color = fg;
    console.bg_color = bg;
}

/* Set cursor position */
void console_set_cursor(int x, int y) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        console.x = x;
        console.y = y;
    }
}

/* Get current X position */
int console_get_x(void) {
    return console.x;
}

/* Get current Y position */
int console_get_y(void) {
    return console.y;
}

/* Kernel entry point */
void main(void) {
    console_clear();
    
    console_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    console_writeln("TestOS Bootloader Loaded!");
    
    console_set_color(COLOR_CYAN, COLOR_BLACK);
    console_writeln("Protected Mode Activated");
    
    console_set_color(COLOR_YELLOW, COLOR_BLACK);
    console_write("Status: ");
    console_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    console_writeln("OK");
    
    console_set_color(COLOR_WHITE, COLOR_BLACK);
    console_writeln("");
    console_writeln("Hello World!");
    
    while (1) {
        __asm__("hlt");
    }
}

/* GCC requires these symbols for 32-bit bare metal */
void __cxa_pure_virtual() { }
void _exit() { while(1); }

