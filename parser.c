#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "zt.h"
#include "ctrl.h"

int ctrl_error;
struct Esc esc;

void lclear(int, int, int, int);
void lerase(int, int, int);
void lscroll_up(int, int);
void lscroll_down(int, int);
void lnew(void);
void lwrite(uint32_t);
void linsert(int);
void ldelete(int);
void lmove(int, int);
void lsettb(int, int);
void ltab(int);
void ltab_clear(void);
void twrite(char*, int);
void lmemmove(int, int, int, int);
int utf8_decode(unsigned char*, int,
    uint32_t*, int*);

#define ERR_OTHER  -1
#define ERR_RETRY   1
#define ERR_UNSUPP  2
#define ERR_ESC     3
#define ERR_PAR     4

#define RETRY_MAX 3

//#define CTRL_DEBUG
//#define TERM_DEBUG

void tdump(void);

void 
sgr_handle(void) {
    int n, m, v, r, g, b;

#define SGR_PAR(idx, v, v0) \
        if (csi_int_par(&esc, idx, &v, v0)) { \
            ctrl_error = ERR_PAR; \
            return;\
        }
    SGR_PAR(0, n, 0);

    if (n >= 30 && n <= 37) {
        SET_COLOR8(zt.attr.fg, n-30);
        ATTR_UNSET(zt.attr, ATTR_DEFAULT_FG);
        return;
    }

    if (n >= 40 && n <= 47) {
        SET_COLOR8(zt.attr.bg, n-40);
        ATTR_UNSET(zt.attr, ATTR_DEFAULT_BG);
        return;
    }

    if (n >= 90 && n <= 97) {
        SET_COLOR8(zt.attr.fg, n-90+8);
        ATTR_UNSET(zt.attr, ATTR_DEFAULT_FG);
        return;
    }

    if (n >= 100 && n <= 107) {
        SET_COLOR8(zt.attr.bg, n-100+8);
        ATTR_UNSET(zt.attr, ATTR_DEFAULT_BG);
        return;
    }

    switch (n) {

        CASE(0,  ATTR_RESET(zt.attr))
        CASE(4,  ATTR_SET(zt.attr,    ATTR_UNDERLINE))
        CASE(7,  ATTR_SET(zt.attr,    ATTR_COLOR_REVERSE))
        CASE(24, ATTR_UNSET(zt.attr,  ATTR_UNDERLINE))
        CASE(27, ATTR_UNSET(zt.attr,  ATTR_COLOR_REVERSE))
        CASE(39, ATTR_SET(zt.attr,    ATTR_DEFAULT_FG))
        CASE(49, ATTR_SET(zt.attr,    ATTR_DEFAULT_BG))
            
        case 38:
        case 48:
            SGR_PAR(1, m, 0);
            if (m != 5 && m != 2) {
                ctrl_error = ERR_UNSUPP;
                return;
            }

            if (m == 5) {
                SGR_PAR(2, v, 0);
                if (v < 0 || v > 255) {
                    ctrl_error = ERR_PAR;
                    return;
                }
                if (n == 38) 
                    SET_COLOR8(zt.attr.fg, v);
                else
                    SET_COLOR8(zt.attr.bg, v);
            } else {
                SGR_PAR(2, r, 0);
                if (r < 0 || r > 255) {
                    ctrl_error = ERR_PAR;
                    return;
                }

                SGR_PAR(3, g, 0);
                if (g < 0 || g > 255) {
                    ctrl_error = ERR_PAR;
                    return;
                }

                SGR_PAR(4, b, 0);
                if (b < 0 || b > 255) {
                    ctrl_error = ERR_PAR;
                    return;
                }
                
                if (n == 38)
                    SET_COLOR24(zt.attr.fg, r, g, b);
                else
                    SET_COLOR24(zt.attr.bg, r, b, b);
            }

            if (n == 38)
                ATTR_UNSET(zt.attr, ATTR_DEFAULT_FG);
            else
                ATTR_UNSET(zt.attr, ATTR_DEFAULT_BG);
            break;

        case 1:    // Bold
        case 2:    // Faint
        case 58:   // Set underline color
        case 59:   // Default underline color
            break; // TODO
        default:
            ctrl_error = ERR_UNSUPP;
    }

#undef SGR_PAR
}

