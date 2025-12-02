#include <locale.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/Xft/Xft.h>
#include <X11/Xatom.h>

#include "zt.h"
#include "code.h"

void tty_resize(void);
void lresize(void);
void tty_write(char*, int);
int color_equal(struct color_t, struct color_t);

struct {
    Display *dpy;
    Window root, window;
    GC gc;
    Cursor cursor;
    Colormap colormap;
    Visual *visual;
    Pixmap pixmap;
    XftDraw *draw;
    XftColor fg, bkg, color8[256];
    XftGlyphFontSpec *specs;
    XIM im;
    XIC ic;
    int fd, screen, depth, nspec, fw, fh, fb, nfont;
    struct font_t {
        XftFont *font;
        int weight, slant;
        FcChar8 *family;
    } *fonts;
} xctx = {0};

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

static inline void
xflush(void) {
    XCopyArea(xctx.dpy, xctx.pixmap, xctx.window, xctx.gc,
        0, 0, zt.width, zt.height, 0, 0);
    XFlush(xctx.dpy);
}

static inline int
xcolor_alloc(XftColor *c,
        uint8_t r,
        uint8_t g,
        uint8_t b) {
    XRenderColor rc;

    rc.red = r << 8;
    rc.green = g << 8;
    rc.blue = b << 8;
    rc.alpha = 0xffff;
    if (!XftColorAllocValue(xctx.dpy, xctx.visual, xctx.colormap, &rc, c)) {
        LOGERR("failed to allocate color for (%u %u %u)\n", r, b, g);
        return 1;
    }
    return 0;
}

static inline void
xcolor_free(XftColor *c) {
    XftColorFree(xctx.dpy, xctx.visual, xctx.colormap, c);
}

void
xdraw_specs(struct char_t c) {
    XftColor bg, fg;
    int x, y, w, rf, rb, t;

    if (!xctx.nspec)
        return;

    fg = xctx.fg;
    bg = xctx.bkg;
    y = xctx.specs[0].y - xctx.fb;
    x = xctx.specs[0].x;
    w = xctx.nspec * c.width * xctx.fw;
    rf = rb = 1;

    t = zt.width - x - w;
    if (t > 0 && t < xctx.fw)
        w = zt.width-x;

    if (zt.debug.x)
        LOG("(%d, %d, %d, %d, %d) ", xctx.nspec, c.width, x, w,
            xctx.specs[xctx.nspec-1].x);

    if (!ISSET(c.attr, ATTR_DEFAULT_FG)) {
        switch (c.fg.type) {
        case 8:
            fg = xctx.color8[c.fg.c8];
            break;
        case 24:
            rf = xcolor_alloc(&fg, c.fg.rgb[0], c.fg.rgb[1], c.fg.rgb[2]);
            break;
        }
    }

    if (!ISSET(c.attr, ATTR_DEFAULT_BG)) {
        switch (c.bg.type) {
        case 8:
            bg = xctx.color8[c.bg.c8];
            break;
        case 24:
            rb = xcolor_alloc(&bg, c.bg.rgb[0], c.bg.rgb[1], c.bg.rgb[2]);
            break;
        }
    }

    if (ISSET(c.attr, ATTR_COLOR_REVERSE))
        SWAP(fg, bg);

    XftDrawRect(xctx.draw, &bg, x, y, w, xctx.fh);
    if (ISSET(c.attr, ATTR_UNDERLINE))
        XftDrawRect(xctx.draw, &fg, x, y + xctx.fb + 1, w, 1);
    if (ISSET(c.attr, ATTR_CROSSED_OUT))
        XftDrawRect(xctx.draw, &fg, x, y + xctx.fh / 2, w, 1);

    XftDrawGlyphFontSpec(xctx.draw, &fg, xctx.specs, xctx.nspec);
    xctx.nspec = 0;
    if (!rf)
        xcolor_free(&fg);
    if (!rb)
        xcolor_free(&bg);
}

