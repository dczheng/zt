#include "zt.h"
#if defined(__linux)
#define  __USE_XOPEN
#endif
#include <wchar.h>

#define YLIMIT(y) LIMIT(y, 0, zt.row-1)
#define XLIMIT(x) LIMIT(x, 0, zt.col-1)

struct {
    uint8_t byte, mask;
    uint32_t min, max;
} _utf8[] = {
     {0x80, 0xc0,       0,        0},
     {0x00, 0x80,       0,     0x7f},
     {0xc0, 0xe0,    0x80,    0x7ff},
     {0xe0, 0xf0,   0x800,   0xffff},
     {0xf0, 0xf8, 0x10000, 0x10ffff},
};

int
utf8_byte_decode(uint8_t c, uint32_t *v) {
    for (int i = 0; i < 5; i++)
        if ((c & _utf8[i].mask) == _utf8[i].byte) {
            *v = c & ~_utf8[i].mask;
            return i;
        }
    return -1;
}

int
utf8_validate(uint32_t *u, size_t i) {
    if ((*u < _utf8[i].min || *u > _utf8[i].max) ||
        (*u >= 0xd800 && *u <= 0xdfff))
        return EINVAL;
    return 0;
}

int
utf8_decode(void *buf, int len, uint32_t *u, int *ulen) {
    int i, j;
    uint32_t uu;
    uint8_t *c = buf;

    if (!len)
        return EINVAL;

    if ((*ulen = utf8_byte_decode(c[0], u)) <= 0)
        return EINVAL;

    for (i = 1, j = 1; i < len && j < *ulen; i++, j++) {
        if (utf8_byte_decode(c[i], &uu) != 0)
            return EINVAL;
        *u = (*u << 6) | uu;
    }

    if (j < *ulen)
        return EINVAL;

    return utf8_validate(u, *ulen);
}

void
ldirty(int a, int b) {
    ASSERT(a >= 0);
    ASSERT(b >= 0);
    if (a >= zt.col)
        return;
    XLIMIT(b);
    for (; a <= b; a++)
        zt.dirty[a] = 1;
}

void
ldirty_all(void) {
    ldirty(0, zt.row-1);
}

void
lmove(int y, int dst, int src, int n) {
    ASSERT(y >= 0 && y < zt.row);
    ASSERT(dst >= 0);
    ASSERT(src >= 0);
    ASSERT(n >= 0);
    if (dst >= zt.col || src >= zt.col)
        return;
    LIMIT(n, 0, zt.col-dst);
    LIMIT(n, 0, zt.col-src);
    if (n == 0)
        return;
    memmove(&zt.line[y][dst], &zt.line[y][src],
        n * sizeof(struct char_t));
}

void
lerase(int y, int a, int b) {
    ASSERT(y >= 0 && y < zt.row);
    ASSERT(a >= 0);
    ASSERT(b >= 0);
    if (a >= zt.col)
        return;
    XLIMIT(b);
    for (; a <= b && a < zt.col; a++)
        zt.line[y][a] = zt.c;
    ldirty(y, y);
}

void
ldelete_char(int n) {
    LIMIT(n, 0, zt.col - zt.x);
    lmove(zt.y, zt.x, zt.x+n, zt.col-zt.x-n);
    lerase(zt.y, zt.col-n, zt.col-1);
}

void
linsert_blank(int n) {
    LIMIT(n, 0, zt.col - zt.x);
    lmove(zt.y, zt.x+n, zt.x, zt.col-zt.x-n);
    lerase(zt.y, zt.x, zt.x+n-1);
}

void
lclear(int y1, int x1, int y2, int x2) {
    if (y1 > y2)
        return;
    lerase(y1, x1, zt.col-1);
    lerase(y2, 0, x2);
    for (y1++; y1 < y2; y1++)
        lerase(y1, 0, zt.col-1);
}

void
lclear_all(void) {
    lclear(0, 0, zt.row-1, zt.col-1);
}

void
lmoveto(int y, int x) {
    zt.y = y;
    zt.x = x;
    XLIMIT(zt.x);
    YLIMIT(zt.y);
}

void
lsettb(int t, int b) {
    zt.top = t;
    zt.bot = b;
    lmoveto(0, 0);
    YLIMIT(zt.top);
    YLIMIT(zt.bot);
    ASSERT(zt.top <= zt.bot);
}

void
lscroll_up(int y, int n) {
    if (n <= 0)
        return;
    n = MIN(n, zt.bot-y+1);
    for (int i = y; i <= zt.bot-n; i++)
        SWAP(zt.line[i], zt.line[i+n]);
    ldirty(y, zt.bot);
    lclear(zt.bot-n+1, 0, zt.bot, zt.col-1);
}

void
lscroll_down(int y, int n) {
    if (n <= 0)
        return;
    n = MIN(n, zt.bot-y+1);
    for (int i = zt.bot; i >= y+n; i--)
        SWAP(zt.line[i], zt.line[i-n]);
    ldirty(y, zt.bot);
    lclear(y, 0, y+n-1, zt.col-1);
}

