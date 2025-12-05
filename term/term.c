#include <pwd.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#if defined(__linux)
#include <pty.h>
#define  __USE_XOPEN
#elif defined(__APPLE__)
#include <util.h>
#elif defined(__DragonFly__) || defined(__FreeBSD__)
#include <libutil.h>
#endif

#include <wchar.h>
#include "term.h"

#define YLIMIT(y) LIMIT(y, 0, t->row-1)
#define XLIMIT(x) LIMIT(x, 0, t->col-1)

static struct {
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
_term_utf8_decode(uint8_t c, uint32_t *v) {
    for (int i = 0; i < 5; i++)
        if ((c & _utf8[i].mask) == _utf8[i].byte) {
            *v = c & ~_utf8[i].mask;
            return i;
        }
    return -1;
}

int
term_utf8_decode(void *buf, int len, uint32_t *u, int *ulen) {
    int i, j;
    uint32_t uu;
    uint8_t *c = buf;

    if (!len) return EINVAL;

    if ((*ulen = _term_utf8_decode(c[0], u)) <= 0)
        return EINVAL;

    for (i = 1, j = 1; i < len && j < *ulen; i++, j++) {
        if (_term_utf8_decode(c[i], &uu) != 0)
            return EINVAL;
        *u = (*u << 6) | uu;
    }

    if (j < *ulen) return EINVAL;

    if ((*u < _utf8[*ulen].min || *u > _utf8[*ulen].max) ||
        (*u >= 0xd800 && *u <= 0xdfff))
        return EINVAL;
    return 0;
}

void
term_line_dirty(struct term_t *t, int a, int b) {
    ASSERT(a >= 0);
    ASSERT(b >= 0);
    if (a >= t->col) return;
    XLIMIT(b);
    for (; a <= b; a++)
        t->dirty[a] = 1;
}

void
term_line_dirty_all(struct term_t *t) {
    term_line_dirty(t, 0, t->row-1);
}

void
term_line_move(struct term_t *t, int y, int dst, int src, int n) {
    ASSERT(y >= 0 && y < t->row);
    ASSERT(dst >= 0);
    ASSERT(src >= 0);
    ASSERT(n >= 0);
    if (dst >= t->col || src >= t->col)
        return;
    LIMIT(n, 0, t->col-dst);
    LIMIT(n, 0, t->col-src);
    if (n == 0) return;
    memmove(&t->line[y][dst], &t->line[y][src],
        n * sizeof(struct term_char_t));
}

void
term_line_erase(struct term_t *t, int y, int a, int b) {
    ASSERT(y >= 0 && y < t->row);
    ASSERT(a >= 0);
    ASSERT(b >= 0);
    if (a >= t->col) return;
    XLIMIT(b);
    for (; a <= b && a < t->col; a++)
        t->line[y][a] = t->c;
    term_line_dirty(t, y, y);
}

void
term_line_delete_char(struct term_t *t, int n) {
    LIMIT(n, 0, t->col - t->x);
    term_line_move(t, t->y, t->x, t->x+n, t->col-t->x-n);
    term_line_erase(t, t->y, t->col-n, t->col-1);
}

void
term_line_insert_blank(struct term_t *t, int n) {
    LIMIT(n, 0, t->col - t->x);
    term_line_move(t, t->y, t->x+n, t->x, t->col-t->x-n);
    term_line_erase(t, t->y, t->x, t->x+n-1);
}

void
term_line_clear(struct term_t *t, int y1, int x1, int y2, int x2) {
    if (y1 > y2) return;
    term_line_erase(t, y1, x1, t->col-1);
    term_line_erase(t, y2, 0, x2);
    for (y1++; y1 < y2; y1++)
        term_line_erase(t, y1, 0, t->col-1);
}

void
term_line_clear_all(struct term_t *t) {
    term_line_clear(t, 0, 0, t->row-1, t->col-1);
}

void
term_line_moveto(struct term_t *t, int y, int x) {
    t->y = y;
    t->x = x;
    XLIMIT(t->x);
    YLIMIT(t->y);
}

void
term_line_top_bottom(struct term_t *t, int top, int bot) {
    t->top = top;
    t->bot = bot;
    term_line_moveto(t, 0, 0);
    YLIMIT(t->top);
    YLIMIT(t->bot);
    ASSERT(t->top <= t->bot);
}

void
term_line_scroll_up(struct term_t *t, int y, int n) {
    if (n <= 0) return;
    n = MIN(n, t->bot-y+1);
    for (int i = y; i <= t->bot-n; i++)
        SWAP(t->line[i], t->line[i+n]);
    term_line_dirty(t, y, t->bot);
    term_line_clear(t, t->bot-n+1, 0, t->bot, t->col-1);
}

void
term_line_scroll_down(struct term_t *t, int y, int n) {
    if (n <= 0) return;
    n = MIN(n, t->bot-y+1);
    for (int i = t->bot; i >= y+n; i--)
        SWAP(t->line[i], t->line[i-n]);
    term_line_dirty(t, y, t->bot);
    term_line_clear(t, y, 0, y+n-1, t->col-1);
}

void
term_line_new(struct term_t *t) {
    if (t->y == t->bot) {
        term_line_scroll_up(t, t->top, 1);
        return;
    }
    t->y++;
    ASSERT(t->y >= 0 && t->y < t->row);
    //YLIMIT(t->y);
}

void
term_line_tab(struct term_t *t, int n) {
    for (; t->x < t->col && n > 0; n--)
        for (t->x++; t->x < t->col && !t->tabs[t->x]; t->x++);
    for (; t->x > 0 && n < 0; n++)
        for (t->x--; t->x > 0 && !t->tabs[t->x]; t->x--);
    XLIMIT(t->x);
}

void
term_line_tab_put(struct term_t *t) {
    t->tabs[t->x] = 1;
}

void
term_line_tab_remove(struct term_t *t) {
    t->tabs[t->x] = 0;
}

void
term_line_tab_clear(struct term_t *t) {
    memset(t->tabs, 0, t->col * sizeof(*t->tabs));
}

void
term_line_tab_reset(struct term_t *t)  {
    term_line_tab_clear(t);
    for (int i = 8; i < t->col; i += 8)
        t->tabs[i] = 1;
}

void
_term_line_write(struct term_t *t, uint32_t c) {
    int w;

    if ((w = wcwidth(c)) <= 0) {
        LOG("can't find character width for %u\n", c);
        w = 1;
    }

    if (t->col - t->x < w) {
        term_line_new(t);
        t->x = 0;
    }
    t->line[t->y][t->x] = t->c;
    t->line[t->y][t->x].c = c;
    t->line[t->y][t->x].width = w;
    t->lastc = t->line[t->y][t->x];

    term_line_dirty(t, t->y, t->y);
    for (w--, t->x++; w > 0; t->x++, w--)
        t->line[t->y][t->x] = t->c;
}

int
term_line_write(struct term_t *t, uint8_t *buf, int len, int *wlen) {
    int ret = 0;
    uint32_t c;

    *wlen = 0;
    if (len <= 0) return 0;
    c = buf[0];

    if (MODE_ISSET(t, MODE_GZD4) && c >= GZD4_MIN && c <= GZD4_MAX) {
        ret = term_utf8_decode((uint8_t*)gzd4[c-GZD4_MIN], 4, &c, wlen);
        *wlen = 1;
    } else {
        ret = term_utf8_decode(buf, len, &c, wlen);
    }

    if (!ret)
        _term_line_write(t, c);
    return ret;
}

void
term_line_insert(struct term_t *t, int n) {
    if (t->y >= t->top && t->y <= t->bot)
        term_line_scroll_down(t, t->y, n);
}

void
term_line_delete(struct term_t *t, int n) {
    if (t->y >= t->top && t->y <= t->bot)
        term_line_scroll_up(t, t->y, n);
}

void
term_line_dirty_reset(struct term_t *t) {
    memset(t->dirty, 0, t->row * sizeof(*t->dirty));
}

void
term_line_repeat_last(struct term_t *t, int n) {
    while (t->lastc.c && n--)
        _term_line_write(t, t->lastc.c);
}

void
term_line_cursor(struct term_t *t, int save) {
    if (save) {
        t->x_saved = t->x;
        t->y_saved = t->y;
    } else
        term_line_moveto(t, t->y_saved, t->x_saved);
}

void
term_line_alt(struct term_t *t, int alt, int clear) {
    t->line = (alt ? t->alt.line : t->normal.line);
    if (clear) term_line_clear_all(t);
    term_line_dirty_all(t);
}

void
term_line_free(struct term_t *t) {
    free(t->normal.buffer);
    free(t->alt.buffer);
    free(t->dirty);
}

void
term_line_alloc(struct term_t *t) {
    char *p;
    int i, n;

    ASSERT(t->dirty = malloc(t->row * sizeof(*t->dirty)));
    term_line_dirty_all(t);

    ASSERT(t->tabs = malloc(t->col * sizeof(*t->tabs)));
    term_line_tab_reset(t);

    n = (sizeof(struct term_char_t*) +
        sizeof(struct term_char_t) * t->col) * t->row;

#define X(x) \
    ASSERT(t->x.buffer = malloc(n)); \
    t->x.line = (struct term_char_t **)t->x.buffer; \
    p = t->x.buffer + sizeof(struct term_char_t*) * t->row; \
    for (i = 0; i < t->row; i++) { \
        t->x.line[i] = (struct term_char_t*)p; \
        p += sizeof(struct term_char_t) * t->col; \
    }  \
    t->line = t->x.line; \
    term_line_clear_all(t);

    X(alt);
    X(normal);
#undef X
}

void
term_line_resize(struct term_t *t, int r, int c) {
    int i, j, mr, mc, row = t->row, col = t->col;
    char *normal_buffer = t->normal.buffer;
    char *alt_buffer = t->alt.buffer;
    struct term_char_t **normal_line = t->normal.line;
    struct term_char_t **alt_line = t->alt.line;
    int *tabs = t->tabs;

    t->row = r;
    t->col = c;

    mr = MIN(t->row, row);
    mc = MIN(t->col, col);

    free(t->dirty);
    term_line_alloc(t);

    for (i = 0; i < mr; i++)
        for (j = 0; j < mc; j++) {
            t->normal.line[i][j] = normal_line[i][j];
            t->alt.line[i][j] = alt_line[i][j];
        }
    free(normal_buffer);
    free(alt_buffer);

    for (i = 0; i < mc; i++)
        t->tabs[i] = tabs[i];
    free(tabs);

    XLIMIT(t->x);
    YLIMIT(t->y);
    t->bot = row - t->bot + t->row;
    YLIMIT(t->top);
    YLIMIT(t->bot);
}

void // for debug
term_dump(struct term_t *t) {
    static int frame = 0;
    int i, j, n;
    struct term_char_t c;
    char buf[10];

#define _space \
    for (i = 0; i < 6; i++) \
        LOG(" ");

#define _sep \
    _space; \
    for (i = 0; i < t->col; i++) \
        LOG("-"); \
    LOG("\n"); \

    _sep;
    _space;
    LOG("frame: %d, size: %dx%d, margin: %d-%d, cur: %dx%d\n",
        frame++, t->row, t->col, t->top, t->bot, t->y, t->x);
    _sep;

    _space;
    for (i = 0; i < t->col; i++)
        if (i % 10 == 0) {
            n = snprintf(buf, sizeof(buf), "%d", i);
            i += n-1;
            LOG("%s", buf);
        } else
            LOG(" ");
    LOG("\n");
    for (i = 0; i < t->row; i++) {
        LOG("%c %3d ", t->dirty[i] ? '*' : ' ', i);
        for (j = 0; j < t->col; j++) {
            c = t->line[i][j];
            if (isprint(c.c))
                LOG("%c", c.c);
            else
                LOG("%X ", c.c);
        }
        LOG("\n");
    }
    _sep;

#undef _sep
#undef _space
}

int
term_range_end(struct term_t *t, uint8_t *e) {
    for (int i = t->ctrl.n; i < t->ctrl.size; i++) {
        if ((t->ctrl.base[i] >= e[0] && t->ctrl.base[i] <= e[1])) {
            t->ctrl.n = i+1;
            return 0;
        }
    }
    return EAGAIN;
}

int
term_ctrl_end(struct term_t *t, uint8_t *e, int n) {
    int i, j;
    for (i = 0; i < n; i++) {
        for (j = t->ctrl.n; j < t->ctrl.size; j++) {
            if (t->ctrl.base[j] == e[i]) {
                t->ctrl.n = j+1;
                return 0;
            }
        }
    }
    return EAGAIN;
}
#define CTRL_END(c, codes) term_ctrl_end(c, codes, (int)sizeof(codes))

int
term_ctrl_param(struct term_t *t, int off, int idx, char **p) {
    int i, a = off, b = off, n;

    for (i = off, n = 0; i < t->ctrl.n-1; i++) {
        if (t->ctrl.base[i] != ';')
            continue;
        if (n == idx) {
            b = i-1;
            break;
        }
        a = i+1;
        n++;
    }

    if (i == t->ctrl.n-1) {
        if (n == idx)
            b = t->ctrl.n-2;
        else
            return EINVAL;
    }

    if (a > b) {
        a = -1;
        b = -1;
    }

    if (a < 0) {
        *p = NULL;
        return 0;
    }

    n = b-a+1;
    if (n > (int)sizeof(t->param)-1)
        return ENOMEM;

    memcpy(t->param, t->ctrl.base+a, n);
    t->param[n] = '\0';
    *p = t->param;
    return 0;

}

int
term_ctrl_param_int(struct term_t *t, int off, int idx, int *v, int v0) {
    int ret;
    char *p;

    ret = term_ctrl_param(t, off, idx, &p);
    if (ret)
        return ret;

    if (p == NULL) {
        *v = v0;
        return 0;
    }

    return stoi(v, p);
}
#define CTRL_PARAM_INT(idx, v, v0) \
    if (term_ctrl_param_int(t, 2, idx, v, v0)) \
        return EPROTO;

#define CHAR_MODE_RESET() do { \
    ZERO(t->c.fg); \
    ZERO(t->c.bg); \
    t->c.mode = CHAR_MODE_DEFAULT_FG | CHAR_MODE_DEFAULT_BG; \
    t->c.width = 1;\
    t->c.c = ' ';\
} while(0)

