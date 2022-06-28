#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/Xft/Xft.h>
#include <X11/Xatom.h>

#include "zt.h"
#include "ctrl.h"
#include "config.h"

//#define XDEBUG

Display *display;
Window root, window;
GC gc;
Cursor cursor;
Colormap colormap;
Visual *visual;
Atom wm_protocols, wm_delete;
XftColor background, foreground, color8[256];
XftDraw *drawable;
Pixmap pixmap;
XftGlyphFontSpec *specs;
int screen, depth, nspec;

struct MyFont {
    XftFont *font;
    int weight, slant;
} *fonts;
int font_width, font_height, font_base, nfont;

static inline void
xflush(void) {
    XCopyArea(display, pixmap, window, gc, 
        0, 0, zt.width, zt.height, 0, 0);
    XFlush(display);
}

static inline int
xcolor_alloc(XftColor *c,
        unsigned char r,
        unsigned char g,
        unsigned char b) {
    XRenderColor rc;

    rc.red = r << 8;
    rc.green = g << 8;
    rc.blue = b << 8;
    rc.alpha = 0xffff;
    if (!XftColorAllocValue(display, visual, colormap, &rc, c)) {
        printf("failed to allocate color for (%u %u %u)\n", r, b, g);
        return 1;
    }
    return 0;
}

static inline void
xcolor_free(XftColor *c) {
    XftColorFree(display, visual, colormap, c);
}

void
xdraw_specs(struct MyChar c) {
    XftColor bg, fg;
    int x, y, w, rf, rb, t;

    if (!nspec)
        return;

    fg = foreground;
    bg = background;
    y = specs[0].y - font_base;
    x = specs[0].x;
    w = nspec * c.width * font_width;
    rf = rb = 1;

    t = zt.width - x - w;
    if (t > 0 && t < font_width)
        w = zt.width-x;

#ifdef XDEBUG
        printf("(%d, %d, %d, %d, %d) ", nspec, c.width, x, w,
            specs[nspec-1].x);
#endif

    if ((!ATTR_HAS(c, ATTR_DEFAULT_FG))) {
        switch (c.fg.type) {
            CASE(COLOR8, fg = color8[c.fg.c8])
            CASE(COLOR24,
                rf = xcolor_alloc(&fg, c.fg.rgb[0], c.fg.rgb[1], c.fg.rgb[2]))
        }
    }

    if ((!ATTR_HAS(c, ATTR_DEFAULT_BG))) {
        switch (c.bg.type) {
            CASE(COLOR8, bg = color8[c.bg.c8])
            CASE(COLOR24,
                rb = xcolor_alloc(&bg, c.bg.rgb[0], c.bg.rgb[1], c.bg.rgb[2]))
        }
    }

    if (ATTR_HAS(c, ATTR_COLOR_REVERSE))
        SWAP(fg, bg);

    XftDrawRect(drawable, &bg, x, y, w, font_height);
    if (ATTR_HAS(c, ATTR_UNDERLINE)) 
        XftDrawRect(drawable, &fg, x, y + font_base + 1, w, 1);
    if (ATTR_HAS(c, ATTR_CROSSED_OUT))
        XftDrawRect(drawable, &fg, x, y + font_height / 2, w, 1);

    XftDrawGlyphFontSpec(drawable, &fg, specs, nspec);
    nspec = 0;
    if (!rf)
        xcolor_free(&fg);
    if (!rb)
        xcolor_free(&bg);
}

void
xfont_lookup(struct MyChar c, XftFont **f, FT_UInt *idx) {
    int i, weight, slant;

    weight = FC_WEIGHT_REGULAR;
    slant = FC_SLANT_ROMAN;

    if (ATTR_HAS(c, ATTR_BOLD))
        weight = FC_WEIGHT_BOLD;

    // TODO
    if (ATTR_HAS(c, ATTR_FAINT))
        weight = FC_WEIGHT_REGULAR;

    if (ATTR_HAS(c, ATTR_ITALIC))
        slant = FC_SLANT_ITALIC;

    for (i = 0; i < nfont; i++) {
        if (fonts[i].weight != weight ||
            fonts[i].slant != slant)
            continue;
        *f = fonts[i].font;
        if ((*idx = XftCharIndex(display, *f, c.c)))
            return;
    }

    printf("can't find font for %X\n", c.c);
    *f = fonts[0].font;
    *idx = XftCharIndex(display, *f, ' ');
}

