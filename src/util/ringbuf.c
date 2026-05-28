#include "ringbuf.h"

#define MASK (RINGBUF_CAPACITY - 1U)

void ringbuf_init(ringbuf_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
}

int ringbuf_empty(const ringbuf_t *rb)
{
    return rb->head == rb->tail;
}

int ringbuf_full(const ringbuf_t *rb)
{
    return ((rb->head + 1U) & MASK) == rb->tail;
}

void ringbuf_push(ringbuf_t *rb, uint8_t byte)
{
    if (ringbuf_full(rb)) {
        return; /* drop on overflow */
    }
    rb->buf[rb->head & MASK] = byte;
    rb->head = (rb->head + 1U) & MASK;
}

uint8_t ringbuf_pop(ringbuf_t *rb)
{
    uint8_t byte = rb->buf[rb->tail & MASK];
    rb->tail = (rb->tail + 1U) & MASK;
    return byte;
}

uint8_t ringbuf_peek(const ringbuf_t *rb)
{
    return rb->buf[rb->tail & MASK];
}
