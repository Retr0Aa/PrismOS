#include "debug/log.h"

#include "comport/comport.h"

static int log_ready = 0;
static int log_init_attempted = 0;

static void log_write_unsigned(unsigned int value) {
    char digits[10];
    int count = 0;

    if (value == 0U) {
        comport_write_char('0');
        return;
    }

    while (value > 0U && count < (int)sizeof(digits)) {
        digits[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    while (count > 0) {
        count--;
        comport_write_char(digits[count]);
    }
}

static int log_init(void) {
    // Initialize COM1 once; if it fails, logging stays disabled silently.
    if (!log_init_attempted) {
        log_init_attempted = 1;
        log_ready = (comport_init() == 0) ? 1 : 0;
    }

    return log_ready;
}

void log_write(const char* level, const char* file, int line, const char* message) {
    if (!log_init()) {
        return;
    }

    // Format: [LEVEL] file:line - message
    comport_write_char('[');
    comport_write_string(level);
    comport_write_string("] ");
    comport_write_string(file);
    comport_write_char(':');

    if (line >= 0) {
        log_write_unsigned((unsigned int)line);
    } else {
        comport_write_char('0');
    }

    comport_write_string(" - ");
    comport_write_string(message);
    comport_write_char('\n');
}