void
_term_sgr_c8(struct term_t *t, int fg, int v) {
    struct term_color_t *c = fg ? & t->c.fg : &t->c.bg;
    MODE_UNSET(&t->c, fg ? CHAR_MODE_DEFAULT_FG : CHAR_MODE_DEFAULT_BG);
    c->type = 8;
    c->c8 = v;
}

void
_term_sgr_c24(struct term_t *t, int fg, int r, int g, int b) {
    struct term_color_t *c = fg ? & t->c.fg : &t->c.bg;
    MODE_UNSET(&t->c, fg ? CHAR_MODE_DEFAULT_FG : CHAR_MODE_DEFAULT_BG);
    LIMIT(r, 0, 255);
    LIMIT(g, 0, 255);
    LIMIT(b, 0, 255);
    c->type = 24;
    c->r = r;
    c->g = g;
    c->b = b;
}

int
term_sgr(struct term_t *t) {
    int n, m, v, r, g, b, i;

    if (t->ctrl.npar == 0) {
        CHAR_MODE_RESET();
        return 0;
    }

    for (i = 0; i < t->ctrl.npar;) {
        CTRL_PARAM_INT(i++, &n, 0);

        if (n >= 30 && n <= 37) {
            _term_sgr_c8(t, 1, n-30);
            continue;
        }

        if (n >= 40 && n <= 47) {
            _term_sgr_c8(t, 0, n-40);
            continue;
        }

        if (n >= 90 && n <= 97) {
            _term_sgr_c8(t, 1, n-90+8);
            continue;
        }

        if (n >= 100 && n <= 107) {
            _term_sgr_c8(t, 0, n-100+8);
            continue;
        }

        switch (n) {
        case  0: CHAR_MODE_RESET(); break;
        case  1: MODE_SET(&t->c, CHAR_MODE_BOLD); break;
        case  2: MODE_SET(&t->c, CHAR_MODE_FAINT); break;
        case  3: MODE_SET(&t->c, CHAR_MODE_ITALIC); break;
        case  4: MODE_SET(&t->c, CHAR_MODE_UNDERLINE); break;
        case  7: MODE_SET(&t->c, CHAR_MODE_COLOR_REVERSE); break;
        case  9: MODE_SET(&t->c, CHAR_MODE_CROSSED_OUT); break;
        case 10: MODE_UNSET(&t->c,
            CHAR_MODE_BOLD|CHAR_MODE_FAINT|CHAR_MODE_ITALIC); break;
        case 22: MODE_UNSET(&t->c, CHAR_MODE_BOLD|CHAR_MODE_FAINT); break;
        case 23: MODE_UNSET(&t->c, CHAR_MODE_ITALIC); break;
        case 24: MODE_UNSET(&t->c, CHAR_MODE_UNDERLINE); break;
        case 27: MODE_UNSET(&t->c, CHAR_MODE_COLOR_REVERSE); break;
        case 29: MODE_UNSET(&t->c, CHAR_MODE_CROSSED_OUT); break;
        case 39: MODE_SET(&t->c, CHAR_MODE_DEFAULT_FG); break;
        case 49: MODE_SET(&t->c, CHAR_MODE_DEFAULT_BG); break;
        case 38:
        case 48:
            CTRL_PARAM_INT(i++, &m, 0);
            switch (m) {
            case 5:
                CTRL_PARAM_INT(i++, &v, 0);
                if (v < 0 || v > 255)
                    return EPROTO;
                _term_sgr_c8(t, n == 38, v);
                break;
            case 2:
                CTRL_PARAM_INT(i++, &r, 0);
                CTRL_PARAM_INT(i++, &g, 0);
                CTRL_PARAM_INT(i++, &b, 0);
                _term_sgr_c24(t, n == 38, r, g, b);
                break;
            default: return EPROTO;
            }
            break;
        default: return EPROTO;
        }
    }
    return 0;
}

