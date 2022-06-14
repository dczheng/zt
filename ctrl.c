#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ctrl.h"
#include "util.h"

int
str_to_dec(char *p, int *r) {
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
esc_find(unsigned char *seq, int len,
        unsigned char a, unsigned char b, int mode) {
    int i;

    for (i=0; i<len; i++) {
        if ((!mode) && (seq[i] >= a && seq[i] <= b))
            break;
        if ((mode) && (seq[i] < a && seq[i] > b))
            break;
    }
    if (i == len)
        return -1;
    return i;
}

int
get_par_num(struct Esc *esc) {
    int i, n;

    for (i=1, n=0; i<esc->len-1; i++) {
        if (esc->seq[i] == ';')
            n++;
    }
    return n+1;
}

int
get_par(struct Esc *esc, int idx, int *a, int *b) {
    int i, n;

    *a = *b = 1;
    for (i=1, n=0; i<esc->len-1; i++) {
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
get_str_par(struct Esc *esc, int idx, char **p) {
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
get_int_par(struct Esc *esc, int idx, int *v, int v0) {
    int ret;
    char *p;

    ret = get_str_par(esc, idx, &p);
    if (ret)
        return ret;

    if (p == NULL) {
        *v = v0;
        return 0;
    }
    return str_to_dec(p, v);
}

void // debug
csi_dump(struct Esc *esc) {
    int i, r, rr, a, b;
    
     if (esc->csi != SGR)
        return;
     dump(esc->seq, esc->len);
     for (i=0; i<get_par_num(esc); i++) {
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
osc_find_end(unsigned char *seq, int len, int *n) {
    *n = esc_find(seq, len, ST, ST, 0);
    if (*n<0) {
        *n = esc_find(seq, len, BEL, BEL, 0);
        if (*n<0)
            return ESCOSCNOEND;
    }
    return 0;
}

int
csi_find_end(unsigned char *seq, int len, int *n) {
    *n = esc_find(seq, len, 0x40, 0x7E, 0);
    if (*n < 0)
        return ESCCSINOEND;
    return 0;
}

int
esc_parse(unsigned char *seq, int len, struct Esc *esc) {
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
        n = esc_find(seq+1, len-1, 0x30, 0x7E, 0);
        if (n<0)
            return ESCNFNOEND;
        esc->len += n+1;
    }

    if (c >= 0x40 && c <= 0x5F) {
        esc->type = ESCFE;
        esc->esc = c - 0x40 + 0x80;
        switch (esc->esc) {
            case CSI:
                if ((ret = csi_find_end(seq+1, len-1, &n)))
                    return ret;
                esc->len += n+1;
                esc->csi = seq[esc->len-1];
                //csi_dump(esc);
                break;

            case OSC:
                if ((ret = osc_find_end(seq+1, len-1, &n)))
                    return ret;
                esc->len += n+1;
                //dump(esc->seq, esc->len-1);
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

void
esc_reset(struct Esc *esc) {
    bzero(esc, sizeof(struct Esc));
    esc->len = 0;
    esc->seq = NULL;
}

char* // debug
get_esc_str(struct Esc *esc, int desc) {
    struct CtrlInfo *info, *info2;
    int i, pos, n, ret;
    char *p;
    static char buf[BUFSIZ];

#define MYPRINT(fmt, ...) \
    pos += snprintf(buf+pos, sizeof(buf)-pos, fmt, ##__VA_ARGS__)

    pos = 0;
    buf[0] = 0;
    if (!esc->len)
        return buf;

    if (get_esc_info(esc->type, &info)) {
        MYPRINT("Unknown esc type");
        return buf;
    }
    
    MYPRINT("ESC %s ", info->name);
    switch (esc->type) {
        case ESCFE: goto escfe;
        case ESCNF: goto escnf;
        case ESCFS: goto escfs;
        case ESCFP: goto escfp;
        default: ASSERT(0, "can't be");
    }

escfe:
    if (get_ctrl_info(esc->esc, &info)) {
        MYPRINT("error fe esc type");
        return buf;
    }
    MYPRINT("%s ", info->name);

    switch (esc->esc) {
        case CSI:
            if (get_csi_info(esc->csi, &info)) {
                MYPRINT("%c ", esc->csi);
                desc = 0;
            } else {
                MYPRINT("%s ", info->name);
            }

            if (esc->csi == RM ||  esc->csi == SM) {
                if (esc->seq[1] != '?')
                    MYPRINT("unknown ");
                else
                    for (i=0; i<get_par_num(esc); i++) {
                        if (get_str_par(esc, i, &p) || p == NULL) {
                            MYPRINT("unknown ");
                            continue;
                        }
                        if (i == 0)
                            ret = str_to_dec(p+1, &n);
                        else
                            ret = str_to_dec(p, &n);

                        if (ret || (ret = get_mode_info(n, &info2))) {
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
            for (i=0; i<n; i++) {
                if (get_str_par(esc, i, &p) || p == NULL)
                    MYPRINT("%s", "-");
                else
                    MYPRINT("%s", p);
                if (i < n-1)
                    MYPRINT(",");
            }
            MYPRINT("] ");
            get_int_par(esc, 0, &i, 0);
            get_sgr_info(i, &info);
            break;

        case OSC:
            break;

    }
    if (desc)
        MYPRINT("`%s`", info->desc);
    return buf;

escnf:
    if (get_nf_esc_info(esc->esc, &info))
        MYPRINT("0x%X (%c)", esc->esc, esc->esc);
    else {
        MYPRINT("%s ", info->name);
        if (desc)
            MYPRINT("`%s`", info->desc);
    }
    return buf;

escfp:
    if (get_fp_esc_info(esc->esc, &info))
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
