#include <ctype.h>
#include <limits.h>
#include <stdarg.h>

#include "zt.h"
#include "ctrl.h"

int ctrl_error, esc_error;
struct esc_t esc;

/*
  References
  https://en.wikipedia.org/wiki/ANSI_escape_code
  https://en.wikipedia.org/wiki/C0_and_C1_control_codes
  https://vt100.net/docs
  https://vt100.net/docs/vt102-ug/contents.html
  https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
*/

#define VT102 "\033[?6c"

#define ERR_OTHER  -1
#define ERR_RETRY   1
#define ERR_UNSUPP  2
#define ERR_ESC     3
#define ERR_PAR     4

#define RETRY_MAX 3

//#define DEBUG_CTRL
//#define DEBUG_CTRL_TERM
//#define DEBUG_TERM
//#define DEBUG_NOUTF8
//#define DEBUG_WRITE
//#define DEBUG_BUF

int utf8_decode(unsigned char*, int, uint32_t*, int*);
void lclear(int, int, int, int);
void lclear_all();
void lerase(int, int, int);
void lscroll_up(int, int);
void lscroll_down(int, int);
void lnew(void);
void lwrite(uint32_t);
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
void ldirty_all(void);
void twrite(char*, int);

struct esc_t {
    int len;
    unsigned char *seq, type, esc, csi;
};


void
dump(unsigned char *buf, int n) {
    struct ctrl_info_t *info = NULL;
    int i;

    if (n == 0) return;
    for (i = 0; i < n; i++) {
        if (isprint(buf[i])) {
            printf("%c", buf[i]);
            continue;
        }
        if (ISCTRL(buf[i])) {
            ctrl_info(buf[i], &info);
            printf("%s", info->name);
            continue;
        }
        printf("%0x02X", buf[i]);
    }
    printf("\n");
    fflush(stdout);
}

int
parse_int(char *p, int *r) {
    long ret;
    char *ed;

    *r = 0;
    if (p == NULL || !strlen(p))
        return EINVAL;

    ret = strtol(p, &ed, 10);
    if (ed[0])
        return EILSEQ;
    *r = ret;
    return 0;
}

int
range_search(unsigned char *seq, int len,
        unsigned char a, unsigned char b, int mode) {
    for (int i = 0; i < len; i++) {
        if ((!mode) && (seq[i] >= a && seq[i] <= b))
            return i;
        if ((mode) && (seq[i] < a && seq[i] > b))
            return i;
    }
    return -1;
}

int
search(unsigned char *seq, int len, int n, unsigned char *c) {
    int i, j;
    for (j = 0; j < len; j++) {
        for (i = 0; i < n; i++) {
            if (seq[j] == c[i])
                return j;
        }
    }
    return -1;
}

int
get_par_num(struct esc_t *esc) {
    int i, n;

    for (i = 1, n = 0; i < esc->len-1; i++) {
        if (esc->seq[i] == ';')
            n++;
    }
    return n+1;
}

int
get_par(struct esc_t *esc, int idx, int *a, int *b) {
    int i, n;

    *a = *b = 1;
    for (i = 1, n = 0; i < esc->len-1; i++) {
        if (esc->seq[i] != ';')
            continue;
        if (n == idx) {
            *b = i-1;
            break;
        }
        *a = i+1;
        n++;
    }

    if (i == esc->len-1) {
        if (n == idx)
            *b = esc->len-2;
        else
            return 1;
    }

    if (*a > *b) {
        *a = -1;
        *b = -1;
    }

    return 0;
}

int
get_str_par(struct esc_t *esc, int idx, char **p) {
    static char buf[BUFSIZ];
    int a, b;
    size_t n;

    if (get_par(esc, idx, &a, &b))
        return EDOM;

    if (a<0) {
        *p = NULL;
        return 0;
    }

    n = b-a+1;
    if (n > sizeof(buf)-1)
        return ENOMEM;

    memcpy(buf, esc->seq+a, n);
    buf[n] = '\0';
    *p = buf;
    return 0;

}

int
get_int_par(struct esc_t *esc, int idx, int *v, int v0) {
    int ret;
    char *p;

    ret = get_str_par(esc, idx, &p);
    if (ret)
        return ret;

    if (p == NULL) {
        *v = v0;
        return 0;
    }
    return parse_int(p, v);
}

