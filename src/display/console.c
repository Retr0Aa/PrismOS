#include <stdint.h>

#include "console.h"
#include "psf_font.h"

#define FONT_WIDTH 8U
#define FONT_HEIGHT 16U
#define MAX_COLUMNS 160U
#define MAX_ROWS 120U

typedef struct {
    uintptr_t address;
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
} FramebufferState;

typedef struct {
    int x;
    int y;
    VGA_Color fg_color;
    VGA_Color bg_color;
    int ready;
    int cursor_visible;
    uint32_t blink_counter;
    FramebufferState framebuffer;
    uint8_t cells[MAX_ROWS][MAX_COLUMNS];
    uint8_t cell_fg[MAX_ROWS][MAX_COLUMNS];
    uint8_t cell_bg[MAX_ROWS][MAX_COLUMNS];
} Console;

static Console console = {0, 0, COLOR_WHITE, COLOR_BLACK, 0, 0, 0, {0}, {{0}}, {{0}}, {{0}}};

static const uint32_t color_palette[16] = {
    0x000000U, 0x0000AAU, 0x00AA00U, 0x00AAAAU,
    0xAA0000U, 0xAA00AAU, 0xAA5500U, 0xAAAAAAU,
    0x555555U, 0x5555FFU, 0x55FF55U, 0x55FFFFU,
    0xFF5555U, 0xFF55FFU, 0xFFFF55U, 0xFFFFFFU,
};

static uint32_t scale_component(uint8_t component, uint8_t mask_size) {
    if (mask_size == 0U) {
        return 0U;
    }

    if (mask_size >= 8U) {
        return (uint32_t)component;
    }

    return ((uint32_t)component * ((1U << mask_size) - 1U) + 127U) / 255U;
}

static uint32_t framebuffer_make_pixel(VGA_Color color) {
    const uint32_t rgb = color_palette[color & 0x0FU];
    const uint8_t red = (uint8_t)((rgb >> 16) & 0xFFU);
    const uint8_t green = (uint8_t)((rgb >> 8) & 0xFFU);
    const uint8_t blue = (uint8_t)(rgb & 0xFFU);

    return (scale_component(red, console.framebuffer.red_mask_size) << console.framebuffer.red_position)
        | (scale_component(green, console.framebuffer.green_mask_size) << console.framebuffer.green_position)
        | (scale_component(blue, console.framebuffer.blue_mask_size) << console.framebuffer.blue_position);
}

static uint8_t* framebuffer_base(void) {
    return (uint8_t*)(uintptr_t)console.framebuffer.address;
}

static uint32_t console_source_font_width(void) {
    return psf_font_get_width();
}

static uint32_t console_source_font_height(void) {
    return psf_font_get_height();
}

static uint32_t console_cell_width(void) {
    return FONT_WIDTH;
}

static uint32_t console_cell_height(void) {
    return FONT_HEIGHT;
}

static uint32_t console_cursor_thickness(void) {
    const uint32_t cell_height = console_cell_height();
    uint32_t thickness = cell_height / 8U;

    if (thickness == 0U) {
        thickness = 1U;
    }

    return thickness;
}

static int console_columns(void) {
    const uint32_t cell_width = console_cell_width();

    if (!console.ready || cell_width == 0U || console.framebuffer.width < cell_width) {
        return 0;
    }

    if (console.framebuffer.width / cell_width > MAX_COLUMNS) {
        return (int)MAX_COLUMNS;
    }

    return (int)(console.framebuffer.width / cell_width);
}

static int console_rows(void) {
    const uint32_t cell_height = console_cell_height();

    if (!console.ready || cell_height == 0U || console.framebuffer.height < cell_height) {
        return 0;
    }

    if (console.framebuffer.height / cell_height > MAX_ROWS) {
        return (int)MAX_ROWS;
    }

    return (int)(console.framebuffer.height / cell_height);
}

static void console_draw_cell(int column, int row);
static void framebuffer_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);

