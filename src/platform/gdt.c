#include "gdt.h"

#include <stdint.h>

/* -------------------------------------------------------------------------
 * GDT entry / pointer types
 * ---------------------------------------------------------------------- */
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity; /* [7:4] flags | [3:0] limit[19:16] */
    uint8_t  base_high;
} __attribute__((packed)) GdtEntry;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) GdtPtr;

/* -------------------------------------------------------------------------
 * The three descriptors we need (null, code, data).
 * Selectors: 0x08 = kernel code, 0x10 = kernel data (matches GRUB default).
 * ---------------------------------------------------------------------- */
static GdtEntry gdt[3];
static GdtPtr   gdtr;

static void gdt_set_entry(int idx, uint32_t base, uint32_t limit,
                           uint8_t access, uint8_t gran_flags)
{
    gdt[idx].base_low   = (uint16_t)(base  & 0xFFFFU);
    gdt[idx].base_mid   = (uint8_t)((base  >> 16) & 0xFFU);
    gdt[idx].base_high  = (uint8_t)((base  >> 24) & 0xFFU);
    gdt[idx].limit_low  = (uint16_t)(limit & 0xFFFFU);
    /* upper nibble of granularity = flags, lower nibble = limit[19:16] */
    gdt[idx].granularity = (uint8_t)(((limit >> 16) & 0x0FU) | (gran_flags & 0xF0U));
    gdt[idx].access = access;
}

void gdt_init(void)
{
    gdtr.limit = (uint16_t)(sizeof(gdt) - 1);
    gdtr.base  = (uint32_t)(uintptr_t)gdt;

    gdt_set_entry(0, 0,          0,           0x00, 0x00); /* Null          */
    gdt_set_entry(1, 0, 0xFFFFFFFFU, 0x9A, 0xCF); /* Kernel code   */
    gdt_set_entry(2, 0, 0xFFFFFFFFU, 0x92, 0xCF); /* Kernel data   */

    /* Load the new GDT and reload all segment registers.
     * The far-return idiom (push cs + eip, then lret) is used to reload %cs
     * because a direct ljmp is not easily expressible in inline AT&T asm. */
    __asm__ volatile(
        "lgdt (%0)          \n\t"
        "movw $0x10, %%ax   \n\t"
        "movw %%ax,  %%ds   \n\t"
        "movw %%ax,  %%es   \n\t"
        "movw %%ax,  %%fs   \n\t"
        "movw %%ax,  %%gs   \n\t"
        "movw %%ax,  %%ss   \n\t"
        "pushl $0x08        \n\t"   /* new CS */
        "pushl $1f          \n\t"   /* new EIP */
        "lret               \n\t"
        "1:                 \n\t"
        :
        : "r"(&gdtr)
        : "eax", "memory"
    );
}