int
term_mode(struct term_t *t) {
    int i, n, s, pri;
    char *p;

    pri = t->ctrl.base[2] == '?';
    s = (t->ctrl.csi == SM ? 1 : 0);

#define _M(v) s ? MODE_SET(t, v) : MODE_UNSET(t , v)
    for (i = 0; i < t->ctrl.npar; i++) {
        if (term_ctrl_param(t, 2, i, &p) || p == NULL)
            continue;

        if (stoi(&n, i == 0 && pri ? p+1 : p))
            return EPROTO;

        switch (n) {
        case DECTCEM: _M(MODE_CURSOR); break;
        case 1000: _M(MODE_MOUSE_PRESS|MODE_MOUSE_RELEASE); break;
        case 1002: _M(MODE_MOUSE_MOTION); break;
        case 1003: _M(MODE_MOUSE); break;
        case 1004: _M(MODE_FOCUS); break;
        case 1006: _M(MODE_MOUSE_SGR); break;
        case DECGRPM:
        case 1047: term_line_alt(t, s, 0); break;
        case 1048: term_line_cursor(t, s); break;
        case 1049: term_line_cursor(t, s); term_line_alt(t, s, s); break;
        case DECCKM:
        case DECSCLM:
        case DECAWM:
        case DECKANAM:
        case 1015: // urxvt Mouse
        case 1005: // UTF8 mouse
        case 2004: // Bracketed paste
            return EACCES;
        default: return EPROTO;
        }
    }
    return 0;
#undef _M
}

