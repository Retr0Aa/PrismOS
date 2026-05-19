#include "shell/shell.h"

extern unsigned int g_multiboot_magic;
extern unsigned int g_multiboot_info;

struct multiboot_info {
    unsigned int flags;
    unsigned int mem_lower;
    unsigned int mem_upper;
    unsigned int boot_device;
    unsigned int cmdline;
    unsigned int mods_count;
    unsigned int mods_addr;
    unsigned int syms_num;
    unsigned int syms_size;
    unsigned int syms_addr;
    unsigned int syms_shndx;
    unsigned int mmap_length;
    unsigned int mmap_addr;
} __attribute__((packed));

void main(void) {
    unsigned int magic = g_multiboot_magic;
    struct multiboot_info* mbi = (struct multiboot_info*)g_multiboot_info;

    if (magic != 0x2BADB002) {
        while (1) {
        }
    }

    (void)mbi;

    /* TODO: Later, parse mbi->flags and use memory info from mbi->mem_lower/mem_upper */

    shell_run();
}

void __cxa_pure_virtual() { }

void _exit(void) { while (1); }