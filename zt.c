#include <getopt.h>
#include <locale.h>
#include <sys/select.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <X11/cursorfont.h>

#include "term/term.h"

#define FOREGROUND "white"
#define BACKGROUND "gray20"
#define LATENCY (10 * MILLISECOND)

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
    int screen, depth, nspec, fw, fh, fb,
        nfont, fontcap, width, height, xfd;
    struct {
        XftFont *font;
        int weight, slant;
    } *fonts;
    struct {
        double fontsize;
        char *term;
        int debug, no_ignore;
    } arg;
} zt = {0};
struct term_t term = {0};

static inline void
xflush(void) {
    XCopyArea(zt.dpy, zt.pixmap, zt.window, zt.gc,
        0, 0, zt.width, zt.height, 0, 0);
    XFlush(zt.dpy);
}

static inline int
xcolor_alloc(XftColor *c, uint8_t r, uint8_t g, uint8_t b) {
    XRenderColor rc;
    rc.red = r << 8;
    rc.green = g << 8;
    rc.blue = b << 8;
    rc.alpha = 0xffff;
    if (!XftColorAllocValue(zt.dpy, zt.visual, zt.colormap, &rc, c)) {
        LOGERR("failed to allocate color for (%u %u %u)\n", r, b, g);
        return 1;
    }
    return 0;
}

static inline void
xcolor_free(XftColor *c) {
    XftColorFree(zt.dpy, zt.visual, zt.colormap, c);
}

void
xdraw_specs(struct term_char_t c) {
    XftColor bg, fg, a;
    int x, y, w, rf, rb, t;

    if (!zt.nspec)
        return;

    fg = zt.fg;
    bg = zt.bkg;
    y = zt.specs[0].y - zt.fb;
    x = zt.specs[0].x;
    w = zt.nspec * c.width * zt.fw;
    rf = rb = 1;

    t = zt.width - x - w;
    if (t > 0 && t < zt.fw)
        w = zt.width-x;

    if (!MODE_ISSET(&c, CHAR_MODE_DEFAULT_FG)) {
        switch (c.fg.type) {
        case 8: fg = zt.color8[c.fg.c8]; break;
        case 24:
            if (!(rf = xcolor_alloc(&a, c.fg.rgb[0], c.fg.rgb[1], c.fg.rgb[2])))
                fg = a;
            break;
        }
    }

    if (!MODE_ISSET(&c, CHAR_MODE_DEFAULT_BG)) {
        switch (c.bg.type) {
        case 8: bg = zt.color8[c.bg.c8]; break;
        case 24:
            if (!(rb = xcolor_alloc(&a, c.bg.rgb[0], c.bg.rgb[1], c.bg.rgb[2])))
                bg = a;
            break;
        }
    }

    if (MODE_ISSET(&c, CHAR_MODE_COLOR_REVERSE))
        SWAP(fg, bg);

    XftDrawRect(zt.draw, &bg, x, y, w, zt.fh);
    if (MODE_ISSET(&c, CHAR_MODE_UNDERLINE))
        XftDrawRect(zt.draw, &fg, x, y + zt.fb + 1, w, 1);
    if (MODE_ISSET(&c, CHAR_MODE_CROSSED_OUT))
        XftDrawRect(zt.draw, &fg, x, y + zt.fh / 2, w, 1);

    XftDrawGlyphFontSpec(zt.draw, &fg, zt.specs, zt.nspec);
    zt.nspec = 0;
    if (!rf) xcolor_free(&fg);
    if (!rb) xcolor_free(&bg);
}