void // debug
csi_dump(struct esc_t *esc) {
    int i, r, rr, a, b;

     if (esc->csi != SGR)
        return;
     dump(esc->seq, esc->len);
     for (i = 0; i < get_par_num(esc); i++) {
         if ((rr = get_int_par(esc, i, &r, 1)))
             printf("%s\n", strerror(rr));
         else
             printf("%d\n", r);
         if (!get_par(esc, i, &a, &b)) {
             if (a<0)
                 printf("-\n");
             else
                 dump(esc->seq+a, b-a+1);
         }
     }
    // printf("%s\n", get_esc_str(esc, 1));
}

int
find_osc_end(unsigned char *seq, int len, int *n) {
    unsigned char c[] = {BEL, ST, C1ALT(ST), ESC};
    if ((*n = search(seq, len, sizeof(c), c)) < 0)
        return ESCOSCNOEND;
    return 0;
}

int
find_csi_end(unsigned char *seq, int len, int *n) {
    if ((*n = range_search(seq, len, 0x40, 0x7E, 0)) < 0)
        return ESCCSINOEND;
    return 0;
}

int
find_dcs_end(unsigned char *seq, int len, int *n) {
    unsigned char c[] = {C1ALT(ST), ESC};
    if ((*n = search(seq, len, sizeof(c), c)) < 0)
        return ESCDCSNOEND;
    return 0;
}

int
find_nfesc_end(unsigned char *seq, int len, int *n) {
    if ((*n = range_search(seq, len, 0x30, 0x7E, 0)) < 0)
        return ESCNFNOEND;
    return 0;
}

int
esc_parse(unsigned char *seq, int len, struct esc_t *esc) {
    unsigned char c;
    int n, ret;

    esc->type = -1;
    esc->seq = seq;
    ASSERT(len >= 0, "");
    if (!len)
        return ESCNOEND;

    //dump(seq, len);
    c = seq[0];

    if (c < 0x20 || c > 0x7E)
        return ESCERR;
    esc->len = 1;

    if (c >= 0x20 && c <= 0x2F) {
        esc->type = ESCNF;
        esc->esc = c;
        if ((ret = find_nfesc_end(seq+1, len-1, &n)))
            return ret;
        esc->len += n+1;
    }

    if (c >= 0x40 && c <= 0x5F) {
        esc->type = ESCFE;
        esc->esc = c - 0x40 + 0x80;
        switch (esc->esc) {
        case CSI:
            if ((ret = find_csi_end(seq+1, len-1, &n)))
                return ret;
            esc->len += n+1;
            esc->csi = seq[esc->len-1];
            //csi_dump(esc);
            break;

        case OSC:
            if ((ret = find_osc_end(seq+1, len-1, &n)))
                return ret;
            esc->len += n+1;
            //dump(esc->seq, esc->len);
            break;
        case DCS:
            if ((ret = find_dcs_end(seq+1, len-1, &n)))
                return ret;
            esc->len += n+1;
            //dump(esc->seq, esc->len);
            break;
        }
    }

    if (c >= 0x60 && c <= 0x7E) {
        esc->type = ESCFS;
        esc->esc = c;
    }

    if (c >= 0x30 && c <= 0x3F) {
        esc->type = ESCFP;
        esc->esc = c;
    }

    return 0;
}

