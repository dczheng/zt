#include <ctype.h>
#include <limits.h>
#include <stdarg.h>

#include "zt.h"
#include "code.h"

void lalloc(void);
void lfree(void);
void lclear(int, int, int, int);
void lclear_all();
void lerase(int, int, int);
void lscroll_up(int, int);
void lscroll_down(int, int);
void lnew(void);
void linsert(int);
void ldelete(int);
void lmoveto(int, int);
void lsettb(int, int);
void ltab(int);
void ltab_clear(void);
void lrepeat_last(int);
void linsert_blank(int);
void ldelete_char(int);
void lcursor(int);
void lalt(int);
void ldirty_all(void);
void tty_write(char*, int);
int lwrite(uint8_t*, int, int*);

#define NOTSUP (1 << 0)
#define RETRY  (1 << 1)
#define SKIP   (1 << 2)

int status;
struct esc_t {
    int len, npar;
    uint8_t *seq, code, csi;
} esc;

char*
to_string(uint8_t *buf, int n) {
    int i, p = 0;
    static char s[BUFSIZ*3+1];

    for (i = 0; i < n; i++) {
        if (isprint(buf[i]))
            p += snprintf(s+p, sizeof(s)-p, "%c", buf[i]);
        else if (ISCTRL(buf[i]))
            p += snprintf(s+p, sizeof(s)-p, "%s", ctrl_name(buf[i]));
        else
            p += snprintf(s+p, sizeof(s)-p, "%02x", buf[i]);
    }
    s[p] = 0;
    return s;
}

void
tdump(void) {
    static int frame = 0;
    int i, j, n;
    struct char_t c;
    char buf[10];

#define _space \
    for (i = 0; i < 6; i++) \
        LOG(" ");

#define _sep \
    _space; \
    for (i = 0; i < zt.col; i++) \
        LOG("-"); \
    LOG("\n"); \

    _sep;
    _space;
    LOG("frame: %d, size: %dx%d, margin: %d-%d, cur: %dx%d\n",
        frame++, zt.row, zt.col, zt.top, zt.bot, zt.y, zt.x);
    _sep;

    _space;
    for (i = 0; i < zt.col; i++)
        if (i % 10 == 0) {
            n = snprintf(buf, sizeof(buf), "%d", i);
            i += n-1;
            LOG("%s", buf);
        } else
            LOG(" ");
    LOG("\n");
    for (i = 0; i < zt.row; i++) {
        LOG("%c %3d ", zt.dirty[i] ? '*' : ' ', i);
        for (j = 0; j < zt.col; j++) {
            c = zt.line[i][j];
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
range_search(uint8_t *seq, int len, uint8_t *e, int mode) {
    for (int i = 0; i < len; i++) {
        if ((!mode) && (seq[i] >= e[0] && seq[i] <= e[1]))
            return i;
        if ((mode) && (seq[i] < e[0] && seq[i] > e[1]))
            return i;
    }
    return -1;
}

int
search(uint8_t *seq, int len, int n, uint8_t *c) {
    int i, j;
    for (i = 0; i < n; i++) {
        for (j = 0; j < len; j++) {
            if (seq[j] == c[i])
                return j;
        }
    }
    return -1;
}
#define SEARCH(seq, len, codes) \
    search(seq, len, (int)sizeof(codes), codes)

int
param(int idx, char **p) {
    static char buf[BUFSIZ];
    int i, a = 1, b = 1, n;

    for (i = 1, n = 0; i < esc.len-1; i++) {
        if (esc.seq[i] != ';')
            continue;
        if (n == idx) {
            b = i-1;
            break;
        }
        a = i+1;
        n++;
    }

    if (i == esc.len-1) {
        if (n == idx)
            b = esc.len-2;
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
    if (n > (int)sizeof(buf)-1)
        return ENOMEM;

    memcpy(buf, esc.seq+a, n);
    buf[n] = '\0';
    *p = buf;
    return 0;

}
#define PARAM(idx, p) do { \
    if (param(idx, p)) { \
        status |= NOTSUP; \
        return; \
    } \
} while(0)

int
param_int(int idx, int *v, int v0) {
    int ret;
    char *p;

    ret = param(idx, &p);
    if (ret)
        return ret;

    if (p == NULL) {
        *v = v0;
        return 0;
    }

    return stoi(v, p);
}
#define PARAM_INT(idx, v, v0) do { \
    if (param_int(idx, v, v0)) { \
        status |= NOTSUP; \
        return; \
    } \
} while(0)

#define _FG8(v) do { \
    zt.c.fg.type = 8; \
    zt.c.fg.c8 = v; \
    zt.c.attr &= ~ATTR_DEFAULT_FG; \
} while(0)

#define _BG8(v) do { \
    zt.c.bg.type = 8; \
    zt.c.bg.c8 = v; \
    zt.c.attr &= ~ATTR_DEFAULT_BG; \
} while(0)

