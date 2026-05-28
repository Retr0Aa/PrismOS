#include "irq.h"
#include "platform/pic.h"
#include "debug/log.h"

#include <stddef.h>

/* 16 slots — one per hardware IRQ line on the 8259 PIC */
#define IRQ_COUNT 16U
#define EXCEPTION_COUNT 32U

static irq_handler_t irq_handlers[IRQ_COUNT];
static exception_handler_t exception_handlers[EXCEPTION_COUNT];

void irq_init(void)
{
    DEBUG_LOG("irq_init: initializing handler table");
    for (unsigned i = 0; i < IRQ_COUNT; i++) {
        irq_handlers[i] = NULL;
    }

    for (unsigned i = 0; i < EXCEPTION_COUNT; i++) {
        exception_handlers[i] = NULL;
    }
}

void irq_register_handler(uint8_t irq, irq_handler_t handler)
{
    if (irq < IRQ_COUNT) {
        DEBUG_LOG("irq_register_handler: registered handler");
        irq_handlers[irq] = handler;
    }
}

void irq_unregister_handler(uint8_t irq)
{
    if (irq < IRQ_COUNT) {
        DEBUG_LOG("irq_unregister_handler: unregistered handler");
        irq_handlers[irq] = NULL;
    }
}

void exception_register_handler(uint8_t vector, exception_handler_t handler)
{
    if (vector < EXCEPTION_COUNT) {
        exception_handlers[vector] = handler;
    }
}

void exception_unregister_handler(uint8_t vector)
{
    if (vector < EXCEPTION_COUNT) {
        exception_handlers[vector] = NULL;
    }
}

/* Called from irq_common_stub in isr_stubs.s.
 * int_no is 32 + IRQ line, so subtract 32 to get the IRQ index. */
void irq_dispatcher(registers_t *regs)
{
    uint8_t irq = (uint8_t)(regs->int_no - 32U);

    if (irq < IRQ_COUNT && irq_handlers[irq] != NULL) {
        irq_handlers[irq](regs);
    }

    /* Always acknowledge the PIC — even for unhandled IRQs */
    pic_send_eoi(irq);
}

/* Called from isr_common_stub for CPU exceptions (vectors 0–31).
 * Minimal handler: log and halt.  Drivers can hook specific exceptions
 * by adding a dispatch table here later (e.g. for #PF, #BP). */
void isr_handler(registers_t *regs)
{
    if (regs->int_no < EXCEPTION_COUNT && exception_handlers[regs->int_no] != NULL) {
        exception_handlers[regs->int_no](regs);
        return;
    }

    ERROR_LOG("unhandled CPU exception");
    /* TODO: per-exception dispatch table for page-fault handler, etc. */
    /* Halt — the kernel cannot safely continue after an unhandled exception */
    __asm__ volatile("cli; hlt");
}