char* // debug
get_esc_str(struct esc_t *esc, int desc) {
    struct ctrl_info_t *info, *info2;
    int i, pos, n, ret;
    char *p;
    static char buf[BUFSIZ];

#define MYPRINT(fmt, ...) \
    pos += snprintf(buf+pos, sizeof(buf)-pos, fmt, ##__VA_ARGS__)

    pos = 0;
    buf[0] = 0;
    if (!esc->len)
        return buf;

    if (esc_info(esc->type, &info)) {
        MYPRINT("Unknown esc type");
        return buf;
    }

    MYPRINT("ESC.%s ", info->name);
    switch (esc->type) {
    case ESCFE: goto escfe;
    case ESCNF: goto escnf;
    case ESCFS: goto escfs;
    case ESCFP: goto escfp;
    default: ASSERT(0, "can't be");
    }

escfe:
    if (ctrl_info(esc->esc, &info)) {
        MYPRINT("error fe esc type");
        return buf;
    }
    MYPRINT("%s ", info->name);

    switch (esc->esc) {
    case CSI:
        if (csi_info(esc->csi, &info)) {
            MYPRINT("%c ", esc->csi);
            desc = 0;
        } else {
            MYPRINT("%s ", info->name);
        }

        if (esc->csi == RM ||  esc->csi == SM) {
            if (esc->seq[1] != '?')
                MYPRINT("unknown ");
            else
                for (i = 0; i < get_par_num(esc); i++) {
                    if (get_str_par(esc, i, &p) || p == NULL) {
                        MYPRINT("unknown ");
                        continue;
                    }
                    if (i == 0)
                        ret = parse_int(p+1, &n);
                    else
                        ret = parse_int(p, &n);

                    if (ret || (ret = mode_info(n, &info2))) {
                        MYPRINT("unknown ");
                        continue;
                    }
                    MYPRINT("%s ", info2->name);
                    if (desc)
                        MYPRINT("%s ", info2->desc);
                }
        }

        MYPRINT("[");
        n = get_par_num(esc);
        for (i = 0; i < n; i++) {
            if (get_str_par(esc, i, &p) || p == NULL)
                MYPRINT("%s", "-");
            else
                MYPRINT("%s", p);
            if (i < n-1)
                MYPRINT(",");
        }
        MYPRINT("] ");
        break;

    case OSC:
        break;

    }
    if (desc)
        MYPRINT("`%s`", info->desc);
    return buf;

escnf:
    if (nf_esc_info(esc->esc, &info))
        MYPRINT("0x%X (%c)", esc->esc, esc->esc);
    else {
        MYPRINT("%s ", info->name);
        if (desc)
            MYPRINT("`%s`", info->desc);
    }
    return buf;

escfp:
    if (fp_esc_info(esc->esc, &info))
        MYPRINT("0x%X (%c)", esc->esc, esc->esc);
    else {
        MYPRINT("%s ", info->name);
        if (desc)
            MYPRINT("`%s`", info->desc);
    }
    return buf;

escfs:
    MYPRINT("0x%X (%c)", esc->esc, esc->esc);
    return buf;

#undef MYPRINT
}

void
sgr_handle(void) {
    int n, m, v, r, g, b, i, npar;

#define SGR_PAR(idx, v, v0) \
        if (get_int_par(&esc, idx, &v, v0)) { \
            ctrl_error = ERR_PAR; \
            return; \
        }

    npar = get_par_num(&esc);
    if (npar == 0) {
        ATTR_RESET();
        return;
    }

    for (i = 0; i < npar;) {
        SGR_PAR(i++, n, 0);

        if (n >= 30 && n <= 37) {
            ATTR_FG8(n-30);
            return;
        }

        if (n >= 40 && n <= 47) {
            ATTR_BG8(n-40);
            return;
        }

        if (n >= 90 && n <= 97) {
            ATTR_FG8(n-90+8);
            return;
        }

        if (n >= 100 && n <= 107) {
            ATTR_BG8(n-100+8);
            return;
        }

        switch (n) {
        case 0 : ATTR_RESET()                    ; break;
        case 1 : ATTR_SET(ATTR_BOLD)             ; break;
        case 2 : ATTR_SET(ATTR_FAINT)            ; break;
        case 3 : ATTR_SET(ATTR_ITALIC)           ; break;
        case 4 : ATTR_SET(ATTR_UNDERLINE)        ; break;
        case 7 : ATTR_SET(ATTR_COLOR_REVERSE)    ; break;
        case 9 : ATTR_SET(ATTR_CROSSED_OUT)      ; break;
        case 22: ATTR_UNSET(ATTR_BOLD|ATTR_FAINT); break;
        case 23: ATTR_UNSET(ATTR_ITALIC)         ; break;
        case 24: ATTR_UNSET(ATTR_UNDERLINE)      ; break;
        case 27: ATTR_UNSET(ATTR_COLOR_REVERSE)  ; break;
        case 29: ATTR_UNSET(ATTR_CROSSED_OUT)    ; break;
        case 39: ATTR_SET(ATTR_DEFAULT_FG)       ; break;
        case 49: ATTR_SET(ATTR_DEFAULT_BG)       ; break;
        case 38:
        case 48:
            SGR_PAR(i++, m, 0);
            if (m != 5 && m != 2) {
                ctrl_error = ERR_UNSUPP;
                return;
            }

            if (m == 5) {
                SGR_PAR(i++, v, 0);
                if (v < 0 || v > 255) {
                    ctrl_error = ERR_PAR;
                    return;
                }
                if (n == 38)
                    ATTR_FG8(v);
                else
                    ATTR_BG8(v);
            } else {
                SGR_PAR(i++, r, 0);
                if (r < 0 || r > 255) {
                    ctrl_error = ERR_PAR;
                    return;
                }

                SGR_PAR(i++, g, 0);
                if (g < 0 || g > 255) {
                    ctrl_error = ERR_PAR;
                    return;
                }

                SGR_PAR(i++, b, 0);
                if (b < 0 || b > 255) {
                    ctrl_error = ERR_PAR;
                    return;
                }

                if (n == 38)
                    ATTR_FG24(r, g, b);
                else
                    ATTR_BG24(r, g, b);
            }
            break;

        case 58:   // Set underline color
        case 59:   // Default underline color
            break; // TODO
        default:
            ctrl_error = ERR_UNSUPP;
        }
    }
#undef SGR_PAR
}

void
mode_handle(void) {
    int i, n, ret, npar, s;
    char *p;

    if (esc.seq[1] != '?') {
        ctrl_error = ERR_UNSUPP;
        return;
    }

    //printf("%s\n", get_esc_str(&esc, 0));
    npar = get_par_num(&esc);
    s = (esc.csi == SM ? SET : RESET);
    for (i = 0; i < npar; i++) {
        if (get_str_par(&esc, i, &p) || p == NULL)
            continue;
        if (i==0)
            ret = parse_int(p+1, &n);
        else
            ret = parse_int(p, &n);
        if (ret) {
            ctrl_error = ERR_PAR;
            continue;
        }
        switch (n) {
        case M_SF:
            MODE_SET(s, MODE_SEND_FOCUS);
            break;
        case DECTCEM:
            MODE_SET(s, MODE_TEXT_CURSOR);
            break;
        case M_BP:
            MODE_SET(s, MODE_BRACKETED_PASTE);
            break;
        case M_MP:
            MODE_SET(s, MODE_MOUSE|MODE_MOUSE_PRESS);
            break;
        case M_MMP:
            MODE_SET(s, MODE_MOUSE|MODE_MOUSE_PRESS|MODE_MOUSE_MOTION_PRESS);
            break;
        case M_MMA:
            MODE_SET(s, MODE_MOUSE|MODE_MOUSE_MOTION_ANY);
            break;
        case M_ME:
            MODE_SET(s, MODE_MOUSE|MODE_MOUSE_RELEASE|MODE_MOUSE_EXT);
            break;
        case M_SC:
            lcursor(s);
            break;
        case M_ALTS:
            zt.line = (s ? zt.alt_line : zt.norm_line);
            ldirty_all();
            break;
        case M_SC_ALTS:
            lcursor(s);
            zt.line = (s ? zt.alt_line : zt.norm_line);
            if (s == SET)
                lclear_all();
            ldirty_all();
            break;
        case DECAWM:
        case DECCKM:
        case M_SBC:
        case M_MUTF8:
            break;
        default:
            ctrl_error = ERR_UNSUPP;
        }
    }
}

void
dsr_handle(void) {
    int n, ret, nw;
    char wbuf[32], *p;

    ctrl_error = ERR_PAR;
    if (get_par_num(&esc) == 0 || get_str_par(&esc, 0, &p) || p == NULL)
        return;

    if (p[0] == '?')
        ret = parse_int(p+1, &n);
    else
        ret = parse_int(p, &n);

    if (ret)
        ctrl_error = ERR_PAR;

    ctrl_error = ERR_UNSUPP;
    switch (n) {
    case 5:
        nw = snprintf(wbuf, sizeof(wbuf), "\0330n");
        break;
    case 6:
        nw = snprintf(wbuf, sizeof(wbuf), "\033[%d;%dR",
            zt.y+1, zt.x+1);
        break;
    default:
        return;
    }
    ctrl_error = 0;
    twrite(wbuf, nw);
}

void
csi_handle(void) {
    int n, m;

#define CSI_PAR(idx, v, v0) \
        if (get_int_par(&esc, idx, &v, v0)) { \
            ctrl_error = ERR_PAR; \
            return; \
        }
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
        CSI_PAR(0, n, 1);
        break;

    case ED:
    case EL:
    case TBC:
        CSI_PAR(0, n, 0);
        break;

    case DECSTBM:
        CSI_PAR(0, n, 1);
        CSI_PAR(1, m, zt.row);
        break;

    case CUP:
    case HVP:
        if (get_par_num(&esc) == 1) {
            m = 1;
            CSI_PAR(0, n, 1);
            break;
        }
        CSI_PAR(0, n, 1);
        CSI_PAR(1, m, 1);
        break;
    case DA:
        if (get_par_num(&esc) == 0)
            n = 0;
        else
            CSI_PAR(0, n, 0);
        break;
    case DECRC:
        if (get_par_num(&esc)) {
            ctrl_error = ERR_PAR;
            return;
        }
        break;
    }
#undef CSI_PAR

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
    case SGR    : sgr_handle()                ; break;
    case SU     : lscroll_up(zt.top, n)       ; break;
    case SD     : lscroll_down(zt.top, n)     ; break;
    case ECH    : lerase(zt.y, zt.x, zt.x+n-1); break;
    case SM     : mode_handle()               ; break;
    case RM     : mode_handle()               ; break;
    case DECSTBM: lsettb(n-1, m-1)            ; break;
    case CHT    : ltab(n)                     ; break;
    case CBT    : ltab(-n)                    ; break;
    case ICH    : linsert_blank(n)            ; break;
    case DCH    : ldelete_char(n)             ; break;
    case REP    : lrepeat_last(n)             ; break;
    case DECSC  : lcursor(SET)                ; break;
    case DECRC  : lcursor(RESET)              ; break;
    case DSR    : dsr_handle()                ; break;

    case DA:
        if (n == 0)
            twrite(VT102, strlen(VT102));
        break;

    case EL:
        switch (n) {
        case 0: lerase(zt.y, zt.x, zt.col-1); break;
        case 1: lerase(zt.y, 0, zt.x)       ; break;
        case 2: lerase(zt.y, 0, zt.col-1)   ; break;
        default: ctrl_error = ERR_UNSUPP;
        }
        break;

    case ED:
        switch (n) {
        case 0: lclear(zt.y, zt.x, zt.row-1, zt.col-1); break;
        case 1: lclear(0, 0, zt.y , zt.x)             ; break;
        case 2: lclear_all(); lmoveto(0,0)            ; break;
        case 3: lclear_all(); lmoveto(0,0)            ; break;
        default: ctrl_error = ERR_UNSUPP;
        }
        break;

    case TBC:
        switch (n) {
        case 0: zt.tabs[zt.x] = 0; break;
        case 3: ltab_clear()     ; break;
        default: ctrl_error = ERR_UNSUPP;
        }
        break;

    case WINMAN: // Window Manipulation
    case DECLL:  // load LEDs
    case MC:     // Media Copy
        break;   // TODO
    default:
        ctrl_error = ERR_UNSUPP;
    }
}

