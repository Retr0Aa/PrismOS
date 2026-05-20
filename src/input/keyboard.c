#include "keyboard.h"

#include "display/console.h"
#include "platform/io.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

static const char keyboard_map[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
    '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
    '\n', 0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0,
    ' ', 0
};

static const char keyboard_shift_map[128] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',
    '\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',
    '\n', 0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0,
    ' ', 0
};

static int keyboard_data_available(void) {
    return (inb(KEYBOARD_STATUS_PORT) & 1) != 0;
}

static unsigned char keyboard_read_scancode(void) {
    static unsigned int poll_counter = 0;
    while (!keyboard_data_available()) {
        /* Rate-limit console_tick to avoid extremely fast blinking when polling.
         * Use a much larger interval so blinking isn't driven by tight polling loops.
         */
        poll_counter++;
        if ((poll_counter & 0x3FFF) == 0) { /* every 16384 iterations */
            console_tick();
        }
        cpu_relax();
    }
    poll_counter = 0;

    return inb(KEYBOARD_DATA_PORT);
}

KeyEvent keyboard_read_event(void) {
    static int extended_prefix = 0;
    static int shift_pressed = 0;
    static int caps_lock = 0;

    for (;;) {
        const unsigned char scancode = keyboard_read_scancode();

        if (scancode == 0xE0) {
            extended_prefix = 1;
            continue;
        }

        if (scancode & 0x80) {
            const unsigned char key = (unsigned char)(scancode & 0x7F);

            if (key == 42 || key == 54) {
                shift_pressed = 0;
            }

            extended_prefix = 0;
            continue;
        }

        if (extended_prefix) {
            extended_prefix = 0;

            switch (scancode) {
                case 0x48: return (KeyEvent){KEY_EVENT_UP, 0};
                case 0x50: return (KeyEvent){KEY_EVENT_DOWN, 0};
                case 0x4B: return (KeyEvent){KEY_EVENT_LEFT, 0};
                case 0x4D: return (KeyEvent){KEY_EVENT_RIGHT, 0};
                case 0x47: return (KeyEvent){KEY_EVENT_HOME, 0};
                case 0x4F: return (KeyEvent){KEY_EVENT_END, 0};
                case 0x53: return (KeyEvent){KEY_EVENT_DELETE, 0};
                default: continue;
            }
        }

        if (scancode == 42 || scancode == 54) {
            shift_pressed = 1;
            continue;
        }

        if (scancode == 58) {
            caps_lock = !caps_lock;
            continue;
        }

        if (scancode == 14) {
            return (KeyEvent){KEY_EVENT_BACKSPACE, 0};
        }

        if (scancode == 28) {
            return (KeyEvent){KEY_EVENT_ENTER, 0};
        }

        if (scancode >= 128) {
            continue;
        }

        if (keyboard_map[scancode] == 0) {
            continue;
        }

        char character = shift_pressed ? keyboard_shift_map[scancode] : keyboard_map[scancode];

        if (caps_lock && character >= 'a' && character <= 'z') {
            character = (char)(character - 'a' + 'A');
        } else if (caps_lock && character >= 'A' && character <= 'Z') {
            character = (char)(character - 'A' + 'a');
        }

        return (KeyEvent){KEY_EVENT_CHARACTER, character};
    }
}