#define _FG24(r, g, b) do { \
    zt.c.fg.type = 24; \
    zt.c.fg.rgb[0] = r; \
    zt.c.fg.rgb[1] = g; \
    zt.c.fg.rgb[2] = b; \
    zt.c.attr &= ~ATTR_DEFAULT_FG; \
} while(0)

#define _BG24(r, g, b) do { \
    zt.c.bg.type = 24; \
    zt.c.bg.rgb[0] = r; \
    zt.c.bg.rgb[1] = g; \
    zt.c.bg.rgb[2] = b; \
    zt.c.attr &= ~ATTR_DEFAULT_BG; \
} while(0)

#define ATTR_RESET() do { \
    ZERO(zt.c.fg); \
    ZERO(zt.c.bg); \
    zt.c.attr = ATTR_DEFAULT_FG | ATTR_DEFAULT_BG; \
    zt.c.width = 1;\
    zt.c.c = ' ';\
} while(0)

void
tsgr(void) {
    int n, m, v, r, g, b, i;

    if (esc.npar == 0) {
        ATTR_RESET();
        return;
    }

    for (i = 0; i < esc.npar;) {
        PARAM_INT(i++, &n, 0);

        if (n >= 30 && n <= 37) {
            _FG8(n-30);
            return;
        }

        if (n >= 40 && n <= 47) {
            _BG8(n-40);
            return;
        }

        if (n >= 90 && n <= 97) {
            _FG8(n-90+8);
            return;
        }

        if (n >= 100 && n <= 107) {
            _BG8(n-100+8);
            return;
        }

        switch (n) {
        case  0: ATTR_RESET()                                   ; break;
        case  1: zt.c.attr |= ATTR_BOLD                         ; break;
        case  2: zt.c.attr |= ATTR_FAINT                        ; break;
        case  3: zt.c.attr |= ATTR_ITALIC                       ; break;
        case  4: zt.c.attr |= ATTR_UNDERLINE                    ; break;
        case  7: zt.c.attr |= ATTR_COLOR_REVERSE                ; break;
        case  9: zt.c.attr |= ATTR_CROSSED_OUT                  ; break;
        case 10: zt.c.attr &= ~ATTR_BOLD|ATTR_FAINT|ATTR_ITALIC ; break;
        case 22: zt.c.attr &= ~ATTR_BOLD|ATTR_FAINT             ; break;
        case 23: zt.c.attr &= ~ATTR_ITALIC                      ; break;
        case 24: zt.c.attr &= ~ATTR_UNDERLINE                   ; break;
        case 27: zt.c.attr &= ~ATTR_COLOR_REVERSE               ; break;
        case 29: zt.c.attr &= ~ATTR_CROSSED_OUT                 ; break;
        case 39: zt.c.attr |= ATTR_DEFAULT_FG                   ; break;
        case 49: zt.c.attr |= ATTR_DEFAULT_BG                   ; break;
        case 38:
        case 48:
            PARAM_INT(i++, &m, 0);
            if (m != 5 && m != 2) {
                status |= NOTSUP;
                return;
            }

            if (m == 5) {
                PARAM_INT(i++, &v, 0);
                if (v < 0 || v > 255) {
                    status |= NOTSUP;
                    return;
                }
                if (n == 38)
                    _FG8(v);
                else
                    _BG8(v);
            } else {
                PARAM_INT(i++, &r, 0);
                if (r < 0 || r > 255) {
                    status |= NOTSUP;
                    return;
                }

                PARAM_INT(i++, &g, 0);
                if (g < 0 || g > 255) {
                    status |= NOTSUP;
                    return;
                }

                PARAM_INT(i++, &b, 0);
                if (b < 0 || b > 255) {
                    status |= NOTSUP;
                    return;
                }

                if (n == 38)
                    _FG24(r, g, b);
                else
                    _BG24(r, g, b);
            }
            break;

        case 58:
        case 59:
            status |= SKIP;
            break;
        default:
            status |= NOTSUP;
        }
    }
}
#undef _FG8
#undef _BG8
#undef _FG24
#undef _BG24

