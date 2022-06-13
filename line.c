#include <stdlib.h>

#include "zt.h"

char *line_buffer;

void
ldirty(int a, int b) {
    LIMIT(a, 0, zt.col);
    LIMIT(b, 0, zt.col);
    for (; a <= b; a++)
        zt.dirty[a] = 1;
}

void
lmemmove(int y, int dst, int src, int n) {
    ASSERT(y >= 0 && y < zt.row, "");
    ASSERT(dst >= 0 && dst < zt.col, "");
    ASSERT(src >= 0 && src < zt.col, "");
    ASSERT(n >= 0, "");
    for (; dst + n >= zt.col; n--)
    for (; src + n >= zt.col; n--)
    if (!n)
        return;
    memmove(&zt.line[y][dst], &zt.line[y][src],
        n * sizeof(struct MyChar));
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
    LIMIT(y, 0, zt.row-1);
    LIMIT(a, 0, zt.col-1);
    LIMIT(b, 0, zt.col-1);
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
ltab(int n) {
    for (; zt.x < zt.col && n > 0; n--)
        for (zt.x++; zt.x < zt.col && !zt.tabs[zt.x]; zt.x++);
    for (; zt.x > 0 && n < 0; n++)
        for (zt.x--; zt.x > 0 && !zt.tabs[zt.x]; zt.x--);
    LIMIT(zt.x, 0, zt.col-1);
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
ldirty_reset(void) {
    memset(zt.dirty, 0, zt.row * sizeof(*zt.dirty));
}

void
ltab_clear(void) {
    memset(zt.tabs, 0, zt.col * sizeof(*zt.tabs));
}

void
ltab_reset(void)  {
    int ts, i;

    ltab_clear();
    ts = MAX(4, TABSPACE);
    for (i = ts; i < zt.col; i += ts)
        zt.tabs[i] = 1;
}

void
lalloc(void) {
    char *p;
    int i, n;

    zt.dirty = malloc(zt.row * sizeof(*zt.dirty));
    ASSERT(zt.dirty != NULL, "");
    ldirty(0, zt.row-1);

    zt.tabs = malloc(zt.col * sizeof(*zt.tabs));
    ASSERT(zt.tabs != NULL, "");
    ltab_reset();

    n = (sizeof(struct MyChar*) +
        sizeof(struct MyChar) * zt.col) * zt.row;
    line_buffer = malloc(n);
    ASSERT(line_buffer != NULL, "");

    zt.line = (struct MyChar **)line_buffer;
    p = line_buffer + sizeof(struct MyChar*) * zt.row;
    for (i=0; i<zt.row; i++) {
        zt.line[i] = (struct MyChar*)p;
        p += sizeof(struct MyChar) * zt.col;
    }
    lclear(0, 0, zt.row-1, zt.col-1);

}

void
linit(void) {
    lalloc();
    zt.top = 0;
    zt.bot = zt.row-1;
    zt.x = zt.y = 0;
    ATTR_RESET(zt.attr);
}

void 
lresize(void) {
    int i, j, mr, mc;
    char *line_buffer_old = line_buffer;
    struct MyChar **line_old = zt.line;
    int *tabs_old = zt.tabs;

    mr = MIN(zt.row, zt.row_old);
    mc = MIN(zt.col, zt.col_old);

    free(zt.dirty);
    lalloc();

    for (i = 0; i < mr; i++)
        for (j = 0; j < mc; j++)
            zt.line[i][j] = line_old[i][j];
    free(line_buffer_old);

    for (i = 0; i < mc; i++) 
        zt.tabs[i] = tabs_old[i];
    free(tabs_old);

    LIMIT(zt.x, 0, zt.col-1);
    LIMIT(zt.y, 0, zt.row-1);
    zt.bot = zt.row_old - zt.bot + zt.row;
    LIMIT(zt.top, 0, zt.row-1);
    LIMIT(zt.bot, 0, zt.row-1);
}