int
term_dsr(struct term_t *t) {
    int n, nw;
    char wbuf[32], *p;

    if (t->ctrl.npar == 0 || term_ctrl_param(t, 2, 0, &p) || p == NULL)
        return EPROTO;

    if (stoi(&n, p[0] == '?' ? p+1 : p))
        return EPROTO;

    switch (n) {
    case 5:
        nw = snprintf(wbuf, sizeof(wbuf), "\0330n");
        break;
    case 6:
        nw = snprintf(wbuf, sizeof(wbuf), "\033[%d;%dR",
            t->y+1, t->x+1);
        break;
    default: return EPROTO;
    }
    term_write(t, wbuf, nw);
    return 0;
}

int
term_csi(struct term_t *t) {
    int n = 0, m = 0;

    switch (t->ctrl.csi) {
    case CUF: case CUB: case CUU: case CUD: case CPL: case CNL:
    case IL: case DL: case DCH: case CHA: case HPA: case VPA:
    case VPR: case HPR: case SU: case SD: case ECH: case CHT:
    case CBT: case ICH: case REP:
        CTRL_PARAM_INT(0, &n, 1);
        break;
    case ED:
    case EL:
    case TBC:
        CTRL_PARAM_INT(0, &n, 0);
        break;
    case DECSTBM:
        CTRL_PARAM_INT(0, &n, 1);
        CTRL_PARAM_INT(1, &m, t->row);
        break;
    case CUP:
    case HVP:
        if (t->ctrl.npar == 1) {
            m = 1;
            CTRL_PARAM_INT(0, &n, 1);
            break;
        }
        CTRL_PARAM_INT(0, &n, 1);
        CTRL_PARAM_INT(1, &m, 1);
        break;
    case DA:
        if (t->ctrl.npar == 0)
            n = 0;
        else
            CTRL_PARAM_INT(0, &n, 0);
        break;
    case DECRC:
        if (t->ctrl.npar)
            return EPROTO;
        break;
    }

    switch (t->ctrl.csi) {
    case SGR: return term_sgr(t);
    case SM: return term_mode(t);
    case RM: return term_mode(t);
    case CUF: term_line_moveto(t, t->y, t->x+n); break;
    case CUB: term_line_moveto(t, t->y, t->x-n); break;
    case CUU: term_line_moveto(t, t->y-n, t->x); break;
    case CUD: term_line_moveto(t, t->y+n, t->x); break;
    case CPL: term_line_moveto(t, t->y-n, 0); break;
    case CNL: term_line_moveto(t, t->y+n, 0); break;
    case CUP: term_line_moveto(t, n-1, m-1); break;
    case HVP: term_line_moveto(t, n-1,m-1); break;
    case CHA: term_line_moveto(t, t->y, n-1); break;
    case HPA: term_line_moveto(t, t->y, n-1); break;
    case VPA: term_line_moveto(t, n-1, t->x); break;
    case HPR: term_line_moveto(t, t->y, t->x+n); break;
    case VPR: term_line_moveto(t, t->y+n, t->y); break;
    case IL: term_line_insert(t, n); break;
    case DL: term_line_delete(t, n); break;
    case SU: term_line_scroll_up(t, t->top, n); break;
    case SD: term_line_scroll_down(t, t->top, n); break;
    case ECH: term_line_erase(t,t->y, t->x, t->x+n-1); break;
    case DECSTBM: term_line_top_bottom(t, n-1, m-1); break;
    case CHT: term_line_tab(t, n); break;
    case CBT: term_line_tab(t, -n); break;
    case ICH: term_line_insert_blank(t, n); break;
    case DCH: term_line_delete_char(t, n); break;
    case REP: term_line_repeat_last(t, n); break;
    case DECSC: term_line_cursor(t, 1); break;
    case DECRC: term_line_cursor(t, 0); break;
    case DSR: term_dsr(t); break;
    case DA:
        if (n == 0)
            term_write(t, PRIMARY_DA, strlen(PRIMARY_DA));
        break;

    case EL:
        switch (n) {
        case 0: term_line_erase(t, t->y, t->x, t->col-1); break;
        case 1: term_line_erase(t, t->y, 0, t->x); break;
        case 2: term_line_erase(t, t->y, 0, t->col-1); break;
        default: return EPROTO;
        }
        break;

    case ED:
        switch (n) {
        case 0: term_line_clear(t, t->y, t->x, t->row-1, t->col-1); break;
        case 1: term_line_clear(t, 0, 0, t->y , t->x); break;
        case 2: term_line_clear_all(t); term_line_moveto(t, 0,0); break;
        case 3: term_line_clear_all(t); term_line_moveto(t, 0,0); break;
        default: return EPROTO;
        }
        break;

    case TBC:
        switch (n) {
        case 0: term_line_tab_remove(t); break;
        case 3: term_line_tab_clear(t); break;
        default: return EPROTO;
        }
        break;
    case WINMAN:
        return EACCES;
    default: return EPROTO;
    }
    return 0;
}

