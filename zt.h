#ifndef __ZT_H__
#define __ZT_H__

#define VTIDEN "\033[?6c"
#define TERM "xterm-256color"
#define BACKGROUND "gray20"
#define FOREGROUND "white"
#define FONT "Ubuntu Mono:pixelsize=20:antialias=true:autohint=true"
#define LATENCY 30
#define ROW 24
#define COL 80 
#define TABSPACE 4

#include <string.h>
#include <stdint.h>

#include "util.h"

struct MyColor {
    unsigned char type;
    union {
        unsigned char rgb[3];
        unsigned char c8;
    };
};

struct MyAttr {
    struct MyColor fg, bg;
    uint32_t flag;
};

struct MyChar {
    uint32_t c;
    struct MyAttr attr;
};

// global namespace
struct ZT {
    int *dirty, *tabs;
    int top, bot, x, y, row, col, width ,height;
    int row_old, col_old; // for resize
    int tty, xfd;
    struct MyChar **line;
    struct MyAttr attr;
    unsigned long mode;
};
extern struct ZT zt;

// color
int color_equal(struct MyColor, struct MyColor);
#define UBUNTU_COLOR8    1
#define XTERM_COLOR8     2
#define COLOR8    8
#define COLOR24  24 

#define USED_COLOR8   UBUNTU_COLOR8

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
#define MODE_TEXT_CURSOR        (1 << 0)
#define MODE_SEND_FOCUS         (1 << 1)
#define MODE_BRACKETED_PASTE    (1 << 2)
#define MODE_REPORT_BUTTON      (1 << 3)
#define MODE_REPORT_MOTION      (1 << 4)

#define MODE_HAS(m)     (BIT_GET(zt.mode, m) != 0)
#define MODE_RESET() do { \
      zt.mode \
    = MODE_TEXT_CURSOR \
    | MODE_SEND_FOCUS \
    | MODE_REPORT_BUTTON \
    | MODE_REPORT_MOTION \
    ; \
} while(0)
#define MODE_SET(m, v) ( \
    (m) == SM ? BIT_SET(zt.mode, v) : \
    ((m) == RM ? BIT_UNSET(zt.mode, v) : zt.mode ) \
)

// attr
#define ATTR_DEFAULT_FG                (1<<0)
#define ATTR_DEFAULT_BG                (1<<1)
#define ATTR_UNDERLINE                 (1<<2)
#define ATTR_BOLD                      (1<<3)
#define ATTR_COLOR_REVERSE             (1<<4)

#define ATTR_SET(a, t)    BIT_SET((a).flag, t)
#define ATTR_UNSET(a, t)  BIT_UNSET((a).flag, t)
#define ATTR_HAS(a, t)    (BIT_GET((a).flag, t) != 0)

#define ATTR_RESET(c) do { \
    COLOR_RESET((c).fg); \
    COLOR_RESET((c).bg); \
      (c).flag \
    = ATTR_DEFAULT_FG \
    | ATTR_DEFAULT_BG \
    ;\
} while(0)

#define ATTR_EQUAL(a, b) \
    (color_equal((a).fg, (b).fg) && \
     color_equal((a).bg, (b).bg) && \
     (a).flag == (b).flag)

#endif
