#include <stddef.h>

void *memset(void *destination, int value, size_t count)
{
    unsigned char *bytes = destination;

    while (count-- > 0) {
        *bytes++ = (unsigned char)value;
    }

    return destination;
}