void
esc_handle(unsigned char *buf, int len) {
    esc_error = esc_parse(buf, len, &esc);
    ASSERT(len >= 0, "");
    if (esc_error) {
        if (esc_error != ESCERR)
            ctrl_error = ERR_RETRY;
        else
            ctrl_error = ERR_ESC;
        return;
    }

    switch (esc.type) {
    case ESCFE: goto escfe;
    case ESCNF: goto escnf;
    case ESCFS: goto escfs;
    case ESCFP: goto escfp;
    default: ASSERT(0, "can't be");
    }

escfe:
    switch (esc.esc) {
    case CSI:
        csi_handle();
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
    case OSC:
        //printf("%d\n", get_par_num(&esc));
        //dump(esc.seq, esc.len);
        break;
    case DCS:
        //dump(esc.seq, esc.len);
        break;
    default:
        ctrl_error = ERR_UNSUPP;
    }
    return;

//TODO
escnf:
    switch (esc.esc) {
    case NF_GZD4:
    case NF_G1D4:
    case NF_G2D4:
    case NF_G3D4:
        break;
    default:
        ctrl_error = ERR_UNSUPP;
    }
    return;

//TODO
escfp:
    switch (esc.esc) {
    case FP_DECSC:
        lcursor(SET);
        break;
    case FP_DECRC:
        lcursor(RESET);
        break;
    case FP_DECPAM:
    case FP_DECPNM:
        break;
    default:
        ctrl_error = ERR_UNSUPP;
    }
    return;

//TODO
escfs:
    ctrl_error = ERR_UNSUPP;

}

