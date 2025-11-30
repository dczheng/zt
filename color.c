#include "zt.h"

struct color_t _xterm[] = {
    {.rgb = {  0,   0,   0}}, // black
    {.rgb = {205,   0,   0}}, // red
    {.rgb = {  0, 205,   0}}, // green
    {.rgb = {205, 205,   0}}, // yellow
    {.rgb = {  0,   0, 238}}, // blue
    {.rgb = {205,   0, 205}}, // magenta
    {.rgb = {  0, 205, 205}}, // cyan
    {.rgb = {229, 229, 229}}, // white
    {.rgb = {127, 127, 127}}, // bright black(gray)
    {.rgb = {255,   0,   0}}, // bright red
    {.rgb = {  0, 255,   0}}, // bright green
    {.rgb = {255, 255,   0}}, // bright yellow
    {.rgb = { 92,  92, 255}}, // bright blue
    {.rgb = {255,   0, 255}}, // bright magenta
    {.rgb = {  0, 255, 255}}, // bright cyan
    {.rgb = {255, 255, 255}}, // bright white
};

struct color_t _ubuntu[] = {
    {.rgb = {  1,   1,   1}}, // black
    {.rgb = {222,  56,  43}}, // red
    {.rgb = { 57, 181,  74}}, // green
    {.rgb = {255, 199,   6}}, // yellow
    {.rgb = {  0, 111, 184}}, // blue
    {.rgb = {118,  38, 113}}, // magenta
    {.rgb = { 44, 181, 233}}, // cyan
    {.rgb = {204, 204, 204}}, // white
    {.rgb = {128, 128, 128}}, // bright black(gray)
    {.rgb = {255,   0,   0}}, // bright red
    {.rgb = {  0, 255,   0}}, // bright green
    {.rgb = {255, 255,   0}}, // bright yellow
    {.rgb = {  0,   0, 255}}, // bright blue
    {.rgb = {255,   0, 255}}, // bright magenta
    {.rgb = {  0, 255, 255}}, // bright cyan
    {.rgb = {255, 255, 255}}, // bright white
};

struct color_t _vga[] = {
    {.rgb = {  0,   0,   0}}, // black
    {.rgb = {170, 170,   0}}, // red
    {.rgb = {  0, 170,   0}}, // green
    {.rgb = {170,  85,   0}}, // yellow
    {.rgb = {  0,   0, 170}}, // blue
    {.rgb = {170,   0, 170}}, // magenta
    {.rgb = {  0, 170, 170}}, // cyan
    {.rgb = {170, 170, 170}}, // white
    {.rgb = { 85,  85,  85}}, // bright black(gray)
    {.rgb = {255,  85,  85}}, // bright red
    {.rgb = { 85, 255,  85}}, // bright green
    {.rgb = {255, 255,  85}}, // bright yellow
    {.rgb = { 85,  85, 255}}, // bright blue
    {.rgb = {255,  85, 255}}, // bright magenta
    {.rgb = { 85, 255, 255}}, // bright cyan
    {.rgb = {255, 255, 255}}, // bright white
};

struct color_t _zt[] = {
    {.rgb = {  0,   0,   0}}, // black
    {.rgb = {205,   0,   0}}, // red
    {.rgb = {  0, 205,   0}}, // green
    {.rgb = {205, 205,   0}}, // yellow
    {.rgb = { 59, 120, 255}}, // blue
    {.rgb = {205,   0, 205}}, // magenta
    {.rgb = {  0, 205, 205}}, // cyan
    {.rgb = {229, 229, 229}}, // white
    {.rgb = {127, 127, 127}}, // bright black(gray)
    {.rgb = {230,   0,   0}}, // bright red
    {.rgb = {  0, 255,   0}}, // bright green
    {.rgb = {255, 255,   0}}, // bright yellow
    {.rgb = {  0,  0,  200}}, // bright blue
    {.rgb = {255,   0, 255}}, // bright magenta
    {.rgb = {  0, 255, 255}}, // bright cyan
    {.rgb = {255, 255, 255}}, // bright white
};

struct color_t
c8_to_rgb(uint8_t v) {
    struct color_t c;

    if (v <= 15) {
        switch (zt.opt.color) {
        case COLOR_UBUNTU:
            c = _ubuntu[v];
            break;
        case COLOR_XTERM:
            c = _xterm[v];
            break;
        case COLOR_VGA:
            c = _vga[v];
            break;
        default:
            c = _zt[v];
        }
    } else {
        // 6 x 6 x 6 = 216 cube colors
        // 16 + 36*r + 6*g + b
        if (v >= 16 && v <= 231) {
            v -= 16;
            c.rgb[0] = v / 36;
            c.rgb[1] = v % 36 / 6;
            c.rgb[2] = v % 6;
            for (int i = 0; i < 3; i++)
                c.rgb[i] = c.rgb[i] * 40 + 55;
        }

        // 24-step grayscale
        if (v >= 232)
            c.rgb[0] = c.rgb[1] = c.rgb[2] = (v-232) * 11;
    }
    c.type = 24;
    return c;
}

int
color_equal(struct color_t a, struct color_t b) {
    if (a.type != b.type)
        return 0;
    if (a.type == 8)
        return a.c8 == b.c8;
    return a.rgb[0] == b.rgb[0] &&
           a.rgb[1] == b.rgb[1] &&
           a.rgb[2] == b.rgb[2];
}
