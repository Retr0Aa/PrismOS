#include <stdint.h>

#include "display/console.h"
#include "shell/shell.h"

#include <stddef.h>
#include "display/serial.h"

extern unsigned int g_multiboot_magic;
extern unsigned int g_multiboot_info;

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289U

struct multiboot_tag {
    uint32_t type;
    uint32_t size;
} __attribute__((packed));

struct multiboot_tag_framebuffer {
    uint32_t type;
    uint32_t size;
    uint64_t address;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t bpp;
    uint8_t framebuffer_type;
    uint16_t reserved;
    uint8_t red_position;
    uint8_t red_mask_size;
    uint8_t green_position;
    uint8_t green_mask_size;
    uint8_t blue_position;
    uint8_t blue_mask_size;
} __attribute__((packed));

static void kernel_halt(void) {
    while (1) {
    }
}

/* Simple VGA text-mode helpers for early diagnostics when no framebuffer is available. */
static void textmode_putc(char c) {
    volatile uint16_t* buf = (volatile uint16_t*)0xB8000;
    static unsigned int pos = 0;

    if (c == '\n') {
        unsigned int row = pos / 80;
        pos = (row + 1) * 80;
        return;
    }

    buf[pos++] = (uint16_t)((0x07 << 8) | (uint8_t)c);
}

static void textmode_write(const char* s) {
    while (s && *s) {
        textmode_putc(*s++);
    }
}

static void textmode_write_hex(uint32_t value) {
    const char* hex = "0123456789ABCDEF";
    char buf[11];
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 0; i < 8; i++) {
        buf[2 + i] = hex[(value >> ((7 - i) * 4)) & 0xF];
    }
    buf[10] = '\0';
    textmode_write(buf);
}

static const struct multiboot_tag* multiboot2_next_tag(const struct multiboot_tag* tag) {
    uintptr_t next = (uintptr_t)tag + ((tag->size + 7U) & ~7U);
    return (const struct multiboot_tag*)next;
}

void main(void) {
    unsigned int magic = g_multiboot_magic;
    const uint8_t* boot_info = (const uint8_t*)(uintptr_t)g_multiboot_info;
    FramebufferInfo framebuffer = {0};
    int framebuffer_found = 0;

    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        kernel_halt();
    }

    if (boot_info != 0) {
        const uint32_t total_size = *(const uint32_t*)boot_info;
        const struct multiboot_tag* tag = (const struct multiboot_tag*)(boot_info + 8U);
        const struct multiboot_tag* end = (const struct multiboot_tag*)(boot_info + total_size);

        while (tag < end && tag->type != 0U) {
            if (tag->type == 8U) {
                const struct multiboot_tag_framebuffer* framebuffer_tag = (const struct multiboot_tag_framebuffer*)tag;

                /* Accept framebuffer provided by bootloader regardless of type.
                 * For RGB (type 1) we also capture color mask positions.
                 */
                framebuffer.address = framebuffer_tag->address;
                framebuffer.width = framebuffer_tag->width;
                framebuffer.height = framebuffer_tag->height;
                framebuffer.pitch = framebuffer_tag->pitch;
                framebuffer.bytes_per_pixel = (framebuffer_tag->bpp > 0) ? (uint8_t)((framebuffer_tag->bpp + 7U) / 8U) : 1U;

                if (framebuffer_tag->framebuffer_type == 1U) {
                    framebuffer.red_position = framebuffer_tag->red_position;
                    framebuffer.red_mask_size = framebuffer_tag->red_mask_size;
                    framebuffer.green_position = framebuffer_tag->green_position;
                    framebuffer.green_mask_size = framebuffer_tag->green_mask_size;
                    framebuffer.blue_position = framebuffer_tag->blue_position;
                    framebuffer.blue_mask_size = framebuffer_tag->blue_mask_size;
                } else {
                    /* For non-RGB framebuffers, choose a reasonable default mapping. */
                    framebuffer.red_position = 16;
                    framebuffer.red_mask_size = 8;
                    framebuffer.green_position = 8;
                    framebuffer.green_mask_size = 8;
                    framebuffer.blue_position = 0;
                    framebuffer.blue_mask_size = 8;
                }

                framebuffer_found = 1;
            }

            tag = multiboot2_next_tag(tag);
        }
    }

    if (!framebuffer_found) {
        /* Initialize serial early so we have output on UEFI/modern machines */
        serial_init();
        serial_write("No framebuffer found. Using serial fallback for diagnostics.\n");

        if (boot_info == 0) {
            serial_write("multiboot info pointer is NULL\n");
            kernel_halt();
        }

        const uint32_t total_size = *(const uint32_t*)boot_info;
        serial_write("multiboot total_size: ");
        serial_write_hex(total_size);
        serial_write("\n");

        const struct multiboot_tag* tag = (const struct multiboot_tag*)(boot_info + 8U);
        const struct multiboot_tag* end = (const struct multiboot_tag*)(boot_info + total_size);

        while (tag < end && tag->type != 0U) {
            serial_write("tag type: ");
            serial_write_hex(tag->type);
            serial_write("\n");
            tag = multiboot2_next_tag(tag);
        }

        kernel_halt();
    }

    console_init(&framebuffer);

    shell_run();
}

void __cxa_pure_virtual() { }

void _exit(void) { kernel_halt(); }