#ifndef PRISMOS_RINGBUF_H
#define PRISMOS_RINGBUF_H

#include <stdint.h>

/* Fixed-capacity byte ring buffer (power-of-two size for fast masking).
 * Interrupt-safe for single-producer / single-consumer use: the ISR can
 * push() without disabling interrupts as long as only one thread pops(). */
#define RINGBUF_CAPACITY 256U

typedef struct {
    uint8_t  buf[RINGBUF_CAPACITY];
    uint32_t head; /* next write position */
    uint32_t tail; /* next read  position */
} ringbuf_t;

void    ringbuf_init(ringbuf_t *rb);
int     ringbuf_empty(const ringbuf_t *rb);
int     ringbuf_full(const ringbuf_t *rb);
void    ringbuf_push(ringbuf_t *rb, uint8_t byte);  /* drops byte if full  */
uint8_t ringbuf_pop(ringbuf_t *rb);                 /* UB if empty         */
uint8_t ringbuf_peek(const ringbuf_t *rb);          /* UB if empty         */

#endif /* PRISMOS_RINGBUF_H */
