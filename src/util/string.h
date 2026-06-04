#ifndef STRING_H
#define STRING_H

#include <stdint.h>

uint32_t strlen(const char* str);

int strcmp(const char* a, const char* b);
int strncmp(const char* a, const char* b, uint32_t n);

char* strcpy(char* dest, const char* src);

void* memcpy(void* dest, const void* src, uint32_t count);
void* memset(void* dest, int value, uint32_t count);

#endif