static void console_draw_cursor(void) {
    const uint32_t foreground = framebuffer_make_pixel(console.fg_color);
    const int columns = console_columns();
    const int rows = console_rows();
    const uint32_t blink_period = 60U;
    const int is_cursor_visible = (console.blink_counter / blink_period) % 2 == 0 ? 1 : 0;

    if (!console.ready || !console.cursor_visible || !is_cursor_visible || columns <= 0 || rows <= 0) {
        return;
    }

    if (console.x < 0 || console.x >= columns || console.y < 0 || console.y >= rows) {
        return;
    }

    framebuffer_fill_rect(
        (uint32_t)console.x * console_cell_width(),
        (uint32_t)console.y * console_cell_height() + (console_cell_height() - console_cursor_thickness()),
        console_cell_width(),
        console_cursor_thickness(),
        foreground);
}

static void console_erase_cursor(void) {
    const int columns = console_columns();
    const int rows = console_rows();

    if (!console.ready || !console.cursor_visible || columns <= 0 || rows <= 0) {
        return;
    }

    if (console.x < 0 || console.x >= columns || console.y < 0 || console.y >= rows) {
        return;
    }

    console_draw_cell(console.x, console.y);
}

static void framebuffer_write_pixel(uint32_t x, uint32_t y, uint32_t color) {
    uint8_t* pixel;

    if (!console.ready) {
        return;
    }

    if (x >= console.framebuffer.width || y >= console.framebuffer.height) {
        return;
    }

    pixel = framebuffer_base() + (y * console.framebuffer.pitch) + (x * console.framebuffer.bytes_per_pixel);

    switch (console.framebuffer.bytes_per_pixel) {
        case 4:
            *(uint32_t*)pixel = color;
            break;
        case 3:
            pixel[0] = (uint8_t)(color & 0xFFU);
            pixel[1] = (uint8_t)((color >> 8) & 0xFFU);
            pixel[2] = (uint8_t)((color >> 16) & 0xFFU);
            break;
        case 2:
            *(uint16_t*)pixel = (uint16_t)color;
            break;
        case 1:
            *pixel = (uint8_t)color;
            break;
        default:
            break;
    }
}

static void framebuffer_fill_row(uint32_t y, uint32_t color) {
    uint32_t x;

    if (!console.ready || y >= console.framebuffer.height) {
        return;
    }

    for (x = 0; x < console.framebuffer.width; x++) {
        framebuffer_write_pixel(x, y, color);
    }
}

static void framebuffer_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    uint32_t row;
    uint32_t col;

    if (!console.ready) {
        return;
    }

    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            framebuffer_write_pixel(x + col, y + row, color);
        }
    }
}

static void framebuffer_copy_rows_up(uint32_t pixel_rows) {
    uint32_t y;
    uint8_t* base;

    if (!console.ready || pixel_rows == 0U || pixel_rows >= console.framebuffer.height) {
        return;
    }

    base = framebuffer_base();
    for (y = pixel_rows; y < console.framebuffer.height; y++) {
        uint8_t* destination = base + ((y - pixel_rows) * console.framebuffer.pitch);
        uint8_t* source = base + (y * console.framebuffer.pitch);

        for (uint32_t index = 0; index < console.framebuffer.pitch; index++) {
            destination[index] = source[index];
        }
    }
}

static void console_render_glyph(unsigned char codepoint, int column, int row, VGA_Color fg, VGA_Color bg) {
    const uint8_t* glyph = psf_font_get_glyph(codepoint);
    const uint32_t cell_width = console_cell_width();
    const uint32_t cell_height = console_cell_height();
    const uint32_t font_width = console_source_font_width();
    const uint32_t font_height = console_source_font_height();
    const uint32_t foreground = framebuffer_make_pixel(fg);
    const uint32_t background = framebuffer_make_pixel(bg);
    const uint32_t start_x = (uint32_t)column * cell_width;
    const uint32_t start_y = (uint32_t)row * cell_height;

    if (glyph == 0 || font_width == 0U || font_height == 0U || cell_width == 0U || cell_height == 0U) {
        return;
    }

    for (uint32_t pixel_row = 0; pixel_row < cell_height; pixel_row++) {
        const uint32_t glyph_row = (pixel_row * font_height) / cell_height;
        const uint8_t row_bits = glyph[glyph_row];

        for (uint32_t pixel_column = 0; pixel_column < cell_width; pixel_column++) {
            const uint32_t glyph_column = (pixel_column * font_width) / cell_width;
            const uint32_t color = (row_bits & (1U << glyph_column)) != 0U ? foreground : background;
            const uint32_t pixel_x = start_x + pixel_column;
            const uint32_t pixel_y = start_y + pixel_row;

            framebuffer_write_pixel(pixel_x, pixel_y, color);
        }
    }
}

