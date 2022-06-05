#include <stdlib.h>

#include "zt.h"

#define RASSERT(r) \
    ASSERT(r >= 0 && r < zt.row, "r: %d, row: %d", r, zt.row)
#define CASSERT(c) \
    ASSERT(c >= 0 && c < zt.col, "c: %d, col: %d", c, zt.col)

char *line_buffer;

void
ldirty(int a, int b) {
    CASSERT(a);
    CASSERT(b);
    for (; a <= b; a++)
        zt.dirty[a] = 1;
}
void
lsettb(int t, int b) {
    zt.top = t;
    zt.bot = b;
    LIMIT(zt.top, 0, zt.row-1);
    LIMIT(zt.bot, 0, zt.row-1);
    ASSERT(zt.top < zt.bot, "top: %d, bot: %d", zt.top, zt.bot);
}

void
lerase(int y, int a, int b) {
    RASSERT(y);
    CASSERT(a);
    CASSERT(b);
    for (; a<=b; a++) {
        zt.line[y][a].c = ' ';
        zt.line[y][a].attr = zt.attr;
    }
    ldirty(y, y);
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
lclean(void) {
    printf("free line buffer\n");
    free(line_buffer);
    printf("free dirty\n");
    free(zt.dirty);
}

void
lscroll_up(int y, int n) {
    int i;

    if (n <= 0)
        return;
    n = MIN(n, zt.bot-y+1);
    for (i = y; i <= zt.bot-n; i++)
        SWAP(zt.line[i], zt.line[i+n]);
    ldirty(y, zt.bot);
    lclear(zt.bot-n+1, 0, zt.bot, zt.col-1);
    if (zt.y >= y && zt.y <= zt.bot)
        zt.y = MAX(zt.y-n, y);
}

void
lscroll_down(int y, int n) {
    int i;

    if (n <= 0)
        return;
    n = MIN(n, zt.bot-y+1);
    for (i = zt.bot; i >= y+n; i--)
        SWAP(zt.line[i], zt.line[i-n]);
    ldirty(y, zt.bot);
    lclear(y, 0, y+n-1, zt.col-1);
    if (zt.y >= y && zt.y <= zt.bot)
        zt.y = MIN(zt.y+n, zt.bot);
}

void
lnew(void) {
    if (zt.y == zt.bot)
        lscroll_up(zt.top, 1);
    zt.y++;
}

void
ltab(void) {
    for (int i=0; i<4 && zt.x < zt.col; i++)
        zt.line[zt.y][zt.x].c = ' ';
}

void
lwrite(uint32_t c) {
    if (zt.x >= zt.col) {
        lnew();
        zt.x = 0;
    }
    if (zt.line[zt.y][zt.x].c != c ||
        !(ATTR_EQUAL(zt.line[zt.y][zt.x].attr, zt.attr))) {
            zt.line[zt.y][zt.x].c = c;
            zt.line[zt.y][zt.x].attr = zt.attr;
            ldirty(zt.y, zt.y);
    }
    zt.x++;
}

void
linsert(int n) {
    int y = zt.y;
    lscroll_down(zt.y, n);
    zt.y = y;
}

void
ldelete(int n) {
    int y = zt.y;
    lscroll_up(zt.y, n);
    zt.y = y;
}

void
lmove(int y, int x) {
    zt.y = y;
    zt.x = x;
    LIMIT(zt.y, 0, zt.row-1);
    LIMIT(zt.x, 0, zt.col-1);
}

void
linit(void) {
    char *p;
    int i, n;

    n = (sizeof(struct MyChar*) +
        sizeof(struct MyChar) * zt.col) * zt.row;
    line_buffer = malloc(n);
    ASSERT(line_buffer != NULL, "");
    printf("allocate %s for line buffer\n", to_bytes(n));

    zt.dirty = malloc(zt.row);
    ASSERT(zt.dirty != NULL, "");
    printf("allocate %s for dirty\n", to_bytes(zt.row));

    zt.line = (struct MyChar **)line_buffer;
    p = line_buffer + sizeof(struct MyChar*) * zt.row;
    for (i=0; i<zt.row; i++) {
        zt.line[i] = (struct MyChar*)p;
        p += sizeof(struct MyChar) * zt.col;
    }
    lclear(0, 0, zt.row-1, zt.col-1);
    zt.top = 0;
    zt.bot = zt.row-1;
    zt.x = zt.y = 0;
    ldirty(0, zt.row-1);
    ATTR_RESET(zt.attr);
}
