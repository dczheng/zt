#ifndef __ZT_H__
#define __ZT_H__

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

typedef uint32_t MyRune;

struct MyColor {
    unsigned char type;
    union {
        unsigned char rgb[3];
        unsigned char c8;
    };
};

struct MyChar {
    MyRune c;
    char width;
    struct MyColor fg, bg;
    unsigned int flag;
};

// global namespace
struct ZT {
    int *dirty, *tabs,
        top, bot, width ,height,
        x, y, x_saved, y_saved,
        row, col, row_old, col_old,
        tty, xfd;
    struct MyChar **line, **alt_line, **norm_line, c;
    MyRune lastc;
    unsigned long mode;
};
extern struct ZT zt;

long get_time(void);
void dump_hex(unsigned char*, int);
void dump(unsigned char*, int);
char *to_bytes(unsigned long);

#define UNUSED __attribute__((unused))
#define NANOSEC 1000000000
#define MICROSEC 1000000

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define LEN(x) ((int)(sizeof(x) / sizeof(x[0])))

#define SWAP(a,b) do { \
    typeof(a) _t;\
    _t = a;\
    a = b;\
    b = _t;\
} while(0);

#define CASE(value, doing) \
    case value: \
    doing;\
    break;

#define LIMIT(x, a, b) \
    x = (x) < (a) ? (a) : ((x) > (b) ? (b) : (x))

#define ASSERT(exp, fmt, ...) do { \
    if (!(exp)) { \
        fprintf(stderr, "Assert failed: %s in %s %d, "fmt"\n", \
            #exp, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        _exit(1);\
    }\
} while(0)

// color
int color_equal(struct MyColor, struct MyColor);
#define UBUNTU_COLOR    1
#define XTERM_COLOR     2
#define COLOR8    8
#define COLOR24  24 

#define COLOR_RESET(c)  bzero(&c, sizeof(struct MyColor))
#define SET_COLOR8(c, v) do { \
    (c).type = COLOR8; \
    (c).c8 = v; \
} while(0)
#define SET_COLOR24(c, r, g, b) do { \
    (c).type = COLOR24; \
    (c).rgb[0] = r; \
    (c).rgb[1] = g; \
    (c).rgb[2] = b; \
} while(0)

// mode
#define MODE_TEXT_CURSOR        (1<<0)
#define MODE_SEND_FOCUS         (1<<1)
#define MODE_BRACKETED_PASTE    (1<<2)
#define MODE_MOUSE              (1<<3)
#define MODE_MOUSE_PRESS        (1<<4)
#define MODE_MOUSE_RELEASE      (1<<5)
#define MODE_MOUSE_MOTION_PRESS (1<<6)
#define MODE_MOUSE_MOTION_ANY   (1<<7)
#define MODE_MOUSE_EXT          (1<<8)

#define SET      1
#define RESET    0

#define MODE_HAS(m)     ((zt.mode & (m)) != 0)
#define MODE_RESET() do { \
    zt.mode = MODE_TEXT_CURSOR \
            ; \
} while(0)

#define MODE_SET(s, v) do { \
    ASSERT(s == SET || s == RESET, "");\
    if (s == SET) \
        zt.mode |= (v); \
    else \
        zt.mode &= ~(v); \
} while (0)

// attr
#define ATTR_DEFAULT_FG                (1<<0)
#define ATTR_DEFAULT_BG                (1<<1)
#define ATTR_UNDERLINE                 (1<<2)
#define ATTR_BOLD                      (1<<3)
#define ATTR_ITALIC                    (1<<4)
#define ATTR_FAINT                     (1<<5)
#define ATTR_COLOR_REVERSE             (1<<6)
#define ATTR_CROSSED_OUT               (1<<7)

#define ATTR_SET(v)       (zt.c.flag |= (v))
#define ATTR_UNSET(v)     (zt.c.flag &= ~(v))
#define ATTR_HAS(a, v)    (((a).flag & (v)) != 0)

#define ATTR_FG8(v) do { \
    SET_COLOR8(zt.c.fg, v); \
    ATTR_UNSET(ATTR_DEFAULT_FG);\
} while(0)

#define ATTR_BG8(v) do { \
    SET_COLOR8(zt.c.bg, v); \
    ATTR_UNSET(ATTR_DEFAULT_BG);\
} while(0)

#define ATTR_FG24(r, g, b) do { \
    SET_COLOR24(zt.c.fg, r, g, b); \
    ATTR_UNSET(ATTR_DEFAULT_FG);\
} while(0)

#define ATTR_BG24(r, g, b) do { \
    SET_COLOR24(zt.c.bg, r, g, b); \
    ATTR_UNSET(ATTR_DEFAULT_BG);\
} while(0)

#define ATTR_RESET() do { \
    COLOR_RESET(zt.c.fg); \
    COLOR_RESET(zt.c.bg); \
    zt.c.flag = ATTR_DEFAULT_FG \
              | ATTR_DEFAULT_BG \
              ;\
    zt.c.width = 1;\
    zt.c.c = ' ';\
} while(0)

#define ATTR_EQUAL(a, b) \
    (color_equal((a).fg, (b).fg) && \
     color_equal((a).bg, (b).bg) && \
     (a).flag == (b).flag && (a).width == (b).width)

#include "config.h"

#endif