int
term_esc(struct term_t *t) {
    int i;

    if (t->ctrl.size < 2)
        return EAGAIN;

    t->ctrl.esc = t->ctrl.base[1];
    t->ctrl.n += 1;

    if (!ISESC(t->ctrl.esc))
        return EPROTO;

    if (!ISFEESC(t->ctrl.esc)) {
        if (ISNFESC(t->ctrl.esc)) {
            if (term_range_end(t, nf_end))
                return EAGAIN;
        }
        switch (t->ctrl.esc) {
        case NF_GZD4:
            if (t->ctrl.size < 3) return EPROTO;
            switch (t->ctrl.base[2]) {
            case 'B': MODE_UNSET(t, MODE_GZD4); break;
            case '0': MODE_SET(t, MODE_GZD4); break;
            default: return EPROTO;
            }
            break;
        case FP_DECSC: term_line_cursor(t, 1); break;
        case FP_DECRC: term_line_cursor(t, 0); break;
        case NF_G1D4:
        case NF_G2D4:
        case NF_G3D4:
        case FP_DECKPAM:
        case FP_DECKPNM:
            return EACCES;
        default: return EPROTO;
        }
        return 0;
    }

    switch (FETOC1(t->ctrl.esc)) {
    case CSI:
        if (term_range_end(t, csi_end))
            return EAGAIN;

        t->ctrl.csi = t->ctrl.base[t->ctrl.n-1];
        for (i = 2; i < t->ctrl.n-1; i++) {
            if (t->ctrl.base[i] == ';')
                t->ctrl.npar++;
        }
        t->ctrl.npar++;
        return term_csi(t);

    case OSC:
        if (CTRL_END(t, osc_end))
            return EAGAIN;
        return 0;

    case DCS:
        if (CTRL_END(t, dcs_end))
            return EAGAIN;
        return 0;

    case HTS:
        term_line_tab_put(t);
        break;

    case RI:
        if (t->y == t->top) {
            term_line_scroll_down(t, t->top, 1);
            t->y = t->top;
        }
        else
            term_line_moveto(t, t->y-1, t->x);
        break;

    default: return EPROTO;
    }
    return 0;
}