void
xdraw_line(int k, int y) {
    XRectangle r;
    struct MyChar c, c0;
    int i, x;

    r.x = 0;
    r.y = 0;
    r.width = zt.width;
    r.height = font_height;

#ifdef XDEBUG
    printf("[%3d] ", k);
#endif
    XftDrawSetClipRectangles(drawable, 0, y, &r, 1);
    //XftDrawRect(drawable, &background, 0, y, zt.width, font_height);
    for (i = 0, x = 0, nspec = 0; i < zt.col;) {
        c = zt.line[k][i];
        if (nspec == 0)
            c0 = c;

        if (ATTR_EQUAL(c0, c)) {
            xfont_lookup(c, &specs[nspec].font,
                &specs[nspec].glyph);
            specs[nspec].x = x;
            specs[nspec].y = y + font_base;
            nspec++;
            i += c0.width;
            x += c0.width * font_width;
            continue;
        }
        xdraw_specs(c0);
    }
    xdraw_specs(c0);
#ifdef XDEBUG
    printf("\n");
#endif
    XftDrawSetClip(drawable, 0);
}

void
xdraw_cursor(void) {
    static int last_y = -1;

    if (last_y != -1) {
        if (last_y < zt.row)
            xdraw_line(last_y, last_y*font_height);
        last_y = zt.y;
    } else {
        last_y = zt.y;
    }

    if (MODE_HAS(MODE_TEXT_CURSOR))
        XftDrawRect(drawable, &foreground,
            zt.x*font_width, (zt.y+1)*font_height-3, font_width, 3);
}

void
xdraw(void) {
    int y, i, nline=0;

    //XftDrawRect(drawable, &background, 0, 0, zt.width, zt.height);
#ifdef XDEBUG
    printf("size: %dx%d, %dx%d\n", zt.width, zt.height, zt.row, zt.col);
    printf("font size: %dx%d\n", font_width, font_height);
#endif
    for (i = 0, y = 0; i < zt.row; i++, y += font_height) {
        if (!zt.dirty[i])
            continue;
        nline++;
        xdraw_line(i, y);
    }
    //printf("drawed: %d lines\n", nline);
    xdraw_cursor();
    xflush();
}

// TODO
void
xkeymap(KeySym k, unsigned int state, char *buf, int *len) {
    switch (k) {
        case XK_BackSpace:
            if (state == 0) {
                buf[0] = DEL;
                *len = 1;
            }
            break;
        case XK_Up:
            *len = snprintf(buf, 4, "\033[A");
            break;
        case XK_Down:
            *len = snprintf(buf, 4, "\033[B");
            break;
        case XK_Right:
            *len = snprintf(buf, 4, "\033[C");
            break;
        case XK_Left:
            *len = snprintf(buf, 4, "\033[D");
            break;
    }
}

void
xpointer(int *x, int *y) {
    int di;
    unsigned int dui;
    Window dw;
    XQueryPointer(display, root, &dw, &dw, x, y, &di, &di, &dui);
}

void
xresize() {
    XFreePixmap(display, pixmap);
    pixmap = XCreatePixmap(display, window, zt.width, zt.height, depth);
    XftDrawChange(drawable, pixmap);
    XftDrawRect(drawable, &background, 0, 0, zt.width, zt.height);
    specs = realloc(specs, zt.col * sizeof(XftGlyphFontSpec));
    ASSERT(specs != NULL, "");
}

void
_Mouse(XEvent *ev) {
    int r, c, b, n;
    static int or=-1, oc=-1, ob=-1;
    char t;
    static char buf[64];

    r = ev->xbutton.y / font_height + 1;
    c = ev->xbutton.x / font_width + 1;
    b = ev->xbutton.button-Button1;
    if (b >= 3)
        b += 64-3;

/*
    switch (ev->xbutton.type) {
        case ButtonPress:
            t = 'P';
            break;
        case ButtonRelease:
            t = 'R';
            break;

        case MotionNotify:
            t = 'M';
            break;
        default:
            t = '-';
    }
    printf("%c (%d %d): %d, %d, %d, %d, %d, %d\n", t, r, c,
        MODE_HAS(MODE_MOUSE), 
        MODE_HAS(MODE_MOUSE_PRESS), 
        MODE_HAS(MODE_MOUSE_RELEASE), 
        MODE_HAS(MODE_MOUSE_MOTION_PRESS),
        MODE_HAS(MODE_MOUSE_MOTION_ANY),
        MODE_HAS(MODE_MOUSE_EXT)
        );
*/

    if (!MODE_HAS(MODE_MOUSE))
        return;

    switch (ev->xbutton.type) {
        case ButtonPress:
            if (!MODE_HAS(MODE_MOUSE_PRESS))
                return;
            t = 'M';
            or = r;
            oc = c;
            ob = b;
            break;

        case ButtonRelease:
            if (!MODE_HAS(MODE_MOUSE_RELEASE))
                return;
            t = 'm';
            break;

        case MotionNotify:
            if (!MODE_HAS(MODE_MOUSE_MOTION_PRESS) &&
                !MODE_HAS(MODE_MOUSE_MOTION_ANY))
                return;
            if (r == or && c == oc)
                return;
            t = 'M';
            or = r;
            oc = c;
            b = ob+32;
            break;

        default:
            printf("Unsupported button type: %d\n",
                ev->xbutton.type);
            return;
    }

    if (MODE_HAS(MODE_MOUSE_EXT)) {
        n = snprintf(buf, sizeof(buf),
            "\033[<%d;%d;%d%c", b, c, r, t);
    } else {
        n = snprintf(buf, sizeof(buf),
            "\033[M%c%c%c", 32+b, 32+c, 32+r);
    } 
    twrite(buf, n);
}