void
xfont_lookup(struct term_char_t c, XftFont **f, FT_UInt *idx) {
    int i, weight, slant;

    weight = FC_WEIGHT_REGULAR;
    slant = FC_SLANT_ROMAN;

    if (MODE_ISSET(&c, CHAR_MODE_BOLD))
        weight = FC_WEIGHT_BOLD;

    // TODO
    if (MODE_ISSET(&c, CHAR_MODE_FAINT))
        weight = FC_WEIGHT_REGULAR;

    if (MODE_ISSET(&c, CHAR_MODE_ITALIC))
        slant = FC_SLANT_ITALIC;

    for (i = 0; i < zt.nfont; i++) {
        if (zt.fonts[i].weight != weight ||
            zt.fonts[i].slant != slant)
            continue;
        *f = zt.fonts[i].font;
        if ((*idx = XftCharIndex(zt.dpy, *f, c.c)))
            return;
    }

    if (zt.arg.debug < 0)
        LOGERR("can't find font for 0x%x\n", c.c);
    *f = zt.fonts[0].font;
    *idx = XftCharIndex(zt.dpy, *f, ' ');
}

void
xdraw_line(int k, int y) {
    XRectangle r;
    struct term_char_t c, c0;
    int i, x;

    r.x = 0;
    r.y = 0;
    r.width = zt.width;
    r.height = zt.fh;

    XftDrawSetClipRectangles(zt.draw, 0, y, &r, 1);
    //XftDrawRect(zt.draw, &zt.bkg, 0, y, zt.width, zt.fh);
    for (i = 0, x = 0, zt.nspec = 0; i < term.col;) {
        c = term.line[k][i];
        if (zt.nspec == 0)
            c0 = c;

        if (term_attr_equal(&c0, &c)) {
            xfont_lookup(c, &zt.specs[zt.nspec].font,
                &zt.specs[zt.nspec].glyph);
            zt.specs[zt.nspec].x = x;
            zt.specs[zt.nspec].y = y + zt.fb;
            zt.nspec++;
            i += c0.width;
            x += c0.width * zt.fw;
            continue;
        }
        xdraw_specs(c0);
    }

    xdraw_specs(c0);
    XftDrawSetClip(zt.draw, 0);
}

void
xdraw_cursor(void) {
    static int last_y = -1;

    if (last_y != -1) {
        if (last_y < term.row)
            xdraw_line(last_y, last_y*zt.fh);
        last_y = term.y;
    } else {
        last_y = term.y;
    }

    if (MODE_ISSET(&term, MODE_CURSOR))
        XftDrawRect(zt.draw, &zt.fg,
            term.x*zt.fw, (term.y+1)*zt.fh-3, zt.fw, 3);
}

void
xdraw(void) {
    for (int i = 0, y = 0; i < term.row; i++, y += zt.fh)
        if (term.dirty[i])
            xdraw_line(i, y);
    xdraw_cursor();
    xflush();
    term_flush(&term);
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
    case XK_Up: *len = snprintf(buf, 4, "\033[A"); break;
    case XK_Down: *len = snprintf(buf, 4, "\033[B"); break;
    case XK_Right: *len = snprintf(buf, 4, "\033[C"); break;
    case XK_Left: *len = snprintf(buf, 4, "\033[D"); break;
    }
}

void
xresize() {
    XFreePixmap(zt.dpy, zt.pixmap);
    zt.pixmap = XCreatePixmap(zt.dpy, zt.window,
        zt.width, zt.height, zt.depth);
    XftDrawChange(zt.draw, zt.pixmap);
    XftDrawRect(zt.draw, &zt.bkg, 0, 0, zt.width, zt.height);
    ASSERT(zt.specs = realloc(zt.specs, term.col*sizeof(XftGlyphFontSpec)));
}