void
lnew(void) {
    if (zt.y == zt.bot) {
        lscroll_up(zt.top, 1);
        return;
    }
    zt.y++;
    ASSERT(zt.y >= 0 && zt.y < zt.row);
    //YLIMIT(zt.y);
}

void
ltab(int n) {
    for (; zt.x < zt.col && n > 0; n--)
        for (zt.x++; zt.x < zt.col && !zt.tabs[zt.x]; zt.x++);
    for (; zt.x > 0 && n < 0; n++)
        for (zt.x--; zt.x > 0 && !zt.tabs[zt.x]; zt.x--);
    XLIMIT(zt.x);
}

void
_lwrite(uint32_t c) {
    int w;

    if ((w = wcwidth(c)) <= 0) {
        LOG("can't find character width for %u\n", c);
        w = 1;
    }

    if (zt.col - zt.x < w) {
        lnew();
        zt.x = 0;
    }
    zt.line[zt.y][zt.x] = zt.c;
    zt.line[zt.y][zt.x].c = c;
    zt.line[zt.y][zt.x].width = w;
    zt.lastc = zt.line[zt.y][zt.x];

    ldirty(zt.y, zt.y);
    for (w--, zt.x++; w > 0; zt.x++, w--)
        zt.line[zt.y][zt.x] = zt.c;
}

int
lwrite(uint8_t *buf, int len, int *wlen) {
    int ret = 0;
    uint32_t c;

    *wlen = 0;
    if (len <= 0) return 0;
    if (!(ret = utf8_decode(buf, len, &c, wlen)))
        _lwrite(c);
    return ret;
}

void
linsert(int n) {
    if (zt.y >= zt.top && zt.y <= zt.bot)
        lscroll_down(zt.y, n);
}

void
ldelete(int n) {
    if (zt.y >= zt.top && zt.y <= zt.bot)
        lscroll_up(zt.y, n);
}

void
ldirty_reset(void) {
    memset(zt.dirty, 0, zt.row * sizeof(*zt.dirty));
}

void
ltab_clear(void) {
    memset(zt.tabs, 0, zt.col * sizeof(*zt.tabs));
}

void
ltab_reset(void)  {
    ltab_clear();
    for (int i = 8; i < zt.col; i += 8)
        zt.tabs[i] = 1;
}

void
lrepeat_last(int n) {
    while (zt.lastc.c && n--)
        _lwrite(zt.lastc.c);
}

void
lcursor(int save) {
    if (save) {
        zt.x_saved = zt.x;
        zt.y_saved = zt.y;
    } else
        lmoveto(zt.y_saved, zt.x_saved);
}

void
lalt(int alt) {
    zt.line = (alt ? zt.alt.line : zt.normal.line);
}

void
lfree(void) {
    free(zt.normal.buffer);
    free(zt.alt.buffer);
    free(zt.dirty);
}

void
lalloc(void) {
    char *p;
    int i, n;

    ASSERT(zt.dirty = malloc(zt.row * sizeof(*zt.dirty)));
    ldirty_all();

    ASSERT(zt.tabs = malloc(zt.col * sizeof(*zt.tabs)));
    ltab_reset();

    n = (sizeof(struct char_t*) +
        sizeof(struct char_t) * zt.col) * zt.row;

#define X(x) \
    ASSERT(zt.x.buffer = malloc(n)); \
    zt.x.line = (struct char_t **)zt.x.buffer; \
    p = zt.x.buffer + sizeof(struct char_t*) * zt.row; \
    for (i = 0; i < zt.row; i++) { \
        zt.x.line[i] = (struct char_t*)p; \
        p += sizeof(struct char_t) * zt.col; \
    }  \
    zt.line = zt.x.line; \
    lclear_all();

    X(alt);
    X(normal);
#undef X
}

void
lresize(void) {
    int i, j, mr, mc;
    char *normal_buffer_old = zt.normal.buffer;
    char *alt_buffer_old = zt.alt.buffer;
    struct char_t **normal_line_old = zt.normal.line;
    struct char_t **alt_line_old = zt.alt.line;
    int *tabs_old = zt.tabs;

    mr = MIN(zt.row, zt.row_old);
    mc = MIN(zt.col, zt.col_old);

    free(zt.dirty);
    lalloc();

    for (i = 0; i < mr; i++)
        for (j = 0; j < mc; j++) {
            zt.normal.line[i][j] = normal_line_old[i][j];
            zt.alt.line[i][j] = alt_line_old[i][j];
        }
    free(normal_buffer_old);
    free(alt_buffer_old);

    for (i = 0; i < mc; i++)
        zt.tabs[i] = tabs_old[i];
    free(tabs_old);

    XLIMIT(zt.x);
    YLIMIT(zt.y);
    zt.bot = zt.row_old - zt.bot + zt.row;
    YLIMIT(zt.top);
    YLIMIT(zt.bot);
}