void
_Focus(XEvent *ev) {
    if (!MODE_HAS(MODE_SEND_FOCUS))
        return;
    if (ev->type == FocusIn)
        twrite("\033[I", 3);
    else
        twrite("\033[O", 3);
}

void
_Expose(XEvent *ev UNUSED) {
    xflush();
}

void
_KeyPress(XEvent *ev) {
    int n;
    char buf[64];
    KeySym ksym;
    XKeyEvent *e = &ev->xkey;

    n = XLookupString(e, buf, sizeof(buf), &ksym, NULL);
    xkeymap(ksym, e->state, buf, &n);
    //dump((unsigned char*)buf, n);
    twrite(buf, n);
}

void
_ConfigureNotify(XEvent *ev) {
    int w, h, r, c;

    w = ev->xconfigure.width;
    h = ev->xconfigure.height;

    if (w == zt.width && h == zt.height)
        return;

    r = h / font_height;
    c = w / font_width;
    r = MAX(r, 8);
    c = MAX(c, 8);
        
    zt.row_old = zt.row;
    zt.col_old = zt.col;
    zt.width = w;
    zt.height = h;
    zt.row = r;
    zt.col = c;

    //printf("resize: %dx%d -> %dx%d\n",
    //    zt.row_old, zt.col_old, zt.row, zt.col);

    xresize();
    tresize();
    lresize(); }

#define H(type) \
    case type: \
        _##type(&e); \
        break; 
#define H2(type, f) \
    case type: \
        _##f(&e); \
        break; 
int
xevent(void) {
    XEvent e;
    
    for (;XPending(display);){
        XNextEvent(display, &e);
        switch(e.type) {
            H(Expose)
            H(KeyPress)
            H(ConfigureNotify)
            H2(MotionNotify, Mouse)
            H2(ButtonPress, Mouse)
            H2(ButtonRelease, Mouse)
            H2(FocusIn, Focus)
            H2(FocusOut, Focus)
            case MapNotify:
            case KeyRelease:
            case UnmapNotify:
                break;
            case DestroyNotify:
                return 1;
            default:
                printf("Unsupport event %d\n", e.type);
        }
    }
    return 0;
}
#undef H
#undef H2

int
xerror(Display *display, XErrorEvent *e) {
    XSync(display, False);
    fprintf(stderr, "error: %d\n", e->error_code);
    return 0;
}

void 
xclean(void) {
    int i;

    XFreePixmap(display, pixmap);
    XFreeCursor(display, cursor);
    XftDrawDestroy(drawable);

    printf("free fonts\n");
    for (i = 0; i < nfont; i++)
        XftFontClose(display, fonts[i].font);
    free(fonts);

    printf("free colors\n");
    xcolor_free(&background);
    xcolor_free(&foreground);
    for (i = 0; i < 256; i++) 
        xcolor_free(&color8[i]);

    printf("free specs\n");
    free(specs);

    close(zt.xfd);
}

void
xfont_load(char *str, struct MyFont *f) {
    FcPattern *p, *m;
    FcResult r;

    ASSERT(p = FcNameParse((FcChar8*)str), "");
    FcConfigSubstitute(NULL, p, FcMatchPattern);
    FcDefaultSubstitute(p);
    XftDefaultSubstitute(display, screen, p);

    FcPatternDel(p, FC_WEIGHT);
    FcPatternAddInteger(p, FC_WEIGHT, f->weight);

    FcPatternDel(p, FC_SLANT);
    FcPatternAddInteger(p, FC_SLANT, f->slant);

    m = FcFontMatch(NULL, p, &r);
    ASSERT(f->font = XftFontOpenPattern(display, m), "");

    FcPatternDestroy(m);
    FcPatternDestroy(p);
}

