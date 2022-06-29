#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "zt.h"
#include "tool.h"
#include "ctrl.h"
#include "utf8.h"

int ctrl_error, esc_error;
struct Esc esc;

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
//#define CTRL_TERM_DUMP
//#define NOUTF8

void tdump(void);

void 
sgr_handle(void) {
    int n, m, v, r, g, b, i, npar;

#define SGR_PAR(idx, v, v0) \
        if (get_int_par(&esc, idx, &v, v0)) { \
            ctrl_error = ERR_PAR; \
            return;\
        }
    npar = get_par_num(&esc);
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

            CASE(0,  ATTR_RESET())
            CASE(1,  ATTR_SET(ATTR_BOLD))
            CASE(2,  ATTR_SET(ATTR_FAINT))
            CASE(3,  ATTR_SET(ATTR_ITALIC))
            CASE(4,  ATTR_SET(ATTR_UNDERLINE))
            CASE(7,  ATTR_SET(ATTR_COLOR_REVERSE))
            CASE(9,  ATTR_SET(ATTR_CROSSED_OUT))
            CASE(22, ATTR_UNSET(ATTR_BOLD|ATTR_FAINT))
            CASE(23, ATTR_UNSET(ATTR_ITALIC))
            CASE(24, ATTR_UNSET(ATTR_UNDERLINE))
            CASE(27, ATTR_UNSET(ATTR_COLOR_REVERSE))
            CASE(29, ATTR_UNSET(ATTR_CROSSED_OUT))
            CASE(39, ATTR_SET(ATTR_DEFAULT_FG))
            CASE(49, ATTR_SET(ATTR_DEFAULT_BG))
                
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
            CASE(M_SF,      MODE_SET(s, MODE_SEND_FOCUS))
            CASE(DECTCEM,   MODE_SET(s, MODE_TEXT_CURSOR))
            CASE(M_BP,      MODE_SET(s, MODE_BRACKETED_PASTE))
            CASE(M_MP,
                MODE_SET(s, MODE_MOUSE|MODE_MOUSE_PRESS))
            CASE(M_MMP,
                MODE_SET(s, MODE_MOUSE|MODE_MOUSE_PRESS|MODE_MOUSE_MOTION_PRESS))
            CASE(M_MMA,
                MODE_SET(s, MODE_MOUSE|MODE_MOUSE_MOTION_ANY))
            CASE(M_ME,
                MODE_SET(s, MODE_MOUSE|MODE_MOUSE_RELEASE|MODE_MOUSE_EXT))
            CASE(M_SC,   lcursor(s))
            CASE(M_ALTS, 
                zt.line = (s ? zt.alt_line : zt.norm_line);
                ldirty_all())
            CASE(M_SC_ALTS,
                lcursor(s);
                zt.line = (s ? zt.alt_line : zt.norm_line);
                if (s == SET)
                    lclear_all();
                ldirty_all())

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
        CASE(5, nw = snprintf(wbuf, sizeof(wbuf), "\0330n"))
        CASE(6,
            nw = snprintf(wbuf, sizeof(wbuf), "\033[%d;%dR",
                zt.y+1, zt.x+1))
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
            return;\
        }
    switch (esc.csi) {
        CASE(CUF,        CSI_PAR(0, n, 1))
        CASE(CUB,        CSI_PAR(0, n, 1))
        CASE(CUU,        CSI_PAR(0, n, 1))
        CASE(CUD,        CSI_PAR(0, n, 1))
        CASE(CPL,        CSI_PAR(0, n, 1))
        CASE(CNL,        CSI_PAR(0, n, 1))
        CASE(IL,         CSI_PAR(0, n, 1))
        CASE(DL,         CSI_PAR(0, n, 1))
        CASE(DCH,        CSI_PAR(0, n, 1))
        CASE(CHA,        CSI_PAR(0, n, 1))
        CASE(HPA,        CSI_PAR(0, n, 1))
        CASE(VPA,        CSI_PAR(0, n, 1))
        CASE(VPR,        CSI_PAR(0, n, 1))
        CASE(HPR,        CSI_PAR(0, n, 1))
        CASE(SU,         CSI_PAR(0, n, 1))
        CASE(SD,         CSI_PAR(0, n, 1))
        CASE(ECH,        CSI_PAR(0, n, 1))
        CASE(CHT,        CSI_PAR(0, n, 1))
        CASE(CBT,        CSI_PAR(0, n, 1))
        CASE(ICH,        CSI_PAR(0, n, 1))
        CASE(REP,        CSI_PAR(0, n, 1))

        CASE(ED,         CSI_PAR(0, n, 0))
        CASE(EL,         CSI_PAR(0, n, 0))
        CASE(TBC,        CSI_PAR(0, n, 0))

        CASE(DECSTBM,    CSI_PAR(0, n, 1);
                         CSI_PAR(1, m, zt.row))

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

        CASE(CUF,       lmoveto(zt.y  , zt.x+n ))
        CASE(CUB,       lmoveto(zt.y  , zt.x-n ))
        CASE(CUU,       lmoveto(zt.y-n, zt.x   ))
        CASE(CUD,       lmoveto(zt.y+n, zt.x   ))
        CASE(CPL,       lmoveto(zt.y-n, 0      ))
        CASE(CNL,       lmoveto(zt.y+n, 0      ))
        CASE(CUP,       lmoveto(n-1   , m-1    ))
        CASE(HVP,       lmoveto(n-1   , m-1    ))
        CASE(CHA,       lmoveto(zt.y  , n-1    ))
        CASE(HPA,       lmoveto(zt.y  , n-1    ))
        CASE(VPA,       lmoveto(n-1   , zt.x   ))
        CASE(HPR,       lmoveto(zt.y  , zt.x+n ))
        CASE(VPR,       lmoveto(zt.y+n, zt.y    ))
        CASE(IL,        linsert(n))
        CASE(DL,        ldelete(n))
        CASE(SGR,       sgr_handle())
        CASE(SU,        lscroll_up(zt.top, n))
        CASE(SD,        lscroll_down(zt.top, n))
        CASE(ECH,       lerase(zt.y, zt.x, zt.x+n-1));
        CASE(SM,        mode_handle())
        CASE(RM,        mode_handle())
        CASE(DECSTBM,   lsettb(n-1, m-1))
        CASE(CHT,       ltab(n))
        CASE(CBT,       ltab(-n))
        CASE(ICH,       linsert_blank(n))
        CASE(DCH,       ldelete_char(n))
        CASE(REP,       lrepeat_last(n))
        CASE(DECSC,     lcursor(SET))
        CASE(DECRC,     lcursor(RESET))
        CASE(DSR,       dsr_handle())
        case DA:
            if (get_par_num(&esc) == 0 || esc.seq[1] != '>') 
                twrite(PRIMARY_DA, strlen(PRIMARY_DA));
            break;

        case EL:
            switch (n) {
                CASE(0, lerase(zt.y, zt.x, zt.col-1))
                CASE(1, lerase(zt.y, 0, zt.x))
                CASE(2, lerase(zt.y, 0, zt.col-1))
                default: ctrl_error = ERR_UNSUPP;
            }
            break;

        case ED:
            switch (n) {
                CASE(0, lclear(zt.y, zt.x, zt.row-1, zt.col-1))
                CASE(1, lclear(0, 0, zt.y , zt.x ))
                CASE(2, lclear_all(); lmoveto(0,0))
                CASE(3, lclear_all(); lmoveto(0,0))
                default: ctrl_error = ERR_UNSUPP;
            }
            break;

        case TBC:
            switch (n) {
                CASE(0, zt.tabs[zt.x] = 0)
                CASE(3, ltab_clear())
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
        CASE(CSI, csi_handle())
        CASE(HTS, zt.tabs[zt.x] = 1)
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
        CASE(FP_DECSC, lcursor(SET))
        CASE(FP_DECRC, lcursor(RESET))
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
        CASE(ESC, esc_handle(buf+1, len-1))
        CASE(LF,  lnew())
        CASE(CR,  lmoveto(zt.y, 0))
        CASE(HT,  ltab(1))
        CASE(HTS, zt.tabs[zt.x] = 1)
        CASE(BS,  lmoveto(zt.y, zt.x-1))
        CASE(CCH, lmoveto(zt.y, zt.x-1))
        // CCH: Cancel character,
        // intended to eliminate ambiguity about meaning of BS.

        case BEL:     // bell, allert
        case SS2:
        case SS3:
        case BPH:     // break permitted here
        case PAD:     // padding character
        case SOS:     // Followed by a control string terminated by ST
        case ST:
        case SO:      // Switch to an alternative character set.
        case SI:      // Return to regular character set after Shift Out.
            break; // TODO
        default:
            ctrl_error = ERR_UNSUPP;
    }
    if (!ctrl_error || ctrl_error == ERR_RETRY)
        return;

    switch (ctrl_error) {
        CASE(ERR_ESC, printf("can not find esc\n"))
        CASE(ERR_PAR, printf("error parameter: %s\n", get_esc_str(&esc, 1)))
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

#ifdef CTRL_TERM_DUMP
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
#endif

#ifdef CTRL_DUMP
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