// https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h2-Mouse-Tracking
void
_Mouse(XEvent *ev) {
    static struct {
        int x, y, b;
    } m, mlast;
    int n;
    char buf[64];

    if (!MODE_ISSET(&term, MODE_MOUSE))
        return;

    m.y = ev->xbutton.y / zt.fh + 1;
    m.x = ev->xbutton.x / zt.fw + 1;
    m.b = ev->xbutton.button - Button1 + 1;

    switch (ev->type) {
    case ButtonPress:
        if (!MODE_ISSET(&term, MODE_MOUSE_PRESS))
            return;
        mlast = m;
        break;
    case ButtonRelease:
        if (!MODE_ISSET(&term, MODE_MOUSE_RELEASE))
            return;
        break;
    case MotionNotify:
        if (!MODE_ISSET(&term, MODE_MOUSE_MOTION))
            return;
        if (m.x == mlast.x && m.y == mlast.y)
            return;
        mlast.x = mlast.x;
        mlast.y = mlast.y;
        m.b = mlast.b;
        break;
    default: DIE();
    }

    if (!MODE_ISSET(&term, MODE_MOUSE_SGR) && ev->type == ButtonRelease)
        m.b = 3;
    else if (m.b >= 8)
        m.b = m.b - 8 + 128;
    else if (m.b >= 4)
        m.b = m.b - 4 + 64;
    else
        m.b--;

    m.b += ev->type == MotionNotify ? 32 : 0;
    if (MODE_ISSET(&term, MODE_MOUSE_SGR))
        n = snprintf(buf, sizeof(buf), "\033[<%d;%d;%d%c", m.b, m.x, m.y,
            ev->type == ButtonRelease ? 'm' : 'M');
    else
        n = snprintf(buf, sizeof(buf), "\033[M%c%c%c", 32+m.b, 32+m.x, 32+m.y);

    term_write(&term, buf, n);
}

