#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "zt.h"
#include "tools.h"
#include "ctrl.h"
#include "utf8.h"

int ctrl_error, esc_error;
struct Esc esc;

/*
  References
  https://en.wikipedia.org/wiki/ANSI_escape_code
  https://en.wikipedia.org/wiki/C0_and_C1_control_codes
  https://vt100.net/docs
  https://vt100.net/docs/vt102-ug/contents.html
  https://vt100.net/docs/vt220-rm/contents.html
*/

#define PRIMARY_DA "\033[?6c" // vt102
#define SECONDARY_DA "\033[>1;95;0c" // vt220

#define ERR_OTHER  -1
#define ERR_RETRY   1
#define ERR_UNSUPP  2
#define ERR_ESC     3
#define ERR_PAR     4

#define RETRY_MAX 3

// for debug
//#define CTRL_DUMP
//#define TERM_DUMP
//#define NOUTF8

void tdump(void);

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
            ret = str_to_dec(p+1, &n);
        else
            ret = str_to_dec(p, &n);
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
        ret = str_to_dec(p+1, &n);
    else
        ret = str_to_dec(p, &n);

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
        case DECRC  :
            if (get_par_num(&esc))
                ctrl_error = ERR_PAR;
            else
                lcursor(RESET);
            break;
        case DSR    : dsr_handle()                ; break;
        case DA:
            if (get_par_num(&esc) == 0 || esc.seq[1] != '>')
                twrite(PRIMARY_DA, strlen(PRIMARY_DA));
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
    struct CtrlInfo *info = NULL;

    ASSERT(len >= 0, "");
    ctrl_error = 0;
    zt.lastc = 0;
    esc_reset(&esc);
    switch (c) {
        case ESC: esc_handle(buf+1, len-1); break;
        case LF : lnew()                  ; break;
        case CR : lmoveto(zt.y, 0)        ; break;
        case HT : ltab(1)                 ; break;
        case HTS: zt.tabs[zt.x] = 1       ; break;
        case BS:
        case CCH:     // CCH: Cancel character, intended to eliminate ambiguity about meaning of BS.
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
                ASSERT(get_ctrl_info(c, &info) == 0, "");
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
    struct MyChar c;
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
            if (ISPRINTABLE(c.c))
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
    struct MyChar c;
    if (x1 < 0)
        x1 = 0;
    if (x2 < 0)
        x2 = zt.col-1;

    printf("[%d] [%d-%d] ", zt.dirty[y], x1, x2);
    for (i = x1; i <= x2; i++) {
        printf("[");
        c = zt.line[y][i];
        if (ISPRINTABLE(c.c))
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
cdump(unsigned char c, int n) {
    struct CtrlInfo *info = NULL;

    if (c == ESC)
        printf("[%3d] %-30s", esc.len, get_esc_str(&esc, 0));
    else {
        ASSERT(get_ctrl_info(c, &info) == 0, "");
        printf("[%3d] %-30s", 1, info->name);
    }
    printf("%5d %dx%d\n", n, zt.y, zt.x);
    //ldump(11, -1, -1);
    //ldump(zt.row-2, -1, -1);
    fflush(stdout);
}

int
parse(unsigned char *buf, int len, int force) {
    MyRune u;
    int nread, n, ulen, char_bytes, ctrl_bytes,
    total_char_bytes, total_ctrl_bytes;
    static int retries = 0, osc_no_end = 0;

    if (!len)
        return 0;

#ifdef CTRL_DUMP
    unsigned char last_c = 0;
    static int count = 0;
    printf("\n-------CTRL--------\n");
    printf("TTY[%d]: %d\n", count, len);
    printf("DISPLAY: %dx%d\n", zt.row, zt.col);
    printf("CURSOR: %dx%d\n", zt.y, zt.x);
    printf("MARGIN: %d-%d\n", zt.top, zt.bot);
    count++;
#endif

    char_bytes = ctrl_bytes =
    total_char_bytes = total_ctrl_bytes = 0;

    if (force)
        printf("force read\n");

    nread = 0;
    if (osc_no_end) {
        if (find_osc_end(buf, len, &nread)) {
            nread = len;
            goto retry;
        }
        osc_no_end = 0;
        nread++;
    }
    //dump(buf, len);

    for (; nread < len; nread += n) {

        if (ISCTRL(buf[nread])) {
#ifdef CTRL_DUMP
            cdump(last_c, char_bytes);
            last_c = buf[nread];
#endif

#ifdef TERM_DUMP
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

#ifndef NOUTF8
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
        lwrite(u);
        n = ulen;
        char_bytes += n;
    }

retry:
    total_char_bytes += char_bytes;
    total_ctrl_bytes += ctrl_bytes;

#ifdef CTRL_DUMP
    cdump(last_c, char_bytes);
    printf("TOTAL: %d, READ: %d, CTRL: %d, CHAR: %d\n",
        len, nread, total_ctrl_bytes, total_char_bytes);
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
