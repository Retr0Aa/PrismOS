#include <stdint.h>

uint32_t strlen(const char* str)
{
    uint32_t len = 0;

    while (str[len] != '\0')
    {
        len++;
    }

    return len;
}

int strcmp(const char* a, const char* b)
{
    while (*a && *b)
    {
        if (*a != *b)
        {
            return *a - *b;
        }

        a++;
        b++;
    }

    return *a - *b;
}

int strncmp(const char* a, const char* b, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++)
    {
        if (a[i] != b[i])
        {
            return a[i] - b[i];
        }

        if (a[i] == '\0')
        {
            return 0;
        }
    }

    return 0;
}

char* strcpy(char* dest, const char* src)
{
    char* start = dest;

    while (*src)
    {
        *dest++ = *src++;
    }

    *dest = '\0';

    return start;
}

void* memcpy(void* dest, const void* src, uint32_t count)
{
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;

    for (uint32_t i = 0; i < count; i++)
    {
        d[i] = s[i];
    }

    return dest;
}

void* memset(void* dest, int value, uint32_t count)
{
    uint8_t* d = (uint8_t*)dest;

    for (uint32_t i = 0; i < count; i++)
    {
        d[i] = (uint8_t)value;
    }

    return dest;
}