int
term_ctrl(struct term_t *t) {
    ASSERT(t->ctrl.size > 0);
    t->ctrl.n = 1;
    switch (t->ctrl.base[0]) {
    case ESC: return term_esc(t);
    case LF: term_line_new(t); break;
    case CR: term_line_moveto(t, t->y, 0); break;
    case HT: term_line_tab(t, 1); break;
    case HTS: term_line_tab_put(t); break;
    case BS:
    case CCH:
        term_line_moveto(t, t->y, t->x-1);
        break;
    case SI: MODE_UNSET(t, MODE_GZD4); break;
    case SO: MODE_SET(t, MODE_GZD4); break;
    case BEL:
        return EACCES;
    default: return EPROTO;
    }
    return 0;
}

int
_term_read(struct term_t *t) {
    uint8_t *p = t->data, *p0;
    int m, retry_max = 3, ret, n = t->size;
    char s[BUFSIZ*3+1];

    ASSERT(n >= 0);
    if (!n) return 0;

    if (t->debug >= 2)
        LOG("\n------\n%s\n------\n",
            ctrl_str(s, sizeof(s), t->data, t->size));

    for (; n > 0; p += m, n -= m, t->retry = 0) {
        if (!ISCTRL(p[0])) {
            if (term_line_write(t, p, n, &m)) {
                t->retry++;
                break;
            }
            continue;
        }

        ZERO(t->ctrl);
        t->ctrl.base = p;
        t->ctrl.size = n;
        ret = term_ctrl(t);
        if (ret == EAGAIN) {
            t->retry++;
            break;
        }

        m = t->ctrl.n;
        if (p[0] != ESC)
            t->lastc.c = 0;

        if (t->debug <= 0)
            continue;

        ctrl_str(s, sizeof(s), t->ctrl.base, t->ctrl.n);
        switch (ret) {
        case 0:
            if (t->debug >= 2)
                LOG("%s\n", s);
            break;
        case EPROTO:
            if (t->debug == 1)
                LOG("%s ?????\n", s);
            break;
        case EACCES:
            if (t->debug == 1 && t->no_ignore)
                LOG("%s ignore\n", s);
            break;
        default: DIE();
        }
        if (t->debug >= 3)
            term_dump(t);
    }

    if (t->retry == retry_max) {
        t->retry = 0;
        p0 = p;
        for (; n > 0; p++, n--) {
            if (ISCTRL(p[0]))
                break;
        }
        LOGERR("drop: %s", ctrl_str(s, sizeof(s), p0, p-p0));
    }

    return p - t->data;
}

