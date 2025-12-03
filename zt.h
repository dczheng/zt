#ifndef __ZT_H__
#define __ZT_H__

#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#define __unused __attribute__((unused))

#define FOREGROUND "white"
#define BACKGROUND "gray20"

static struct {
    char *name;
    int size;
} font_list[] __unused = {
    {"Sarasa Term CL",  26},
    {"Noto Emoji",       8},
    {"Unifont",         18},
};

static struct {
    uint8_t r, g, b;
} standard_colors[] __unused = {
    {  0,   0,   0}, // black
    {205,   0,   0}, // red
    {  0, 205,   0}, // green
    {205, 205,   0}, // yellow
    { 59, 120, 255}, // blue
    {205,   0, 205}, // magenta
    {  0, 205, 205}, // cyan
    {229, 229, 229}, // white
    {127, 127, 127}, // bright black(gray)
    {230,   0,   0}, // bright red
    {  0, 255,   0}, // bright green
    {255, 255,   0}, // bright yellow
    {  0,  0,  200}, // bright blue
    {255,   0, 255}, // bright magenta
    {  0, 255, 255}, // bright cyan
    {255, 255, 255}, // bright white
};

struct color_t {
    uint8_t type;
    union {
        struct {
            uint8_t r, g, b;
        };
        uint8_t rgb[3];
        uint8_t c8;
    };
};

struct char_t {
    uint32_t c;
    char width;
    struct color_t fg, bg;
    unsigned int attr;
};

struct zt_t {
    int *dirty, *tabs, top, bot, width ,height,
        x, y, x_saved, y_saved, row, col,
        row_old, col_old, debug, no_ignore;
    struct {
        struct char_t **line;
        char *buffer;
    } alt, normal;
    struct char_t **line, c, lastc;
    char *term;
    unsigned long mode;
    double fontsize;
};
extern struct zt_t zt;

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

#define SET(a, b) (a |= (b))
#define UNSET(a, b) (a &= ~(b))
#define ISSET(a, b) (!!(a & (b)))

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

#define ATTR_DEFAULT_FG     (1<<0)
#define ATTR_DEFAULT_BG     (1<<1)
#define ATTR_UNDERLINE      (1<<2)
#define ATTR_BOLD           (1<<3)
#define ATTR_ITALIC         (1<<4)
#define ATTR_FAINT          (1<<5)
#define ATTR_COLOR_REVERSE  (1<<6)
#define ATTR_CROSSED_OUT    (1<<7)

#define SECOND         1000000000L
#define MILLISECOND    1000000L
#define MICROSECOND    1000L

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

static inline int
stoi(int *i, char *p) {
    char *e = NULL;
    if (!STRLEN(p)) return EINVAL;
    *i = strtol(p, &e, 10);
    return (STRLEN(e) ? EINVAL : 0);
}

static inline double
stod(double *d, char *p) {
    char *e = NULL;
    if (!STRLEN(p)) return EINVAL;
    *d = strtod(p, &e);
    return (STRLEN(e) ? EINVAL : 0);
}

#endif
