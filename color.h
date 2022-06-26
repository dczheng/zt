#ifndef __COLOR_H__
#define __COLOR_H__

struct MyColor {
    unsigned char type;
    union {
        unsigned char rgb[3];
        unsigned char c8;
    };
};

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

#endif