void
tmode(void) {
    int i, n, s;
    char *p;

    if (esc.seq[1] != '?') {
        status |= NOTSUP;
        return;
    }

    s = (esc.csi == SM ? 1 : 0);
#define _M(v) s ? (zt.mode |= v) : (zt.mode &= ~v)

    for (i = 0; i < esc.npar; i++) {
        if (param(i, &p) || p == NULL)
            continue;

        if (stoi(&n, i == 0 ? p+1 : p)) {
            status |= NOTSUP;
            continue;
        }

        switch (n) {
        case M_SF:
            _M(MODE_SEND_FOCUS);
            break;
        case DECTCEM:
            _M(MODE_TEXT_CURSOR);
            break;
        case M_BP:
            _M(MODE_BRACKETED_PASTE);
            break;
        case M_MP:
            _M(MODE_MOUSE|MODE_MOUSE_PRESS);
            break;
        case M_MMP:
            _M(MODE_MOUSE|MODE_MOUSE_PRESS|MODE_MOUSE_MOTION_PRESS);
            break;
        case M_MMA:
            _M(MODE_MOUSE|MODE_MOUSE_MOTION_ANY);
            break;
        case M_ME:
            _M(MODE_MOUSE|MODE_MOUSE_RELEASE|MODE_MOUSE_EXT);
            break;
        case M_SC:
            lcursor(s);
            break;
        case M_ALTS:
            lalt(s);
            ldirty_all();
            break;
        case M_SC_ALTS:
            lcursor(s);
            lalt(s);
            if (s) lclear_all();
            ldirty_all();
            break;
        case DECAWM:
        case DECCKM:
        case M_SBC:
        case M_MUTF8:
        case M_UM:
        case M_SO:
            status |= SKIP;
            break;
        default:
            status |= NOTSUP;
        }
    }
#undef _M
}

void
tdsr(void) {
    int n, nw;
    char wbuf[32], *p;

    if (esc.npar == 0 || param(0, &p) || p == NULL) {
        status |= NOTSUP;
        return;
    }

    if (stoi(&n, p[0] == '?' ? p+1 : p)) {
        status |= NOTSUP;
        return;
    }

    switch (n) {
    case 5:
        nw = snprintf(wbuf, sizeof(wbuf), "\0330n");
        break;
    case 6:
        nw = snprintf(wbuf, sizeof(wbuf), "\033[%d;%dR",
            zt.y+1, zt.x+1);
        break;
    default:
        status |= NOTSUP;
        return;
    }
    tty_write(wbuf, nw);
}

