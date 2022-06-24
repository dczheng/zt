#include <stdlib.h>
#if defined(__linux)
#define  __USE_XOPEN
#endif
#include <wchar.h>

#include "zt.h"

char *line_buffer;

#define PASSERT(x) \
    ASSERT(x >= 0, #x": %d", x)
#define YASSERT(y) \
    ASSERT(y >= 0 && y < zt.row, #y": %d, row: %d", y, zt.row)

void
ldirty(int a, int b) {
    PASSERT(a);
    PASSERT(b);
    if (a >= zt.col)
        return;
    LIMIT(b, 0, zt.col-1);
    for (; a <= b; a++)
        zt.dirty[a] = 1;
}

void
lmove(int y, int dst, int src, int n) {
    YASSERT(y);
    PASSERT(dst);
    PASSERT(src);
    PASSERT(n);
    if (dst >= zt.col || src >= zt.col)
        return;
    LIMIT(n, 0, zt.col-dst);
    LIMIT(n, 0, zt.col-src);
    if (n == 0)
        return;
    memmove(&zt.line[y][dst], &zt.line[y][src],
        n * sizeof(struct MyChar));
}

void
lerase(int y, int a, int b) {
    YASSERT(y);
    PASSERT(a);
    PASSERT(b);
    if (a >= zt.col)
        return;
    LIMIT(b, 0, zt.col-1);
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
lmoveto(int y, int x) {
    zt.y = y;
    zt.x = x;
    LIMIT(zt.y, 0, zt.row-1);
    LIMIT(zt.x, 0, zt.col-1);
}

void
lsettb(int t, int b) {
    zt.top = t;
    zt.bot = b;
    lmoveto(0, 0);
    LIMIT(zt.top, 0, zt.row-1);
    LIMIT(zt.bot, 0, zt.row-1);
    ASSERT(zt.top < zt.bot, "top: %d, bot: %d", zt.top, zt.bot);
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
    YASSERT(zt.y);
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
lwrite(MyRune c) {
    int w;
    
    if ((w = wcwidth(c)) <= 0) {
        //printf("can't find character width for %u\n", c);
        w = 1;
    }

    if (zt.col - zt.x < w) {
        lnew();
        zt.x = 0;
    }
    zt.line[zt.y][zt.x] = zt.c;
    zt.line[zt.y][zt.x].c = c;
    zt.line[zt.y][zt.x].width = w;
    ldirty(zt.y, zt.y);
    for (w--, zt.x++; w > 0; zt.x++, w--)
        zt.line[zt.y][zt.x] = zt.c;
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
    int ts, i;

    ltab_clear();
    ts = MAX(4, TABSPACE);
    for (i = ts; i < zt.col; i += ts)
        zt.tabs[i] = 1;
}

void
lrepeat_last(int n) {
    if (!zt.lastc)
        return;
    for (; n > 0; n--)
        lwrite(zt.lastc);
}

void
lcursor(int m) {
    if (m == 0) {
        zt.x_saved = zt.x;
        zt.y_saved = zt.y;
        return;
    }
    lmoveto(zt.y_saved, zt.x_saved);
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
    for (i = 0; i < zt.row; i++) {
        zt.line[i] = (struct MyChar*)p;
        p += sizeof(struct MyChar) * zt.col;
    }
    lclear(0, 0, zt.row-1, zt.col-1);
}

void
linit(void) {
    ATTR_RESET();
    lalloc();
    zt.top = 0;
    zt.bot = zt.row-1;
    zt.x = zt.y = 0;
    zt.x_saved = zt.y_saved = 0;
    zt.lastc = 0;
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