int
term_read(struct term_t *t) {
    int ret, m;
    fd_set fds;
    struct timespec tv = to_timespec(SECOND);

    ASSERT(t->size >= 0 && t->size < BUFSIZ);

    FD_ZERO(&fds);
    FD_SET(t->tty, &fds);
    ASSERT(pselect(t->tty+1, &fds, NULL, NULL, &tv, NULL) > 0);
    if ((ret = read(t->tty, t->data+t->size, sizeof(t->data)-1)) < 0) {
        if (errno != EIO)
            LOGERR("failed to read tty: %s\n", strerror(errno));
        return errno;
    }

    t->size += ret;
    m = _term_read(t);
    t->size -= m;
    if (t->size > 0)
        memmove(t->data, t->data+m, t->size);
    return 0;
}

int
term_write(struct term_t *t, char *s, int n) {
    int ret;
    fd_set fds;
    struct timespec tv = to_timespec(SECOND);

    for (;;) {
        if (n <= 0) break;

        FD_ZERO(&fds);
        FD_SET(t->tty, &fds);
        ASSERT(pselect(t->tty+1, NULL, &fds, NULL, &tv, NULL) > 0);

        if ((ret = write(t->tty, s, n)) < 0)  {
            if (errno != EIO)
                LOGERR("failed to read tty: %s\n", strerror(errno));
            return errno;
        }
        n -= ret;
        s += ret;
    }
    return 0;
}