void
xfont_lookup(struct char_t c, XftFont **f, FT_UInt *idx) {
    int i, weight, slant;

    weight = FC_WEIGHT_REGULAR;
    slant = FC_SLANT_ROMAN;

    if (ISSET(c.attr, ATTR_BOLD))
        weight = FC_WEIGHT_BOLD;

    // TODO
    if (ISSET(c.attr, ATTR_FAINT))
        weight = FC_WEIGHT_REGULAR;

    if (ISSET(c.attr, ATTR_ITALIC))
        slant = FC_SLANT_ITALIC;

    for (i = 0; i < xctx.nfont; i++) {
        if (xctx.fonts[i].weight != weight ||
            xctx.fonts[i].slant != slant)
            continue;
        *f = xctx.fonts[i].font;
        if ((*idx = XftCharIndex(xctx.dpy, *f, c.c)))
            return;
    }

    LOGERR("can't find font for %x\n", c.c);
    *f = xctx.fonts[0].font;
    *idx = XftCharIndex(xctx.dpy, *f, ' ');
}

void
xdraw_line(int k, int y) {
    XRectangle r;
    struct char_t c, c0;
    int i, x;

    r.x = 0;
    r.y = 0;
    r.width = zt.width;
    r.height = xctx.fh;

    if (zt.debug.x)
        LOG("[%3d] ", k);

    XftDrawSetClipRectangles(xctx.draw, 0, y, &r, 1);
    //XftDrawRect(xctx.draw, &xctx.bkg, 0, y, zt.width, xctx.fh);
    for (i = 0, x = 0, xctx.nspec = 0; i < zt.col;) {
        c = zt.line[k][i];
        if (xctx.nspec == 0)
            c0 = c;

        if (color_equal(c0.fg, c.fg) &&
            color_equal(c0.bg, c.bg) &&
            c0.attr == c.attr &&
            c0.width == c.width) {
            xfont_lookup(c, &xctx.specs[xctx.nspec].font,
                &xctx.specs[xctx.nspec].glyph);
            xctx.specs[xctx.nspec].x = x;
            xctx.specs[xctx.nspec].y = y + xctx.fb;
            xctx.nspec++;
            i += c0.width;
            x += c0.width * xctx.fw;
            continue;
        }
        xdraw_specs(c0);
    }

    if (zt.debug.x)
        LOG("\n");

    xdraw_specs(c0);
    XftDrawSetClip(xctx.draw, 0);
}

void
xdraw_cursor(void) {
    static int last_y = -1;

    if (last_y != -1) {
        if (last_y < zt.row)
            xdraw_line(last_y, last_y*xctx.fh);
        last_y = zt.y;
    } else {
        last_y = zt.y;
    }

    if (ISSET(zt.mode, MODE_TEXT_CURSOR))
        XftDrawRect(xctx.draw, &xctx.fg,
            zt.x*xctx.fw, (zt.y+1)*xctx.fh-3, xctx.fw, 3);
}

void
xdraw(void) {
    int y, i, nline = 0;

    //XftDrawRect(xctx.draw, &xctx.bkg, 0, 0, zt.width, zt.height);
    if (zt.debug.x) {
        LOG("size: %dx%d, %dx%d\n", zt.width, zt.height, zt.row, zt.col);
        LOG("font size: %dx%d\n", xctx.fw, xctx.fh);
    }

    for (i = 0, y = 0; i < zt.row; i++, y += xctx.fh) {
        if (!zt.dirty[i])
            continue;
        nline++;
        xdraw_line(i, y);
    }
    //LOG("drawed: %d lines\n", nline);
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
    XQueryPointer(xctx.dpy, xctx.root, &dw, &dw, x, y, &di, &di, &dui);
}

void
xresize() {
    XFreePixmap(xctx.dpy, xctx.pixmap);
    xctx.pixmap = XCreatePixmap(xctx.dpy, xctx.window,
        zt.width, zt.height, xctx.depth);
    XftDrawChange(xctx.draw, xctx.pixmap);
    XftDrawRect(xctx.draw, &xctx.bkg, 0, 0, zt.width, zt.height);
    ASSERT(xctx.specs = realloc(xctx.specs, zt.col*sizeof(XftGlyphFontSpec)));
}