void
mode_handle() {
    int i, n, ret;
    char *p;

    if (esc.seq[1] != '?') {
        ctrl_error = ERR_UNSUPP;
        return;
    }

    for (i=0; i<csi_par_num(&esc); i++) {
        if (csi_str_par(&esc, i, &p) || p == NULL)
            continue;
        if (i==0)
            ret = str_to_dec(p+1, &n);
        else
            ret = str_to_dec(p, &n);
        if (ret)
            continue;
        switch (n) {
            CASE(M_SF,      MODE_SET(esc.csi, MODE_SEND_FOCUS))
            CASE(DECTCEM,   MODE_SET(esc.csi, MODE_TEXT_CURSOR))
            CASE(M_RBP,     MODE_SET(esc.csi, MODE_REPORT_BUTTON))
            CASE(M_RMBP,    MODE_SET(esc.csi, MODE_REPORT_BUTTON|MODE_REPORT_MOTION))
            CASE(M_EAMM,    MODE_SET(esc.csi, MODE_REPORT_MOTION))
            CASE(M_ER,      MODE_SET(esc.csi, MODE_REPORT_BUTTON|MODE_REPORT_MOTION))
            CASE(M_BP,      MODE_SET(esc.csi, MODE_BRACKETED_PASTE))
            case DECAWM:
            case DECCKM:
            case M_SS:
            case M_SBC:
            case M_MUTF8:
                break;
            default:
                ctrl_error = ERR_UNSUPP;
        }
    }
}

void
csi_handle(void) {
    int n, m, nw=0;
    char wbuf[32];

#define CSI_PAR(idx, v, v0) \
        if (csi_int_par(&esc, idx, &v, v0)) { \
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
        CASE(SU,         CSI_PAR(0, n, 1))
        CASE(SD,         CSI_PAR(0, n, 1))
        CASE(ECH,        CSI_PAR(0, n, 1))
        CASE(CHT,        CSI_PAR(0, n, 1))
        CASE(CBT,        CSI_PAR(0, n, 1))

        CASE(ED,         CSI_PAR(0, n, 0))
        CASE(EL,         CSI_PAR(0, n, 0))
        CASE(TBC,        CSI_PAR(0, n, 0))

        CASE(DECSTBM,    CSI_PAR(0, n, 1);
                         CSI_PAR(1, m, zt.row))

        case CUP:
            if (csi_par_num(&esc) == 1) {
                m = 1;
                CSI_PAR(0, n, 1);
                break;
            }
            CSI_PAR(0, n, 1);
            CSI_PAR(1, m, 1);
            break;
        case DSR:
            CSI_PAR(0, n, 0);
            nw = 0;
            if (n == 6)
                nw = snprintf(wbuf, sizeof(wbuf), "\033[%d;%dR",
                     zt.y+1, zt.x+1);
            break;
    }
#undef CSI_PAR

    switch (esc.csi) {

        CASE(CUF,       lmove(zt.y  , zt.x+n ))
        CASE(CUB,       lmove(zt.y  , zt.x-n ))
        CASE(CUU,       lmove(zt.y-n, zt.x   ))
        CASE(CUD,       lmove(zt.y+n, zt.x   ))
        CASE(CPL,       lmove(zt.y-n, 0      ))
        CASE(CNL,       lmove(zt.y+n, 0      ))
        CASE(CUP,       lmove(n-1   , m-1    ))
        CASE(CHA,       lmove(zt.y  , n-1    ))
        CASE(HPA,       lmove(zt.y  , n-1    ))
        CASE(VPA,       lmove(n-1   , zt.x   ))
        CASE(DA,        twrite(VTIDEN, strlen(VTIDEN)))
        CASE(DSR,       twrite(wbuf, nw))
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
                CASE(1, lclear(   0, 0   , zt.y , zt.x ))
                CASE(2, lclear(   0, 0   , zt.row-1, zt.col-1); lmove(0,0))
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

        case DCH:
            if (zt.x + n >= zt.col) {
                lerase(zt.y, zt.x, zt.col-1);
                break;
            }
            lmemmove(zt.y, zt.x, zt.x+n, zt.col-zt.x-n);
            lerase(zt.y, zt.col-n, zt.col-1);
            break;

        case DECLL:  // load LEDs
            break;  // TODO
        default:
            ctrl_error = ERR_UNSUPP;
    }
}

