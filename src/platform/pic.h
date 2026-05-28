#ifndef PRISMOS_PIC_H
#define PRISMOS_PIC_H

#include <stdint.h>

/* Remap the 8259 PIC so IRQ0-7 map to IDT vectors offset1..offset1+7
 * and IRQ8-15 map to offset2..offset2+7.
 * Standard call: pic_remap(0x20, 0x28) */
void pic_remap(uint8_t offset1, uint8_t offset2);

/* Send End-Of-Interrupt to the PIC(s) for the given IRQ line (0-15). */
void pic_send_eoi(uint8_t irq);

/* Mask (disable) / unmask (enable) a single IRQ line. */
void pic_mask_irq(uint8_t irq);
void pic_unmask_irq(uint8_t irq);

/* Mask all IRQ lines on both PICs. */
void pic_disable_all(void);

#endif /* PRISMOS_PIC_H */