void
_Mouse(XEvent *ev) {
    int r, c, b, n;
    static int or=-1, oc=-1, ob=-1;
    char t;
    static char buf[64];

    r = ev->xbutton.y / xctx.fh + 1;
    c = ev->xbutton.x / xctx.fw + 1;
    b = ev->xbutton.button-Button1;
    if (b >= 3)
        b += 64-3;

    if (!ISSET(zt.mode, MODE_MOUSE))
        return;

    switch (ev->xbutton.type) {
    case ButtonPress:
        if (!ISSET(zt.mode, MODE_MOUSE_PRESS))
            return;
        t = 'M';
        or = r;
        oc = c;
        ob = b;
        break;

    case ButtonRelease:
        if (!ISSET(zt.mode, MODE_MOUSE_RELEASE))
            return;
        t = 'm';
        break;

    case MotionNotify:
        if (!ISSET(zt.mode, MODE_MOUSE_MOTION_PRESS) &&
            !ISSET(zt.mode, MODE_MOUSE_MOTION_ANY))
            return;
        if (r == or && c == oc)
            return;
        t = 'M';
        or = r;
        oc = c;
        b = ob+32;
        break;

    default:
        LOGERR("Unsupported button type: %d\n",
            ev->xbutton.type);
        return;
    }

    if (ISSET(zt.mode, MODE_MOUSE_EXT)) {
        n = snprintf(buf, sizeof(buf),
            "\033[<%d;%d;%d%c", b, c, r, t);
    } else {
        n = snprintf(buf, sizeof(buf),
            "\033[M%c%c%c", 32+b, 32+c, 32+r);
    }
    tty_write(buf, n);
}

void
_Focus(XEvent *ev) {
    if (!ISSET(zt.mode, MODE_SEND_FOCUS))
        return;
    if (ev->type == FocusIn) {
        if (xctx.ic)
            XSetICFocus(xctx.ic);
        tty_write("\033[I", 3);
    } else {
        if (xctx.ic)
            XUnsetICFocus(xctx.ic);
        tty_write("\033[O", 3);
    }
}

void
_Expose(XEvent *ev __unused) {
    xflush();
}

void
_KeyPress(XEvent *ev) {
    int n;
    char buf[128];
    KeySym ksym;
    XKeyEvent *e = &ev->xkey;
    Status status;

    if (xctx.ic) {
        n = XmbLookupString(xctx.ic, e, buf, sizeof(buf), &ksym, &status);
        if (status == XBufferOverflow) {
            LOGERR("xim buffer overflow\n");
            return;
        }
    } else  {
        n = XLookupString(e, buf, sizeof(buf), &ksym, NULL);
    }

    xkeymap(ksym, e->state, buf, &n);
    //dump((uint8_t*)buf, n);
    tty_write(buf, n);
}

void
_ConfigureNotify(XEvent *ev) {
    int w, h, r, c;

    w = ev->xconfigure.width;
    h = ev->xconfigure.height;

    if (w == zt.width && h == zt.height)
        return;

    r = h / xctx.fh;
    c = w / xctx.fw;
    r = MAX(r, 8);
    c = MAX(c, 8);

    zt.row_old = zt.row;
    zt.col_old = zt.col;
    zt.width = w;
    zt.height = h;
    zt.row = r;
    zt.col = c;

    xresize();
    tty_resize();
    lresize();
}

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

    for (;XPending(xctx.dpy);) {
        XNextEvent(xctx.dpy, &e);
        if (XFilterEvent(&e, None))
            continue;
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
        case MappingNotify:
        case KeyRelease:
        case UnmapNotify:
            break;
        case DestroyNotify:
            return EOF;
        default:
            LOGERR("Unsupport event %d\n", e.type);
        }
    }
    return 0;
}
#undef H
#undef H2

int
xerror(Display *dpy, XErrorEvent *e) {
    XSync(dpy, False);
    LOGERR("xerror: %d\n", e->error_code);
    return 0;
}

