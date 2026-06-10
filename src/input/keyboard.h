#ifndef PRISMOS_KEYBOARD_H
#define PRISMOS_KEYBOARD_H

typedef enum {
    KEY_EVENT_NONE = 0,
    KEY_EVENT_CHARACTER,
    KEY_EVENT_ENTER,
    KEY_EVENT_BACKSPACE,
    KEY_EVENT_DELETE,
    KEY_EVENT_LEFT,
    KEY_EVENT_RIGHT,
    KEY_EVENT_UP,
    KEY_EVENT_DOWN,
    KEY_EVENT_HOME,
    KEY_EVENT_END,
} KeyEventType;

typedef struct {
    KeyEventType type;
    char character;
} KeyEvent;

/* Registers the keyboard IRQ handler (IRQ1) and unmasks the IRQ line.
 * Must be called after interrupts_init(). */
void keyboard_init(void);

/* Block until a key event is available, then return it.
 * With interrupt-driven input this uses 'hlt' to sleep between interrupts
 * instead of busy-polling the hardware port. */
KeyEvent keyboard_read_event(void);

/* Return the next queued event if available, otherwise KEY_EVENT_NONE. */
KeyEvent keyboard_poll_event(void);

#endif