void
esc_handle(unsigned char *buf, int len) {
    int ret;

    ret = esc_parse(buf, len, &esc);
    if (ret) {
        if (ret == EILSEQ)
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
    switch (esc.fe) {
        CASE(CSI, csi_handle())
        CASE(HTS, zt.tabs[zt.x] = 1)
        case RI:
            if (zt.y == zt.top) {
                lscroll_down(zt.top, 1);
                zt.y = zt.top;
            }
            else
                lmove(zt.y-1, zt.x);
            break;
        case OSC:
            //dump(esc.seq, esc.len);
            //printf("OSC: %s\n", esc.fe.osc);
            break;
        default:
            ctrl_error = ERR_UNSUPP;
    }
    return;

escfp:
escfs:
    ctrl_error = ERR_UNSUPP;
escnf:
    return;
}

void
ctrl_handle(unsigned char *buf, int len) {
    unsigned char c = buf[0];
    struct CtrlInfo *info = NULL;
    int i, n, ret;
    char *p;

    ctrl_error = 0;
    esc_reset(&esc);
    switch (c) {
        CASE(ESC, esc_handle(buf+1, len-1))
        CASE(LF,  lnew())
        CASE(CR,  lmove(zt.y, 0))
        CASE(HT,  ltab(1))
        CASE(HTS, zt.tabs[zt.x] = 1)

        case CCH:     // Cancel character,
                           // intended to eliminate ambiguity about meaning of BS.
        case BS:
            lmove(zt.y, zt.x-1);
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
                break;
            } 
            printf("%s", get_esc_str(&esc, 1));
            if (esc.type != ESCFE
             || esc.fe != CSI
             || !(esc.csi == RM || esc.csi == SM)) {
                printf("\n");
                break;
            }

            printf(": ");

            if (esc.seq[1] != '?') {
                printf("unknown\n");
                break;
            }

            for (i=0; i<csi_par_num(&esc); i++) {
                if (csi_str_par(&esc, i, &p) || p == NULL)
                    continue;

                if (i == 0)
                    ret = str_to_dec(p+1, &n);
                else
                    ret = str_to_dec(p, &n);

                if (ret || (ret = get_mode_info(n, &info))) {
                    printf("unknown,");
                    continue;
                }
                printf("%s: %s,", info->name, info->desc);
            }
            printf("\n");
    }
}

void // debug
tdump(void) {
    static int frame = 0;
    int i, j, n;
    char buf[10];
    char *space = "         ";
    
    printf("frame: %d, size: %dx%d, margin: %d-%d, cur: %dx%d\n",
        frame, zt.row, zt.col, zt.top, zt.bot, zt.y, zt.x);
    frame++;

#define _tdump(fmt, ...) { \
    printf("%s", space); \
    for (i=0; i<zt.col; i++) \
        printf("*"); \
    printf("\n"); \
    printf("%s", space); \
    for (i=0; i<zt.col; i++) \
        if (i % 10 == 0) { \
            n = snprintf(buf, sizeof(buf), "%d", i);\
            i += n-1; \
            printf("%s", buf); \
        } else \
            printf(" "); \
    printf("\n"); \
    for (i=0; i<zt.row; i++) { \
        printf("[%d][%3d] ", zt.dirty[i], i); \
        for (j=0; j<zt.col; j++) \
            printf(fmt, ##__VA_ARGS__); \
        printf("\n"); \
    } \
    printf("%s", space); \
    for (i=0; i<zt.col; i++) \
        printf("*"); \
    printf("\n"); \
    fflush(stdout);\
}

    _tdump("%c", zt.line[i][j].c);

}

void // debug
ldump(int y, int x1, int x2) {
    int i;
    unsigned char c;
    struct MyAttr a;
    if (x1 < 0)
        x1 = 0;
    if (x2 < 0)
        x2 = zt.col-1;

    printf("[%d] [%d-%d] ", zt.dirty[y], x1, x2);
    for (i=x1; i<=x2; i++) {
        printf("[");
        c = zt.line[y][x1].c;
        a = zt.line[y][x1].attr;
        printf("%c,", c);
        if (ATTR_HAS(a, ATTR_DEFAULT_FG))
            printf("-");
        else
            printf("%d", a.fg.c8);
        printf(",");
        if (ATTR_HAS(a, ATTR_DEFAULT_BG))
            printf("-");
        else
            printf("%d", a.bg.c8);
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
    //ldump(10, -1, 50);
    fflush(stdout);
}

int
parse(unsigned char *buf, int len, int force) {
    uint32_t u;
    int nread, n, ulen, char_bytes, ctrl_bytes,
    total_char_bytes, total_ctrl_bytes;
    static int retries = 0;

    if (!len)
        return 0;

#ifdef CTRL_DEBUG
    unsigned char last_c = 0;
    printf("\n-------CTRL--------\n");
    printf("TTY: %d\n", len);
    printf("DISPLAY: %dx%d\n", zt.row, zt.col);
    printf("CURSOR: %dx%d\n", zt.y, zt.x);
#endif

    char_bytes = ctrl_bytes =
    total_char_bytes = total_ctrl_bytes = 0;

    if (force)
        printf("force read\n");

    for (nread=0; nread<len; nread+=n) {

        if (ISCTRL(buf[nread])) {
#ifdef CTRL_DEBUG
            cdump(last_c, char_bytes);
            last_c = buf[nread];
#endif
    
#ifdef TERM_DEBUG
     //       tdump();
#endif

            total_char_bytes += char_bytes;
            total_ctrl_bytes += ctrl_bytes;
            char_bytes = 0;
            ctrl_bytes = 0;
            ctrl_handle(buf+nread, len-nread);
            if (!force) {
                if (ctrl_error == ERR_RETRY) {
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

#ifndef TERM_DEBUG
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

#ifdef CTRL_DEBUG
    cdump(last_c, char_bytes);
    printf("CTRL: %d, CHAR: %d\n",
        total_ctrl_bytes, total_char_bytes);
#endif

#ifdef TERM_DEBUG
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