void
ctrl_handle(unsigned char *buf, int len) {
    unsigned char c = buf[0];
    struct ctrl_info_t *info = NULL;

    ASSERT(len >= 0, "");
    ctrl_error = 0;
    zt.lastc = 0;
    bzero(&esc, sizeof(struct esc_t));
    switch (c) {
    case ESC: esc_handle(buf+1, len-1); break;
    case LF : lnew()                  ; break;
    case CR : lmoveto(zt.y, 0)        ; break;
    case HT : ltab(1)                 ; break;
    case HTS: zt.tabs[zt.x] = 1       ; break;
    case BS:
    case CCH:     // CCH: Cancel character, intended to eliminate
                  //      ambiguity about meaning of BS.
        lmoveto(zt.y, zt.x-1);
        break;
    case BEL:     // bell, allert
    case SS2:
    case SS3:
    case BPH:     // break permitted here
    case PAD:     // padding character
    case SOS:     // Followed by a control string terminated by ST
    case ST:
    case SO:      // Switch to an alternative character set.
    case SI:      // Return to regular character set after Shift Out.
        break;    // TODO
    default:
        ctrl_error = ERR_UNSUPP;
    }
    if (!ctrl_error || ctrl_error == ERR_RETRY)
        return;

    switch (ctrl_error) {
    case ERR_ESC:
        printf("can not find esc\n");
        break;
    case ERR_PAR:
        printf("error parameter: %s\n", get_esc_str(&esc, 1));
        break;
    case ERR_UNSUPP:
        printf("unsupported: ");
        if (c != ESC) {
            ASSERT(ctrl_info(c, &info) == 0, "");
            printf("%s %s\n", info->name, info->desc);
        } else
            printf("%s\n", get_esc_str(&esc, 1));
        break;
    }
}