void
tcsi(void) {
    int n, m;

    switch (esc.csi) {
    case CUF:
    case CUB:
    case CUU:
    case CUD:
    case CPL:
    case CNL:
    case IL:
    case DL:
    case DCH:
    case CHA:
    case HPA:
    case VPA:
    case VPR:
    case HPR:
    case SU:
    case SD:
    case ECH:
    case CHT:
    case CBT:
    case ICH:
    case REP:
        PARAM_INT(0, &n, 1);
        break;

    case ED:
    case EL:
    case TBC:
        PARAM_INT(0, &n, 0);
        break;

    case DECSTBM:
        PARAM_INT(0, &n, 1);
        PARAM_INT(1, &m, zt.row);
        break;

    case CUP:
    case HVP:
        if (esc.npar == 1) {
            m = 1;
            PARAM_INT(0, &n, 1);
            break;
        }
        PARAM_INT(0, &n, 1);
        PARAM_INT(1, &m, 1);
        break;
    case DA:
        if (esc.npar == 0)
            n = 0;
        else
            PARAM_INT(0, &n, 0);
        break;
    case DECRC:
        if (esc.npar) {
            status |= NOTSUP;
            return;
        }
        break;
    }

    switch (esc.csi) {
    case CUF    : lmoveto(zt.y  , zt.x+n)     ; break;
    case CUB    : lmoveto(zt.y  , zt.x-n)     ; break;
    case CUU    : lmoveto(zt.y-n, zt.x)       ; break;
    case CUD    : lmoveto(zt.y+n, zt.x)       ; break;
    case CPL    : lmoveto(zt.y-n, 0)          ; break;
    case CNL    : lmoveto(zt.y+n, 0)          ; break;
    case CUP    : lmoveto(n-1   , m-1)        ; break;
    case HVP    : lmoveto(n-1   , m-1)        ; break;
    case CHA    : lmoveto(zt.y  , n-1)        ; break;
    case HPA    : lmoveto(zt.y  , n-1)        ; break;
    case VPA    : lmoveto(n-1   , zt.x)       ; break;
    case HPR    : lmoveto(zt.y  , zt.x+n)     ; break;
    case VPR    : lmoveto(zt.y+n, zt.y)       ; break;
    case IL     : linsert(n)                  ; break;
    case DL     : ldelete(n)                  ; break;
    case SGR    : tsgr()                      ; break;
    case SU     : lscroll_up(zt.top, n)       ; break;
    case SD     : lscroll_down(zt.top, n)     ; break;
    case ECH    : lerase(zt.y, zt.x, zt.x+n-1); break;
    case SM     : tmode()                     ; break;
    case RM     : tmode()                     ; break;
    case DECSTBM: lsettb(n-1, m-1)            ; break;
    case CHT    : ltab(n)                     ; break;
    case CBT    : ltab(-n)                    ; break;
    case ICH    : linsert_blank(n)            ; break;
    case DCH    : ldelete_char(n)             ; break;
    case REP    : lrepeat_last(n)             ; break;
    case DECSC  : lcursor(1)                  ; break;
    case DECRC  : lcursor(0)                  ; break;
    case DSR    : tdsr()                      ; break;
    case DA:
        if (n == 0)
            tty_write(VT102, strlen(VT102));
        break;

    case EL:
        switch (n) {
        case 0: lerase(zt.y, zt.x, zt.col-1); break;
        case 1: lerase(zt.y, 0, zt.x)       ; break;
        case 2: lerase(zt.y, 0, zt.col-1)   ; break;
        default: status |= NOTSUP;
        }
        break;

    case ED:
        switch (n) {
        case 0: lclear(zt.y, zt.x, zt.row-1, zt.col-1); break;
        case 1: lclear(0, 0, zt.y , zt.x)             ; break;
        case 2: lclear_all(); lmoveto(0,0)            ; break;
        case 3: lclear_all(); lmoveto(0,0)            ; break;
        default: status |= NOTSUP;
        }
        break;

    case TBC:
        switch (n) {
        case 0: zt.tabs[zt.x] = 0; break;
        case 3: ltab_clear()     ; break;
        default: status |= NOTSUP;
        }
        break;

    case WINMAN:
    case DECLL:
    case MC:
        status |= SKIP;
        break;
    default:
        status |= NOTSUP;
    }
}

