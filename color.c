#include "tool.h"
#include "color.h"
#include "config.h"

struct MyColor
xterm_c8(unsigned char v) {
    struct MyColor c;
    unsigned char r, g, b;

    r = g = b = 0;
    /* standard colors */
    if (v <= 7) {
        switch (v) {
            CASE(  0, r =   0; g =   0; b =   0) // black
            CASE(  1, r = 205; g =   0; b =   0) // red
            CASE(  2, r =   0; g = 205; b =   0) // green
            CASE(  3, r = 205; g = 205; b =   0) // yellow
            CASE(  4, r =   0; g =   0; b = 238) // blue
            CASE(  5, r = 205; g =   0; b = 205) // magenta
            CASE(  6, r =   0; g = 205; b = 205) // cyan
            CASE(  7, r = 229; g = 229; b = 229) // white
        }
    }

    /* high intensity colors */
    if (v >= 8 && v <= 15) {
        switch (v) {
            CASE(  8, r = 127; g = 127; b = 127) // black (gray)
            CASE(  9, r = 255; g =   0; b =   0) // red
            CASE( 10, r =   0; g = 255; b =   0) // green
            CASE( 11, r = 255; g = 255; b =   0) // yellow
            CASE( 12, r =  92; g =  92; b = 255) // blue
            CASE( 13, r = 255; g =   0; b = 255) // magenta
            CASE( 14, r =   0; g = 255; b = 255) // cyan
            CASE( 15, r = 255; g = 255; b = 255) // white
        }
    }

    c.type = COLOR24;
    c.rgb[0] = r;
    c.rgb[1] = g;
    c.rgb[2] = b;
    return c;
}

struct MyColor
ubuntu_c8(unsigned char v) {
    struct MyColor c;
    unsigned char r, g, b;

    r = g = b = 0;
    /* standard colors */
    if (v <= 7) {
        switch (v) {
            CASE(  0, r =   1; g =   1; b =   1) // black
            CASE(  1, r = 222; g =  56; b =  43) // red
            CASE(  2, r =  57; g = 181; b =  74) // green
            CASE(  3, r = 255; g = 199; b =   6) // yellow
            CASE(  4, r =   0; g = 111; b = 184) // blue
            CASE(  5, r = 118; g =  38; b = 113) // magenta
            CASE(  6, r =  44; g = 181; b = 233) // cyan
            CASE(  7, r = 204; g = 204; b = 204) // white
        }
    }

    /* high intensity colors */
    if (v >= 8 && v <=15) {
        switch (v) {
            CASE(  8, r = 128; g = 128; b = 128) // black (gray)
            CASE(  9, r = 255; g =   0; b =   0) // red
            CASE( 10, r =   0; g = 255; b =   0) // green
            CASE( 11, r = 255; g = 255; b =   0) // yellow
            CASE( 12, r =   0; g =   0; b = 255) // blue
            CASE( 13, r = 255; g =   0; b = 255) // magenta
            CASE( 14, r =   0; g = 255; b = 255) // cyan
            CASE( 15, r = 255; g = 255; b = 255) // white
        }
    }

    c.type = COLOR24;
    c.rgb[0] = r;
    c.rgb[1] = g;
    c.rgb[2] = b;
    return c;
}

struct MyColor
c8_to_rgb(unsigned char v) {
    struct MyColor c;
    unsigned char r, g, b;

    if (v <= 15) {
        switch (USED_COLOR) {
            case UBUNTU_COLOR:
                return ubuntu_c8(v);
            case XTERM_COLOR:
                return xterm_c8(v);
            default:
                return ubuntu_c8(v);
        }
    }

    /* 6 x 6 x 6 = 216 cube colors */
#define S(x) (x = (x == 0 ? 0 : 0x3737 + 0x2828 * x)) // copyed from st.
    if (v >= 16 && v <= 231) {
        v -= 16;
        r = v / 36;
        g = (v / 6) % 6;
        b = v % 6;
        S(r);
        S(g);
        S(b);
    }
#undef S

    /* grayscale from black to white in 24 steps */
    if (v >= 232) {
        v -= 232;
        r = g = b = 0x0808 + 0x0a0a * v; // copyed from st.
    }

    c.type = COLOR24;
    c.rgb[0] = r;
    c.rgb[1] = g;
    c.rgb[2] = b;
    return c;
}

int
color_equal(struct MyColor a, struct MyColor b) {
    if (a.type != b.type)
        return 0;
    if (a.type == COLOR8)
        return a.c8 == b.c8;
    return a.rgb[0] == b.rgb[0] &&
           a.rgb[1] == b.rgb[1] &&
           a.rgb[2] == b.rgb[2];
}

