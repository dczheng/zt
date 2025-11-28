#include "zt.h"

struct color_t
xterm_c8(uint8_t v) {
    struct color_t c;
    uint8_t r, g, b;

    r = g = b = 0;
    /* standard colors */
    if (v <= 7) {
        switch (v) {
        case  0: r =   0; g =   0; b =   0; break; // black
        case  1: r = 205; g =   0; b =   0; break; // red
        case  2: r =   0; g = 205; b =   0; break; // green
        case  3: r = 205; g = 205; b =   0; break; // yellow
        case  4: r =   0; g =   0; b = 238; break; // blue
        case  5: r = 205; g =   0; b = 205; break; // magenta
        case  6: r =   0; g = 205; b = 205; break; // cyan
        case  7: r = 229; g = 229; b = 229; break; // white
        }
    }

    /* high intensity colors */
    if (v >= 8 && v <= 15) {
        switch (v) {
        case  8: r = 127; g = 127; b = 127; break; // black (gray)
        case  9: r = 255; g =   0; b =   0; break; // red
        case 10: r =   0; g = 255; b =   0; break; // green
        case 11: r = 255; g = 255; b =   0; break; // yellow
        case 12: r =  92; g =  92; b = 255; break; // blue
        case 13: r = 255; g =   0; b = 255; break; // magenta
        case 14: r =   0; g = 255; b = 255; break; // cyan
        case 15: r = 255; g = 255; b = 255; break; // white
        }
    }

    c.type = COLOR24;
    c.rgb[0] = r;
    c.rgb[1] = g;
    c.rgb[2] = b;
    return c;
}

struct color_t
ubuntu_c8(uint8_t v) {
    struct color_t c;
    uint8_t r, g, b;

    r = g = b = 0;
    /* standard colors */
    if (v <= 7) {
        switch (v) {
        case  0: r =   1; g =   1; b =   1; break; // black
        case  1: r = 222; g =  56; b =  43; break; // red
        case  2: r =  57; g = 181; b =  74; break; // green
        case  3: r = 255; g = 199; b =   6; break; // yellow
        case  4: r =   0; g = 111; b = 184; break; // blue
        case  5: r = 118; g =  38; b = 113; break; // magenta
        case  6: r =  44; g = 181; b = 233; break; // cyan
        case  7: r = 204; g = 204; b = 204; break; // white
        }
    }

    /* high intensity colors */
    if (v >= 8 && v <=15) {
        switch (v) {
        case  8: r = 128; g = 128; b = 128; break; // black (gray)
        case  9: r = 255; g =   0; b =   0; break; // red
        case 10: r =   0; g = 255; b =   0; break; // green
        case 11: r = 255; g = 255; b =   0; break; // yellow
        case 12: r =   0; g =   0; b = 255; break; // blue
        case 13: r = 255; g =   0; b = 255; break; // magenta
        case 14: r =   0; g = 255; b = 255; break; // cyan
        case 15: r = 255; g = 255; b = 255; break; // white
        }
    }

    c.type = COLOR24;
    c.rgb[0] = r;
    c.rgb[1] = g;
    c.rgb[2] = b;
    return c;
}

struct color_t
c8_to_rgb(uint8_t v) {
    struct color_t c;
    uint8_t r, g, b;

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
#define S(x) (x = (x == 0 ? 0 : 0x3737 + 0x2828 * x)) // copied from st.
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
color_equal(struct color_t a, struct color_t b) {
    if (a.type != b.type)
        return 0;
    if (a.type == COLOR8)
        return a.c8 == b.c8;
    return a.rgb[0] == b.rgb[0] &&
           a.rgb[1] == b.rgb[1] &&
           a.rgb[2] == b.rgb[2];
}