void
tesc(uint8_t *buf, int len) {
    int i, n;

    if (!len) {
        status |= RETRY;
        return;
    }

    ASSERT(len > 0);
    esc.seq = buf;
    esc.code = buf[0];
    esc.len = 1;

    if (ESC_IS_NF(esc.code)) {
        if ((n = range_search(buf+1, len-1, nf_ending, 0)) < 0) {
            status |= RETRY;
            return;
        }
        esc.len += n+1;

        switch (esc.code) {
        case NF_GZD4:
        case NF_G1D4:
        case NF_G2D4:
        case NF_G3D4:
            status |= SKIP;
            break;
        default:
            status |= NOTSUP;
        }
        return;
    }

    if (ESC_IS_FP(esc.code)) {
        switch (esc.code) {
        case FP_DECSC: lcursor(1); break;
        case FP_DECRC: lcursor(0); break;
        case FP_DECPAM:
        case FP_DECPNM:
            status |= SKIP;
            break;
        default:
            status |= NOTSUP;
        }
        return;
    }

    if (ESC_IS_FS(esc.code)) {
        status |= NOTSUP;
        return;
    }

    if (!ESC_IS_FE(esc.code)) {
        status |= NOTSUP;
        return;
    }

    switch (ATOC1(esc.code)) {
    case CSI:
        if ((n = range_search(buf+1, len-1, csi_ending, 0)) < 0) {
            status |= RETRY;
            return;
        }
        esc.len += n+1;
        esc.csi = buf[esc.len-1];
        for (i = 1; i < esc.len-1; i++) {
            if (esc.seq[i] == ';')
                esc.npar++;
        }
        esc.npar++;
        tcsi();
        break;

    case OSC:
        if ((n = SEARCH(buf+1, len-1, osc_ending)) < 0) {
            status |= RETRY;
            return;
        }
        esc.len += n+1;
        break;

    case DCS:
        if ((n = SEARCH(buf+1, len-1, dcs_ending)) < 0) {
            status |= RETRY;
            return;
        }
        esc.len += n+1;
        break;

    case HTS:
        zt.tabs[zt.x] = 1;
        break;

    case RI:
        if (zt.y == zt.top) {
            lscroll_down(zt.top, 1);
            zt.y = zt.top;
        }
        else
            lmoveto(zt.y-1, zt.x);
        break;

    default:
        status |= NOTSUP;
    }
}

void
tctrl(uint8_t *buf, int len) {
    ASSERT(len > 0);
    switch (buf[0]) {
    case ESC: tesc(buf+1, len-1) ; break;
    case LF : lnew()             ; break;
    case CR : lmoveto(zt.y, 0)   ; break;
    case HT : ltab(1)            ; break;
    case HTS: zt.tabs[zt.x] = 1  ; break;
    case BS:
    case CCH:
        lmoveto(zt.y, zt.x-1);
        break;

    case BEL:
    case SS2:
    case SS3:
    case BPH:
    case PAD:
    case SOS:
    case ST:
    case SO:
    case SI:
        status |= SKIP;
        break;
    default:
        status |= NOTSUP;
    }
}

int
twrite(uint8_t *buf, int len) {
    uint8_t *p = buf, *p0;
    int n, retry_max = 3;
    static int retry = 0;
    char *s;

    ASSERT(len >= 0);
    if (!len) return 0;

    if (zt.debug.t)
        LOG("\n------\n%s\n------\n", to_string(buf, len));

    for (; len > 0; p += n, len -= n, retry = 0) {
        if (!ISCTRL(p[0])) {
            if (lwrite(p, len, &n)) {
                retry++;
                break;
            }
            continue;
        }

        status = 0;
        ZERO(esc);
        tctrl(p, len);
        if (status & RETRY) {
            retry++;
            break;
        }

        n = esc.len + 1;

        if (p[0] != ESC)
            zt.lastc.c = 0;

        if (status & NOTSUP || zt.debug.t) {
            s = to_string(p, esc.len+1);
            if (status & NOTSUP)
                LOG("%s unsupported\n", s);
            else if (status & SKIP)
                LOG("%s skip\n", s);
            else
                LOG("%s\n", s);
        }

        if (zt.debug.t > 2)
            tdump();
    }

    if (retry == retry_max) {
        retry = 0;
        p0 = p;
        for (; len > 0; p++, len--) {
            if (ISCTRL(p[0]))
                break;
        }
        LOGERR("drop: %s", to_string(p0, p-p0));
    }

    return p - buf;
}

void
tinit(void) {
    zt.mode = MODE_TEXT_CURSOR;
    ATTR_RESET();
    zt.row = 24;
    zt.col = 80;
    zt.bot = zt.row-1;
    lalloc();
}

void
tfree(void) {
    lfree();
}