void
xfont_init(void) {
    int i;
    struct MyFont *f;
    XGlyphInfo exts;
    char printable[PRINTABLE_END-PRINTABLE_START+2];

    for (i = 0; i <= PRINTABLE_END-PRINTABLE_START; i++)
        printable[i] = i + PRINTABLE_START;
    printable[i] = '\0';

    ASSERT(FcInit(), "can't init fontconfig");
    nfont = LEN(font_list) * 4;
    fonts = malloc(nfont * sizeof(fonts[0]));
    ASSERT(fonts != NULL, "");
    printf("nfont: %d\n", nfont);
    for (i = 0; i < nfont; i++) {
        f = &fonts[i];
        f->weight = ((i%4) / 2 == 0 ? FC_WEIGHT_REGULAR : FC_WEIGHT_BOLD);
        f->slant = ((i%4) % 2 == 0 ? FC_SLANT_ROMAN : FC_SLANT_ITALIC);
            xfont_load(font_list[i/4], f);

        if (i % 4 != 0)
            continue;
        printf("%s: (%d %d %d)\n", font_list[i/4],
            f->font->max_advance_width,
            f->font->height,
            f->font->height - f->font->descent);
    }

    font_height = fonts[0].font->height;
    font_base = fonts[0].font->height-fonts[0].font->descent;
    XftTextExtentsUtf8(display, fonts[0].font,
        (const FcChar8*)printable, strlen(printable), &exts);
    font_width = (exts.xOff + strlen(printable)-1) / strlen(printable);

    zt.width = zt.col * font_width;
    zt.height = zt.row * font_height;
    printf("size: %dx%d, %dx%d\n", zt.width, zt.height, zt.row, zt.col);
    printf("font size: %dx%d\n", font_width, font_height);
}

void
xcolor_init(void) {
    int n;
    struct MyColor mc;

    n = sizeof(XftGlyphFontSpec) * zt.col;
    specs = malloc(n);
    ASSERT(specs != NULL, "");
    printf("allocate %s for specs\n", to_bytes(n));

    for (n = 0; n < 256; n++) {
        mc = c8_to_rgb(n);
        ASSERT(!xcolor_alloc(&color8[n],
            mc.rgb[0], mc.rgb[1], mc.rgb[2]), "n: %d", n);
    }
}

void
xinit(void) {
    XSetWindowAttributes wa;
    XGCValues gcvalues;
    int ret;
    XEvent e;

    display = XOpenDisplay(NULL);
    ASSERT(display, "can't open display");

    XSetErrorHandler(xerror);
    zt.xfd = XConnectionNumber(display);

    screen = DefaultScreen(display);
    root = RootWindow(display, screen);
    visual = XDefaultVisual(display, screen);
    colormap = XDefaultColormap(display, screen);
    depth = XDefaultDepth(display, screen);
    cursor = XCreateFontCursor(display, XC_xterm);
    ret = XftColorAllocName(display, visual, colormap, BACKGROUND, &background);
    ASSERT(ret, "can't allocate color for `%s`", BACKGROUND);
    ret = XftColorAllocName(display, visual, colormap, FOREGROUND, &foreground);
    ASSERT(ret, "can't allocate color for `%s`", FOREGROUND);

    xfont_init();
    xcolor_init();

    wa.cursor = cursor;
    wa.background_pixel = background.pixel;
    wa.bit_gravity = NorthWestGravity;
    wa.colormap = colormap;
    wa.border_pixel = 0;
    wa.event_mask = StructureNotifyMask
                  | ExposureMask
                  | KeyPressMask
                  | ButtonPressMask
                  | ButtonReleaseMask
                  | ButtonMotionMask
                  | FocusChangeMask
                  ;

    window = XCreateWindow(display, root, 0, 0, zt.width,
             zt.height, 0, depth, InputOutput, visual,
             CWBackPixel | CWBorderPixel 
           | CWBitGravity | CWEventMask
           | CWColormap | CWCursor
           , &wa);

    bzero(&gcvalues, sizeof(gcvalues));
    gcvalues.graphics_exposures = False;
    gc = XCreateGC(display, root, GCGraphicsExposures, &gcvalues);
    XSetBackground(display, gc, background.pixel);

    pixmap = XCreatePixmap(display, window, zt.width, zt.height, depth);
    drawable = XftDrawCreate(display, pixmap, visual, colormap);
    XftDrawRect(drawable, &background, 0, 0, zt.width, zt.height);
    XMapWindow(display, window);
    XSync(display, False);
    for (;;){
        XNextEvent(display, &e);
        if (e.type == MapNotify)
            break;
    }
}
