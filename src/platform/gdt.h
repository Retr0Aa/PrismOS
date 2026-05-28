#ifndef PRISMOS_GDT_H
#define PRISMOS_GDT_H

/* Initialise a minimal flat GDT (null / kernel-code / kernel-data) and
 * reload all segment registers.  Must be called before idt_init(). */
void gdt_init(void);

#endif /* PRISMOS_GDT_H */