void // debug
tdump(void) {
    static int frame = 0;
    int i, j, n;
    struct char_t c;
    char buf[10];
    char *space = "         ";

    printf("frame: %d, size: %dx%d, margin: %d-%d, cur: %dx%d\n",
        frame, zt.row, zt.col, zt.top, zt.bot, zt.y, zt.x);
    frame++;

    printf("%s", space);
    for (i = 0; i < zt.col; i++)
        printf("*");
    printf("\n");
    printf("%s", space);
    for (i = 0; i < zt.col; i++)
        if (i % 10 == 0) {
            n = snprintf(buf, sizeof(buf), "%d", i);
            i += n-1;
            printf("%s", buf);
        } else
            printf(" ");
    printf("\n");
    for (i = 0; i < zt.row; i++) {
        printf("[%d][%3d] ", zt.dirty[i], i);
        for (j = 0; j < zt.col; j++) {
            c = zt.line[i][j];
            //printf("%d", c.width);
            if (isprint(c.c))
                printf("%c", c.c);
            else
                printf("%X ", c.c);
        }
        printf("\n");
    }
    printf("%s", space);
    for (i = 0; i < zt.col; i++)
        printf("*");
    printf("\n");
    fflush(stdout);
}

void // debug
ldump(int y, int x1, int x2) {
    int i;
    struct char_t c;
    if (x1 < 0)
        x1 = 0;
    if (x2 < 0)
        x2 = zt.col-1;

    printf("[%d] [%d-%d] ", zt.dirty[y], x1, x2);
    for (i = x1; i <= x2; i++) {
        printf("[");
        c = zt.line[y][i];
        if (isprint(c.c))
            printf("%c,", c.c);
        else
            printf("%X,", c.c);
        if (ATTR_HAS(c, ATTR_DEFAULT_FG))
            printf("-");
        else
            printf("%d", c.fg.c8);
        printf(",");
        if (ATTR_HAS(c, ATTR_DEFAULT_BG))
            printf("-");
        else
            printf("%d", c.bg.c8);
        printf("]");
    }
    printf("\n");
    fflush(stdout);
}