static void console_draw_cell(int column, int row) {
    unsigned char codepoint;
    VGA_Color fg;
    VGA_Color bg;

    if (!console.ready || column < 0 || row < 0 || column >= console_columns() || row >= console_rows()) {
        return;
    }

    codepoint = console.cells[row][column];
    fg = (VGA_Color)console.cell_fg[row][column];
    bg = (VGA_Color)console.cell_bg[row][column];
    console_render_glyph(codepoint, column, row, fg, bg);
}

static void console_write_cell(int column, int row, unsigned char codepoint, VGA_Color fg, VGA_Color bg) {
    if (!console.ready || column < 0 || row < 0 || column >= console_columns() || row >= console_rows()) {
        return;
    }

    console.cells[row][column] = codepoint;
    console.cell_fg[row][column] = (uint8_t)fg;
    console.cell_bg[row][column] = (uint8_t)bg;
    console_render_glyph(codepoint, column, row, fg, bg);
}

static void console_newline(void) {
    const int rows = console_rows();
    const uint32_t cell_height = console_cell_height();

    console_erase_cursor();
    console.x = 0;
    console.y++;

    if (rows <= 0) {
        return;
    }

    if (console.y >= rows) {
        framebuffer_copy_rows_up(cell_height);
        for (uint32_t row = (uint32_t)console.framebuffer.height - cell_height; row < console.framebuffer.height; row++) {
            framebuffer_fill_row(row, framebuffer_make_pixel(console.bg_color));
        }

        for (int row = 1; row < rows; row++) {
            for (int column = 0; column < console_columns(); column++) {
                console.cells[row - 1][column] = console.cells[row][column];
                console.cell_fg[row - 1][column] = console.cell_fg[row][column];
                console.cell_bg[row - 1][column] = console.cell_bg[row][column];
            }
        }

        for (int column = 0; column < console_columns(); column++) {
            console.cells[rows - 1][column] = ' ';
            console.cell_fg[rows - 1][column] = (uint8_t)console.fg_color;
            console.cell_bg[rows - 1][column] = (uint8_t)console.bg_color;
        }

        console.y = rows - 1;
    }

    console_draw_cursor();
}

void console_init(const FramebufferInfo* framebuffer) {
    if (framebuffer == 0
        || framebuffer->address == 0U
        || console_source_font_width() == 0U
        || console_source_font_height() == 0U
        || framebuffer->width < FONT_WIDTH
        || framebuffer->height < FONT_HEIGHT
        || framebuffer->bytes_per_pixel == 0U) {
        console.ready = 0;
        return;
    }

    console.framebuffer.address = (uintptr_t)framebuffer->address;
    console.framebuffer.width = framebuffer->width;
    console.framebuffer.height = framebuffer->height;
    console.framebuffer.pitch = framebuffer->pitch;
    console.framebuffer.bytes_per_pixel = framebuffer->bytes_per_pixel;
    console.framebuffer.red_position = framebuffer->red_position;
    console.framebuffer.red_mask_size = framebuffer->red_mask_size;
    console.framebuffer.green_position = framebuffer->green_position;
    console.framebuffer.green_mask_size = framebuffer->green_mask_size;
    console.framebuffer.blue_position = framebuffer->blue_position;
    console.framebuffer.blue_mask_size = framebuffer->blue_mask_size;
    console.fg_color = COLOR_WHITE;
    console.bg_color = COLOR_BLACK;
    console.x = 0;
    console.y = 0;
    console.cursor_visible = 1;
    console.ready = 1;

    for (uint32_t row = 0; row < MAX_ROWS; row++) {
        for (uint32_t column = 0; column < MAX_COLUMNS; column++) {
            console.cells[row][column] = ' ';
            console.cell_fg[row][column] = (uint8_t)console.fg_color;
            console.cell_bg[row][column] = (uint8_t)console.bg_color;
        }
    }

    console_clear();
}

