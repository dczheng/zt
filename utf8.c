#include "zt.h"

struct {
    uint8_t byte, mask;
    uint32_t min, max;
} utf8[] = {
     {0x80, 0xc0,       0,        0},
     {0x00, 0x80,       0,     0x7f},
     {0xc0, 0xe0,    0x80,    0x7ff},
     {0xe0, 0xf0,   0x800,   0xffff},
     {0xf0, 0xf8, 0x10000, 0x10ffff},
};

int
utf8_byte_decode(uint8_t c, uint32_t *v) {
    for (int i = 0; i < 5; i++)
        if ((c & utf8[i].mask) == utf8[i].byte) {
            *v = c & ~utf8[i].mask;
            return i;
        }
    return -1;
}

int
utf8_validate(uint32_t *u, size_t i) {
    if ((*u < utf8[i].min || *u > utf8[i].max) ||
        (*u >= 0xd800 && *u <= 0xdfff))
        return 1;
    return 0;
}

int
utf8_decode(uint8_t *c, int len, uint32_t *u, int *ulen) {
    int i, j;
    uint32_t uu;

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
