#include "interrupts.h"
#include "idt.h"
#include "irq.h"
#include "platform/gdt.h"
#include "platform/pic.h"
#include "debug/log.h"

void interrupts_init(void)
{
    DEBUG_LOG("interrupts: installing GDT");
    gdt_init();

    DEBUG_LOG("interrupts: remapping PIC");
    /* IRQ0–7  → vectors 0x20–0x27
     * IRQ8–15 → vectors 0x28–0x2F
     * This moves hardware IRQs above the CPU exception range (0x00–0x1F). */
    pic_remap(0x20, 0x28);

    /* Mask every IRQ line so only drivers that explicitly unmask their line
     * will receive interrupts.  This prevents spurious IRQs from firing
     * before any handler is registered. */
    pic_disable_all();

    DEBUG_LOG("interrupts: installing IDT");
    idt_init();

    DEBUG_LOG("interrupts: initialising IRQ handler table");
    irq_init();

    DEBUG_LOG("interrupts: enabling interrupts");
    __asm__ volatile("sti");
    DEBUG_LOG("interrupts_init: complete - system ready for interrupts");
}
