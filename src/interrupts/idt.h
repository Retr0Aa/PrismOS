#ifndef PRISMOS_IDT_H
#define PRISMOS_IDT_H

#include <stdint.h>

/* -------------------------------------------------------------------------
 * x86 IDT gate descriptor (64-bit / 8 bytes).
 * ---------------------------------------------------------------------- */
typedef struct {
    uint16_t base_low;   /* handler address [15:0]  */
    uint16_t selector;   /* code segment selector   */
    uint8_t  always0;    /* must be 0               */
    uint8_t  flags;      /* gate type + DPL + P bit */
    uint16_t base_high;  /* handler address [31:16] */
} __attribute__((packed)) IdtEntry;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) IdtPtr;

/* Initialise and install the IDT (all 256 entries).
 * Installs stubs for CPU exceptions 0–31 and hardware IRQs 0x20–0x2F. */
void idt_init(void);

/* Set a single IDT gate.
 *   flags = 0x8E  ->  32-bit interrupt gate, DPL=0, present */
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

#endif /* PRISMOS_IDT_H */
