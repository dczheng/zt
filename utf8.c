#include <stdio.h>
#include <stdint.h>

#include "line.h"

struct {
    unsigned char byte, mask;
    MyRune min, max;
} utf8[] = {
     {0x80, 0xC0,       0,        0},
     {0x00, 0x80,       0,     0x7F},
     {0xC0, 0xE0,    0x80,    0x7FF},
     {0xE0, 0xF0,   0x800,   0xFFFF},
     {0xF0, 0xF8, 0x10000, 0x10FFFF},
};

int
utf8_byte_decode(unsigned char c, MyRune *v) {
    for (int i = 0; i < 5; i++)
        if ((c & utf8[i].mask) == utf8[i].byte) {
            *v = c & ~utf8[i].mask;
            return i;
        }
    return -1;
}

int
utf8_validate(MyRune *u, size_t i) {
    if ((*u < utf8[i].min || *u > utf8[i].max) ||
        (*u >= 0xD800 && *u <= 0xDFFF))
        return 1;
    return 0;
}

int
utf8_decode(unsigned char *c, int len, MyRune *u, int *ulen) {
    int i, j;
    MyRune uu;

    if (!len)
        return 1;

    if ((*ulen = utf8_byte_decode(c[0], u)) <= 0)
        return 1;

    for (i = 1, j = 1; i < len && j < *ulen; i++, j++) {
        if (utf8_byte_decode(c[i], &uu) != 0)
            return 1;
        *u = (*u << 6) | uu;
    }

    if (j < *ulen)
        return 1;

    return utf8_validate(u, *ulen);
}
