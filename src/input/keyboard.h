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

KeyEvent keyboard_read_event(void);

#endif