void
xfree(void) {
    int i;

    if (xctx.ic) XDestroyIC(xctx.ic);
    if (xctx.im) XCloseIM(xctx.im);
    XFreePixmap(xctx.dpy, xctx.pixmap);
    XFreeCursor(xctx.dpy, xctx.cursor);
    XftDrawDestroy(xctx.draw);

    for (i = 0; i < xctx.nfont; i++) {
        free(xctx.fonts[i].family);
        XftFontClose(xctx.dpy, xctx.fonts[i].font);
    }
    free(xctx.fonts);

    xcolor_free(&xctx.bkg);
    xcolor_free(&xctx.fg);
    for (i = 0; i < 256; i++)
        xcolor_free(&xctx.color8[i]);
    free(xctx.specs);
    close(xctx.fd);
}

void
xfont_load(char *str, struct font_t *f) {
    FcPattern *p, *m;
    FcResult r;

    ASSERT(p = FcNameParse((FcChar8*)str));
    FcConfigSubstitute(NULL, p, FcMatchPattern);
    FcDefaultSubstitute(p);
    XftDefaultSubstitute(xctx.dpy, xctx.screen, p);

    FcPatternDel(p, FC_WEIGHT);
    FcPatternAddInteger(p, FC_WEIGHT, f->weight);

    FcPatternDel(p, FC_SLANT);
    FcPatternAddInteger(p, FC_SLANT, f->slant);

    m = FcFontMatch(NULL, p, &r);
    ASSERT(f->font = XftFontOpenPattern(xctx.dpy, m));
    f->family = FcPatternFormat(m, (FcChar8*)"%{family}");

    FcPatternDestroy(m);
    FcPatternDestroy(p);
}

void
xfont_init(void) {
    int i, j, size;
    struct font_t *f;
    XGlyphInfo exts;
    char printable[257], buf[128];

    for (i = 0, j = 0; i < (int)sizeof(printable); i++) {
        if (isprint(i))
            printable[j++] = i;
    }
    printable[j] = '\0';

    ASSERT(FcInit());
    xctx.nfont = LEN(font_list) * 4;
    ASSERT(xctx.fonts = malloc(xctx.nfont * sizeof(xctx.fonts[0])));
    for (i = 0; i < xctx.nfont; i++) {
        f = &xctx.fonts[i];
        f->weight = ((i%4) / 2 == 0 ? FC_WEIGHT_REGULAR : FC_WEIGHT_BOLD);
        f->slant = ((i%4) % 2 == 0 ? FC_SLANT_ROMAN : FC_SLANT_ITALIC);

        size = font_list[i/4].size * zt.fontsize;
        size = MAX(size, 1);

        snprintf(buf, sizeof(buf), "%s:pixelsize=%d",
            font_list[i/4].name, size);
        xfont_load(buf, f);

        if (i % 4 != 0)
            continue;
    }

    xctx.fh = xctx.fonts[0].font->height;
    xctx.fb = xctx.fonts[0].font->height-xctx.fonts[0].font->descent;
    XftTextExtentsUtf8(xctx.dpy, xctx.fonts[0].font,
        (const FcChar8*)printable, strlen(printable), &exts);
    xctx.fw = (exts.xOff + strlen(printable)-1) / strlen(printable);

    zt.width = zt.col * xctx.fw;
    zt.height = zt.row * xctx.fh;
}

void
xcolor_init(void) {
    int i;
    uint8_t r, g, b;

    ASSERT(xctx.specs = malloc(sizeof(XftGlyphFontSpec)*zt.col));
    for (i = 0; i < 256; i++) {
        if (i <= 15) {
            r = standard_colors[i].r;
            g = standard_colors[i].g;
            b = standard_colors[i].b;
        } else if (i >= 16 && i <= 231) {
            // 6 x 6 x 6 = 216 cube colors
            // 16 + 36*r + 6*g + b
            r = ((i-16) / 36)     * 40 + 55;
            g = ((i-16) % 36 / 6) * 40 + 55;
            b = ((i-16) % 6)      * 40 + 55;
        } else {
            // 24-step grayscale
            r = g = b = (i-232) * 11;
        }
        ASSERT(!xcolor_alloc(&xctx.color8[i], r, g, b));
    }
}