void // debug
cdump(unsigned char c) {
    struct ctrl_info_t *info = NULL;

    if (c == ESC)
        printf("[%3d] %-s\n", esc.len, get_esc_str(&esc, 0));
    else {
        ASSERT(ctrl_info(c, &info) == 0, "");
        printf("[%3d] %-s\n", 1, info->name);
    }
    fflush(stdout);
}

int
parse(unsigned char *buf, int len, int force) {
    uint32_t u;
    int nread = 0, n = 0, ulen = 0,
        char_bytes = 0, ctrl_bytes = 0,
        total_char_bytes = 0, total_ctrl_bytes = 0;
    static int retries = 0, osc_no_end = 0;

    if (!len)
        return 0;

#ifdef DEBUG_CTRL
    static int count = 0;
    unsigned char last_c = 0;
#endif

#if defined(DEBUG_CTRL_TERM) || defined(DEBUG_TERM)
    printf("DISPLAY: %dx%d\n", zt.row, zt.col);
    printf("CURSOR: %dx%d\n", zt.y, zt.x);
    printf("MARGIN: %d-%d\n", zt.top, zt.bot);
#endif

    if (force)
        printf("force read\n");

    if (osc_no_end) {
        if (find_osc_end(buf, len, &nread)) {
            nread = len;
            goto retry;
        }
        osc_no_end = 0;
        nread++;
    }

#ifdef DEBUG_BUF
    printf("nread: %d\n", nread);
    dump(buf, len);
#endif

    for (n = 0; nread < len; nread += n) {
        if (ISCTRL(buf[nread])) {
#ifdef DEBUG_CTRL
            cdump(last_c);
            last_c = buf[nread];
#endif

#ifdef DEBUG_CTRL_TERM
            tdump();
#endif

            total_char_bytes += char_bytes;
            total_ctrl_bytes += ctrl_bytes;
            char_bytes = 0;
            ctrl_bytes = 0;
            ctrl_handle(buf+nread, len-nread);
            if (!force) {
                if (ctrl_error == ERR_RETRY) {
                    if (esc_error == ESCOSCNOEND) {
                        osc_no_end = 1;
                        nread = len;
                        goto retry;
                    }
                    if (retries < RETRY_MAX) {
                        retries++;
                        goto retry;
                    }
                    printf("reach max retry for ctrl\n");
                }
            }

            retries = 0;
            n = esc.len + 1;
            ctrl_bytes += n;
            continue;
        }

#ifndef DEBUG_NOUTF8
        if (utf8_decode(buf+nread, len-nread, &u, &ulen)) {
            if (!force) {
                if (retries < RETRY_MAX) {
                    retries++;
                    goto retry;
                }
                printf("reach max retry for utf8\n");
            }
            u = buf[nread];
            ulen = 1;
        }
        retries = 0;
#else
        u = buf[nread];
        ulen = 1;
#endif

#ifdef DEBUG_WRITE
    if (isprint(u))
        printf("%c", u);
#endif
        lwrite(u);
        n = ulen;
        char_bytes += n;
    }
#ifdef DEBUG_WRITE
    printf("\n");
#endif

retry:
    total_char_bytes += char_bytes;
    total_ctrl_bytes += ctrl_bytes;

#ifdef DEBUG_CTRL
    cdump(last_c);
    printf("[%05d] TOTAL: %d, READ: %d, CTRL: %d, CHAR: %d\n", count++,
        len, nread, total_ctrl_bytes, total_char_bytes);
#endif

#ifdef DEBUG_TERM
    tdump();
#endif

    ASSERT(retries <= RETRY_MAX && retries >= 0, "");
    if (force)
        ASSERT(nread == len,
            "force read error: nread:%d, len: %d, ctrl: %d, char: %d",
            nread, len, total_ctrl_bytes, total_char_bytes);

    if (retries == RETRY_MAX)
        retries = 0;

    ASSERT(nread <= len, "nread: %d, len: %d, bytes: %d, ctrl_byte: %d",
            nread, len, total_char_bytes, total_ctrl_bytes);
    if (force)
        ASSERT(nread == len, "nread:%d, len: %d", nread, len);

    return nread;
}