void
term_resize(struct term_t* t, int r, int c, int w, int h) {
    struct winsize ws;
    int ret;
    ws.ws_row = r;
    ws.ws_col = c;
    ws.ws_xpixel = w;
    ws.ws_ypixel = h;
    ASSERT((ret = ioctl(t->tty, TIOCSWINSZ, &ws)) >= 0);
    term_line_resize(t, r, c);
}

void
term_flush(struct term_t* t) {
    term_line_dirty_reset(t);
}

void
term_init(struct term_t *t, char *term) {
    char *sh, *args[2];
    struct passwd *pw;
    int ret, slave;
    pid_t pid;

    ZERO(*t);
    t->mode = MODE_CURSOR;
    CHAR_MODE_RESET();
    t->row = 24;
    t->col = 80;
    t->bot = t->row-1;
    t->size = 0;

    ASSERT((ret = openpty(&t->tty, &slave, NULL, NULL, NULL)) >= 0);
    ASSERT(pw = getpwuid(getuid()));

    if (!(sh = getenv("SHELL"))) {
        if (pw->pw_shell[0])
            sh = pw->pw_shell;
        else
            sh = "/bin/sh";
    }

    ASSERT((pid = fork()) != -1);
    if (pid) {
        term_line_alloc(t);
        close(slave);
        return;
    }

    setsid();
    close(t->tty);
    dup2(slave, 0);
    dup2(slave, 1);
    dup2(slave, 2);
    ASSERT((ret = ioctl(slave, TIOCSCTTY, NULL)) >= 0);
    if (slave > 2)
        close(slave);

    setenv("SHELL", sh, 1);
    setenv("TERM", term, 1);
    setenv("HOME", pw->pw_dir, 1);
    setenv("USER", pw->pw_name, 1);
    setenv("LONGNAME", pw->pw_name, 1);

    args[0] = sh;
    args[1] = NULL;
    execvp(sh, args);
    _exit(1);
}

void
term_free(struct term_t *t) {
    term_line_free(t);
    close(t->tty);
}