int xim_init(void);
void
_xim_init(Display *dpy __unused, XPointer client __unused,
    XPointer call __unused) {
    if (!xim_init())
        XUnregisterIMInstantiateCallback(xctx.dpy, NULL, NULL, NULL,
            _xim_init, NULL);
}

void
xim_destroy(XIM im __unused, XPointer client __unused,
    XPointer call __unused) {
    xctx.im = NULL;
    xctx.ic = NULL;
    XRegisterIMInstantiateCallback(xctx.dpy, NULL, NULL, NULL,
            _xim_init, NULL);
}

int
xim_init(void) {
    XIMCallback cb = {.client_data = NULL, .callback = xim_destroy};

    if (!(xctx.im = XOpenIM(xctx.dpy, NULL, NULL, NULL))) {
        LOGERR("can't init xim\n");
        return 1;
    }
    ASSERT(!XSetIMValues(xctx.im, XNDestroyCallback, &cb, NULL));
    ASSERT(xctx.ic = XCreateIC(xctx.im,
        XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
        XNClientWindow, xctx.window, NULL));
    XSetICFocus(xctx.ic);
    return 0;
}

int
xinit(void) {
    XSetWindowAttributes wa;
    XGCValues gcvalues;
    XEvent e;

    setlocale(LC_CTYPE, "");
    XSetLocaleModifiers("");

    xctx.dpy = XOpenDisplay(NULL);
    ASSERT(xctx.dpy);

    XSetErrorHandler(xerror);
    xctx.fd = XConnectionNumber(xctx.dpy);

    xctx.screen = DefaultScreen(xctx.dpy);
    xctx.root = RootWindow(xctx.dpy, xctx.screen);
    xctx.visual = XDefaultVisual(xctx.dpy, xctx.screen);
    xctx.colormap = XDefaultColormap(xctx.dpy, xctx.screen);
    xctx.depth = XDefaultDepth(xctx.dpy, xctx.screen);
    xctx.cursor = XCreateFontCursor(xctx.dpy, XC_xterm);

    ASSERT(XftColorAllocName(xctx.dpy, xctx.visual, xctx.colormap,
        FOREGROUND, &xctx.fg));
    ASSERT(XftColorAllocName(xctx.dpy, xctx.visual, xctx.colormap,
        BACKGROUND, &xctx.bkg));

    xfont_init();
    xcolor_init();

    wa.cursor = xctx.cursor;
    wa.background_pixel = xctx.bkg.pixel;
    wa.bit_gravity = NorthWestGravity;
    wa.colormap = xctx.colormap;
    wa.border_pixel = 0;
    wa.event_mask = StructureNotifyMask
                  | ExposureMask
                  | KeyPressMask
                  | ButtonPressMask
                  | ButtonReleaseMask
                  | ButtonMotionMask
                  | FocusChangeMask
                  ;

    xctx.window = XCreateWindow(xctx.dpy, xctx.root, 0, 0, zt.width,
             zt.height, 0, xctx.depth, InputOutput, xctx.visual,
             CWBackPixel | CWBorderPixel
           | CWBitGravity | CWEventMask
           | CWColormap | CWCursor
           , &wa);

    ZERO(gcvalues);
    gcvalues.graphics_exposures = False;
    xctx.gc = XCreateGC(xctx.dpy, xctx.root, GCGraphicsExposures, &gcvalues);
    XSetBackground(xctx.dpy, xctx.gc, xctx.bkg.pixel);

    xctx.pixmap = XCreatePixmap(xctx.dpy, xctx.window,
        zt.width, zt.height, xctx.depth);
    xctx.draw = XftDrawCreate(xctx.dpy, xctx.pixmap,
        xctx.visual, xctx.colormap);
    XftDrawRect(xctx.draw, &xctx.bkg, 0, 0, zt.width, zt.height);

    if (xim_init())
        XRegisterIMInstantiateCallback(xctx.dpy, NULL, NULL, NULL,
            _xim_init, NULL);

    XMapWindow(xctx.dpy, xctx.window);
    XSync(xctx.dpy, False);

    for (;;) {
        XNextEvent(xctx.dpy, &e);
        if (e.type == MapNotify)
            break;
    }
    return xctx.fd;
}