void
_Focus(XEvent *ev) {
    if (!MODE_ISSET(&term, MODE_FOCUS))
        return;
    if (ev->type == FocusIn) {
        if (zt.ic)
            XSetICFocus(zt.ic);
        term_write(&term, "\033[I", 3);
    } else {
        if (zt.ic)
            XUnsetICFocus(zt.ic);
        term_write(&term, "\033[O", 3);
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

    if (zt.ic) {
        n = XmbLookupString(zt.ic, e, buf, sizeof(buf), &ksym, &status);
        if (status == XBufferOverflow) {
            LOGERR("xim buffer overflow\n");
            return;
        }
    } else  {
        n = XLookupString(e, buf, sizeof(buf), &ksym, NULL);
    }

    xkeymap(ksym, e->state, buf, &n);
    //dump((uint8_t*)buf, n);
    term_write(&term, buf, n);
}

void
_ConfigureNotify(XEvent *ev) {
    int w, h, r, c;

    w = ev->xconfigure.width;
    h = ev->xconfigure.height;

    if (w == zt.width && h == zt.height)
        return;

    r = h / zt.fh;
    c = w / zt.fw;
    r = MAX(r, 8);
    c = MAX(c, 8);

    zt.width = w;
    zt.height = h;
    term_resize(&term, r, c, w, h);
    xresize();
}

#define H(type) case type: _##type(&e); break;
#define H2(type, f) case type: _##f(&e); break;
int
xevent(void) {
    XEvent e;
    for (;XPending(zt.dpy);) {
        XNextEvent(zt.dpy, &e);
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

    if (zt.ic) XDestroyIC(zt.ic);
    if (zt.im) XCloseIM(zt.im);
    XFreePixmap(zt.dpy, zt.pixmap);
    XFreeCursor(zt.dpy, zt.cursor);
    XftDrawDestroy(zt.draw);

    for (i = 0; i < zt.nfont; i++)
        XftFontClose(zt.dpy, zt.fonts[i].font);
    free(zt.fonts);

    xcolor_free(&zt.bkg);
    xcolor_free(&zt.fg);
    for (i = 0; i < 256; i++)
        xcolor_free(&zt.color8[i]);
    free(zt.specs);
    close(zt.xfd);
}

void
xfont_open(char *name, int size, int weight, int slant) {
    FcPattern *p, *m;
    char buf[128], *info = NULL;
    XftFont *f;
    FcResult r;

    size *= zt.arg.fontsize;
    size = MAX(size, 5);
    snprintf(buf, sizeof(buf), "%s:pixelsize=%d", name, size);

    ASSERT(p = FcNameParse((FcChar8*)buf));
    FcPatternDel(p, FC_WEIGHT);
    FcPatternAddInteger(p, FC_WEIGHT, weight);
    FcPatternDel(p, FC_SLANT);
    FcPatternAddInteger(p, FC_SLANT, slant);

    ASSERT(m = FcFontMatch(NULL, p, &r));
    ASSERT(f = XftFontOpenPattern(zt.dpy, m));

    if (zt.nfont >= zt.fontcap) {
        zt.fontcap += 4;
        ASSERT(zt.fonts = realloc(zt.fonts,
            zt.fontcap * sizeof(zt.fonts[0])));
    }
    zt.fonts[zt.nfont].font = f;
    zt.fonts[zt.nfont].weight = weight;
    zt.fonts[zt.nfont].slant = slant;
    zt.nfont++;

    info = (char*)FcPatternFormat(m,
        (FcChar8*)"%{family} %{style} %{pixelsize}");

    if (zt.arg.debug < 0)
        LOG("[%02d] %s\n", zt.nfont, info);

    FcPatternDestroy(m);
    FcPatternDestroy(p);
    free(info);
}

void
xfont_init(void) {
    int i, j;
    XftFont *f;
    XGlyphInfo exts;
    char printable[257];

    for (i = 0, j = 0; i < (int)sizeof(printable); i++)
        if (isprint(i))
            printable[j++] = i;
    printable[j] = '\0';

    ASSERT(FcInit());
    for (i = 0; i < LEN(font_list); i++) {
        xfont_open(font_list[i].name, font_list[i].size,
            FC_WEIGHT_REGULAR, FC_SLANT_ROMAN);
        xfont_open(font_list[i].name, font_list[i].size,
            FC_WEIGHT_REGULAR, FC_SLANT_ITALIC);
        xfont_open(font_list[i].name, font_list[i].size,
            FC_WEIGHT_BOLD, FC_SLANT_ROMAN);
        xfont_open(font_list[i].name, font_list[i].size,
            FC_WEIGHT_BOLD, FC_SLANT_ITALIC);
    }

    f = zt.fonts[0].font;
    zt.fh = f->height;
    zt.fb = f->height-f->descent;
    XftTextExtentsUtf8(zt.dpy, f,
        (const FcChar8*)printable, strlen(printable), &exts);
    zt.fw = (exts.xOff + strlen(printable)-1) / strlen(printable);

    zt.width = term.col * zt.fw;
    zt.height = term.row * zt.fh;
}

void
xcolor_init(void) {
    int i;
    uint8_t r, g, b;

    ASSERT(zt.specs = malloc(sizeof(XftGlyphFontSpec)*term.col));
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
        ASSERT(!xcolor_alloc(&zt.color8[i], r, g, b));
    }
}

int xim_init(void);
void
_xim_init(Display *dpy __unused, XPointer client __unused,
    XPointer call __unused) {
    if (!xim_init())
        XUnregisterIMInstantiateCallback(zt.dpy, NULL, NULL, NULL,
            _xim_init, NULL);
}

void
xim_destroy(XIM im __unused, XPointer client __unused,
    XPointer call __unused) {
    zt.im = NULL;
    zt.ic = NULL;
    XRegisterIMInstantiateCallback(zt.dpy, NULL, NULL, NULL,
            _xim_init, NULL);
}

int
xim_init(void) {
    XIMCallback cb = {.client_data = NULL, .callback = xim_destroy};

    if (!(zt.im = XOpenIM(zt.dpy, NULL, NULL, NULL))) {
        LOGERR("can't init xim\n");
        return 1;
    }
    ASSERT(!XSetIMValues(zt.im, XNDestroyCallback, &cb, NULL));
    ASSERT(zt.ic = XCreateIC(zt.im,
        XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
        XNClientWindow, zt.window, NULL));
    XSetICFocus(zt.ic);
    return 0;
}

void
xinit(void) {
    XSetWindowAttributes wa;
    XGCValues gcvalues;
    XEvent e;

    setlocale(LC_CTYPE, "");
    XSetLocaleModifiers("");

    zt.dpy = XOpenDisplay(NULL);
    ASSERT(zt.dpy);

    XSetErrorHandler(xerror);
    zt.xfd = XConnectionNumber(zt.dpy);

    zt.screen = DefaultScreen(zt.dpy);
    zt.root = RootWindow(zt.dpy, zt.screen);
    zt.visual = XDefaultVisual(zt.dpy, zt.screen);
    zt.colormap = XDefaultColormap(zt.dpy, zt.screen);
    zt.depth = XDefaultDepth(zt.dpy, zt.screen);
    zt.cursor = XCreateFontCursor(zt.dpy, XC_xterm);

    ASSERT(XftColorAllocName(zt.dpy, zt.visual, zt.colormap,
        FOREGROUND, &zt.fg));
    ASSERT(XftColorAllocName(zt.dpy, zt.visual, zt.colormap,
        BACKGROUND, &zt.bkg));

    xfont_init();
    xcolor_init();

    wa.cursor = zt.cursor;
    wa.background_pixel = zt.bkg.pixel;
    wa.bit_gravity = NorthWestGravity;
    wa.colormap = zt.colormap;
    wa.border_pixel = 0;
    wa.event_mask = StructureNotifyMask
                  | ExposureMask
                  | KeyPressMask
                  | ButtonPressMask
                  | ButtonReleaseMask
                  | ButtonMotionMask
                  | FocusChangeMask
                  ;

    zt.window = XCreateWindow(zt.dpy, zt.root, 0, 0, zt.width,
             zt.height, 0, zt.depth, InputOutput, zt.visual,
             CWBackPixel | CWBorderPixel
           | CWBitGravity | CWEventMask
           | CWColormap | CWCursor
           , &wa);

    ZERO(gcvalues);
    gcvalues.graphics_exposures = False;
    zt.gc = XCreateGC(zt.dpy, zt.root, GCGraphicsExposures, &gcvalues);
    XSetBackground(zt.dpy, zt.gc, zt.bkg.pixel);

    zt.pixmap = XCreatePixmap(zt.dpy, zt.window,
        zt.width, zt.height, zt.depth);
    zt.draw = XftDrawCreate(zt.dpy, zt.pixmap,
        zt.visual, zt.colormap);
    XftDrawRect(zt.draw, &zt.bkg, 0, 0, zt.width, zt.height);

    if (xim_init())
        XRegisterIMInstantiateCallback(zt.dpy, NULL, NULL, NULL,
            _xim_init, NULL);

    XMapWindow(zt.dpy, zt.window);
    XSync(zt.dpy, False);

    for (;;) {
        XNextEvent(zt.dpy, &e);
        if (e.type == MapNotify)
            break;
    }
}

int
main(int argc, char **argv) {
    int ret, i;
    struct timespec tv;
    long tlast = 0;
    fd_set fds;
    struct option opts[] = {
        {"font-size", required_argument, NULL, 1},
        {"term", required_argument, NULL, 2},
        {"debug", required_argument, NULL, 3},
        {"no-ignore", no_argument, NULL, 4},
        {0, 0, 0, 0}
    };

    zt.arg.fontsize = 1;
    zt.arg.term = "xterm-256color";
    while ((i = getopt_long_only(argc, argv, "", opts, NULL)) != -1) {
        switch(i) {
        case 1: stod(&zt.arg.fontsize, optarg); break;
        case 2: zt.arg.term = optarg; break;
        case 3: stoi(&zt.arg.debug, optarg); break;
        case 4: zt.arg.no_ignore = 1; break;
        }
    }

    term_init(&term, zt.arg.term);
    term.debug = zt.arg.debug;
    term.no_ignore = zt.arg.no_ignore;

    xinit();

    tv = to_timespec(500 * MILLISECOND);

    for (;;) {
        FD_ZERO(&fds);
        FD_SET(zt.xfd, &fds);
        FD_SET(term.tty, &fds);
        ASSERT((ret = pselect(MAX(zt.xfd, term.tty)+1, &fds, NULL, NULL,
            &tv, NULL)) >= 0);
        if (!ret) continue;

        if (FD_ISSET(zt.xfd, &fds) && xevent())
            break;

        if (FD_ISSET(term.tty, &fds)) {
            if (get_time() - tlast < LATENCY)
                continue;
            tlast = get_time();

            if (term_read(&term))
                break;

            xdraw();
        }

    }

    xfree();
    term_free(&term);
    return 0;
}
