#ifndef PRISMOS_COMPORT_H
#define PRISMOS_COMPORT_H

#include <stdint.h>

int comport_init(void);
void comport_write_char(char c);
void comport_write_string(const char* str);
void comport_read_buffer(char* buffer, uint32_t len);

#endif