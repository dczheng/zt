#ifndef __TERM_H__
#define __TERM_H__

#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include "code.h"

struct term_color_t {
    uint8_t type;
    union {
        struct {
            uint8_t r, g, b;
        };
        uint8_t rgb[3];
        uint8_t c8;
    };
};

struct term_char_t {
    uint32_t c;
    char width;
    struct term_color_t fg, bg;
    unsigned int mode;
};

struct term_t {
    int *dirty, *tabs, row, col, top, bot,
        x, y, x_saved, y_saved, debug,
        no_ignore, tty, retry;
    unsigned long mode;
    struct {
        struct term_char_t **line;
        char *buffer;
    } alt, normal;
    struct term_char_t **line, c, lastc;

    uint8_t data[BUFSIZ];
    char param[256];
    int size;
    struct {
        int n, npar, size;
        uint8_t *base, esc, csi;
    } ctrl;
};

#define SECOND      1000000000L
#define MILLISECOND 1000000L
#define MICROSECOND 1000L

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define LEN(x) ((int)(sizeof(x) / sizeof(x[0])))
#define ZERO(a) bzero(&(a), sizeof(a))
#define STRLEN(s) ((s) ? strlen(s) : 0)

#define SWAP(a,b) do { \
    typeof(a) _t;\
    _t = a;\
    a = b;\
    b = _t;\
} while(0);

#define LIMIT(x, a, b) \
    x = (x) < (a) ? (a) : ((x) > (b) ? (b) : (x))

#define LOG(arg...) do { \
    fprintf(stdout, ##arg); \
    fflush(stdout); \
} while(0)

#define LOGERR(arg...) do { \
    fprintf(stderr, ##arg); \
    fflush(stderr); \
} while(0)

#define ASSERT(exp) do { \
    if (!(exp)) { \
        LOG("Assert failed `%s` %s:%s:%d\n", \
            #exp, __FILE__, __FUNCTION__, __LINE__); \
        if (errno) \
            LOG("errno: %s\n", strerror(errno)); \
        _exit(1);\
    }\
} while(0)

#define DIE() do { \
    LOG("DIE %s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__); \
    _exit(1);\
} while(0)

#define MODE_SET(a, b)          ((a)->mode |= (b))
#define MODE_UNSET(a, b)        ((a)->mode &= ~(b))
#define MODE_ISSET(a, b)        (!!((a)->mode & (b)))

#define MODE_CURSOR             (1<<0)
#define MODE_FOCUS              (1<<1)
#define MODE_MOUSE_PRESS        (1<<2)
#define MODE_MOUSE_RELEASE      (1<<3)
#define MODE_MOUSE_MOTION       (1<<4)
#define MODE_MOUSE_SGR          (1<<5)
#define MODE_GZD4               (1<<6)
#define MODE_MOUSE              (MODE_MOUSE_PRESS | \
                                 MODE_MOUSE_RELEASE | \
                                 MODE_MOUSE_MOTION | \
                                 MODE_MOUSE_SGR)

#define CHAR_MODE_DEFAULT_FG    (1<<0)
#define CHAR_MODE_DEFAULT_BG    (1<<1)
#define CHAR_MODE_UNDERLINE     (1<<2)
#define CHAR_MODE_BOLD          (1<<3)
#define CHAR_MODE_ITALIC        (1<<4)
#define CHAR_MODE_FAINT         (1<<5)
#define CHAR_MODE_COLOR_REVERSE (1<<6)
#define CHAR_MODE_CROSSED_OUT   (1<<7)

void term_init(struct term_t*, char*);
void term_free(struct term_t*);
int term_read(struct term_t*);
int term_write(struct term_t*, char*, int);
void term_flush(struct term_t*);
void term_resize(struct term_t*, int, int, int, int);

static inline int
term_color_equal(struct term_color_t *a, struct term_color_t *b) {
    if (a->type != b->type)
        return 0;
    if (a->type == 8)
        return a->c8 == b->c8;
    return a->r == b->r && a->g == b->g && a->b == b->b;
}

static inline int
term_attr_equal(struct term_char_t *a, struct term_char_t *b) {
    return term_color_equal(&a->fg, &b->fg) &&
        term_color_equal(&a->bg, &b->bg) &&
        a->mode == b->mode && a->width == b->width;
}

static inline int
stoi(int *i, char *p) {
    char *e = NULL;
    int v;
    if (!STRLEN(p)) return EINVAL;
    v = strtol(p, &e, 10);
    if (STRLEN(e)) return EINVAL;
    *i = v;
    return 0;
}

static inline double
stod(double *d, char *p) {
    char *e = NULL;
    double v;
    if (!STRLEN(p)) return EINVAL;
    v = strtod(p, &e);
    if (STRLEN(e)) return EINVAL;
    *d = v;
    return 0;
}

static inline long
get_time(void) {
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return tv.tv_sec * SECOND + tv.tv_nsec;
}

static inline struct timespec
to_timespec(long t) {
    struct timespec tv;
    tv.tv_sec = t / SECOND;
    tv.tv_nsec = t % SECOND;
    return tv;
}

#endif