void console_clear_row(int row) {
    const uint32_t background = framebuffer_make_pixel(console.bg_color);
    const int columns = console_columns();
    const uint32_t cell_height = console_cell_height();

    if (!console.ready || row < 0 || row >= console_rows()) {
        return;
    }

    console_erase_cursor();

    for (int column = 0; column < columns; column++) {
        console.cells[row][column] = ' ';
        console.cell_fg[row][column] = (uint8_t)console.fg_color;
        console.cell_bg[row][column] = (uint8_t)console.bg_color;
    }

    framebuffer_fill_rect(0U, (uint32_t)row * cell_height, console.framebuffer.width, cell_height, background);
    console_draw_cursor();
}

void console_clear(void) {
    const int columns = console_columns();
    const int rows = console_rows();

    if (console.ready) {
        framebuffer_fill_rect(0U, 0U, console.framebuffer.width, console.framebuffer.height, framebuffer_make_pixel(console.bg_color));
    }

    console.cursor_visible = 1;
    console.x = 0;
    console.y = 0;

    for (int row = 0; row < rows; row++) {
        for (int column = 0; column < columns; column++) {
            console.cells[row][column] = ' ';
            console.cell_fg[row][column] = (uint8_t)console.fg_color;
            console.cell_bg[row][column] = (uint8_t)console.bg_color;
        }
    }

    console_draw_cursor();
}

void console_write_char(char c) {
    const int columns = console_columns();
    const int rows = console_rows();
    unsigned char codepoint;

    if (!console.ready || columns <= 0 || rows <= 0) {
        return;
    }

    if (c == '\n') {
        console_newline();
        return;
    }

    if (c == '\r') {
        return;
    }

    if (c == '\t') {
        console_write_char(' ');
        console_write_char(' ');
        console_write_char(' ');
        console_write_char(' ');
        return;
    }

    if (console.x >= columns) {
        console_newline();
    }

    console_erase_cursor();

    codepoint = (unsigned char)c;

    if (codepoint < 32U || codepoint > 127U) {
        codepoint = '?';
    }

    console_write_cell(console.x, console.y, codepoint, console.fg_color, console.bg_color);
    console.x++;

    if (console.x >= columns) {
        console_newline();
        return;
    }

    console_draw_cursor();
}

void console_write(const char* str) {
    while (*str != '\0') {
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

    if (value == 0U) {
        console_write_char('0');
        return;
    }

    while (value > 0U && index < 10) {
        buffer[index++] = (char)('0' + (value % 10U));
        value /= 10U;
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

void console_set_cursor_visible(int visible) {
    if (!console.ready) {
        return;
    }

    if (visible) {
        console.cursor_visible = 1;
        console_draw_cursor();
    } else {
        console_erase_cursor();
        console.cursor_visible = 0;
    }
}

void console_set_cursor(int x, int y) {
    const int columns = console_columns();
    const int rows = console_rows();

    if (x >= 0 && x < columns && y >= 0 && y < rows) {
        console_erase_cursor();
        console.x = x;
        console.y = y;
        console_draw_cursor();
    }
}

void console_draw_pixel(int x, int y, VGA_Color color) {
    if (!console.ready || x < 0 || y < 0) {
        return;
    }

    framebuffer_write_pixel((uint32_t)x, (uint32_t)y, framebuffer_make_pixel(color));
}

void console_tick(void) {
    if (console.ready) {
        console.blink_counter++;
        
        // Reset counter to prevent overflow and keep blinking stable
        // One full cycle is 2 * blink_period
        if (console.blink_counter >= 120U) {
            console.blink_counter = 0;
        }
        
        // Redraw cursor at new blink state
        console_erase_cursor();
        console_draw_cursor();
    }
}

int console_get_x(void) {
    return console.x;
}

int console_get_y(void) {
    return console.y;
}

uint32_t console_get_framebuffer_width(void) {
    return console.framebuffer.width;
}

uint32_t console_get_framebuffer_height(void) {
    return console.framebuffer.height;
}