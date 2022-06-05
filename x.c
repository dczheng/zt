#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/Xft/Xft.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xinerama.h>

#include "zt.h"
#include "ctrl.h"

struct MyColor c8_to_rgb(unsigned char);
void twrite(char*, int);

Display *display;
Window root, window;
GC gc;
Cursor cursor;
Colormap colormap;
Visual *visual;
Atom wm_protocols, wm_delete;
XftColor background, foreground, color8[256];
XftFont *font;
XftDraw *drawable;
Pixmap pixmap;
XftGlyphFontSpec *specs;
FT_UInt space_idx;
int screen, depth;
int fw, fh, fb, nspec;

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
    return !XftColorAllocValue(display, visual, colormap, &rc, c);
}

static inline void
xcolor_free(XftColor *c) {
    XftColorFree(display, visual, colormap, c);
}

static inline FT_UInt
xfont_idx(uint32_t c) {
    FT_UInt idx = XftCharIndex(display, font, c);
    if (!idx) {
        //printf("can't draw 0x%X\n", c);
        return space_idx;
    }
    return idx;
}

void
xdraw_specs(struct MyAttr a) {
    XftColor bg, fg;
    int x, y, w;

    if (!nspec)
        return;

    fg = foreground;
    bg = background;
    y = specs[0].y - fb; 
    x = specs[0].x;
    w = specs[nspec-1].x + fw - x;

    if ((!ATTR_HAS(a, ATTR_DEFAULT_FG))) {
        if (a.fg.type == COLOR8) 
            fg = color8[a.fg.c8];
    }

    if ((!ATTR_HAS(a, ATTR_DEFAULT_BG))) {
        if (a.bg.type == COLOR8) 
            bg = color8[a.bg.c8];
    }

    if (ATTR_HAS(a, ATTR_COLOR_REVERSE))
        SWAP(fg, bg);

    XftDrawRect(drawable, &bg, x, y, w, fh);
    if (ATTR_HAS(a, ATTR_UNDERLINE)) 
        XftDrawRect(drawable, &fg, x, y+fb+1, w, 1);

    XftDrawGlyphFontSpec(drawable, &fg, specs, nspec);
    nspec = 0;
}

void
xdraw_line(struct MyChar *l, int y) {
    XRectangle r;
    struct MyChar c;
    struct MyAttr a;
    int i, x;

    r.x = 0;
    r.y = 0;
    r.width = zt.width;
    r.height = fh;

    XftDrawSetClipRectangles(drawable, 0, y, &r, 1);
    //XftDrawRect(drawable, &background, 0, y, zt.width, fh);
    for (i=0, x=0, nspec=0; i<zt.col; i++, x+=fw) {
        c = l[i];
        if (nspec == 0)
            a = c.attr;
        if (ATTR_EQUAL(a, c.attr)) {
            specs[nspec].font = font;
            specs[nspec].glyph = xfont_idx(c.c);
            specs[nspec].x = x;
            specs[nspec].y = y + fb;
            nspec++;
            continue;
        }
        xdraw_specs(a);
        i--;
        x -= fw;
    }
    if (nspec)
        xdraw_specs(a);
    XftDrawSetClip(drawable, 0);
}

void
xdraw_cursor(void) {
    static int last_y = -1;

    if (last_y != -1) {
        xdraw_line(zt.line[last_y], last_y*fh);
        last_y = zt.y;
    } else {
        last_y = zt.y;
    }

    if (MODE_HAS(MODE_TEXT_CURSOR))
        XftDrawRect(drawable, &foreground, zt.x*fw, zt.y*fh+fh-3, fw, 3);
}

void
xdraw(void) {
    int y, i, nline=0;

    //XftDrawRect(drawable, &background, 0, 0, zt.width, zt.height);
    for (i=0, y=0; i<zt.row; i++, y+=fh) {
        if (!zt.dirty[i])
            continue;
        nline++;
        xdraw_line(zt.line[i], y);
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
_Button(XEvent *ev) {
    int r, c, n, b;
    char buf[64];
    XButtonEvent *e = &ev->xbutton;

    r = e->y / fh + 1;
    c = e->x / fw + 1;
    b = e->button;
    /*
    if (b == Button3) {
        if (e->type == ButtonPress)
            twrite("ok", 2);
        return;
    }
    */

    if (!MODE_HAS(MODE_REPORT_BUTTON))
        return;

    b -= Button1;
    if (b >= 3)
        b += 64-3;
    n = snprintf(buf, sizeof(buf), "\033[<%d;%d;%d%c",
        b, c, r, e->type == ButtonRelease ? 'm' : 'M');
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
_Expose(XEvent *e __attribute__((unused))) {
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

#define H(type) \
    case type: \
        _##type(&e); \
        break; 
#define H2(type, f) \
    case type: \
        _##f(&e); \
        break; 

void
xevent(void) {
    XEvent e;
    
    while (XPending(display)){
        XNextEvent(display, &e);
        switch(e.type) {
            H(Expose)
            H(KeyPress)
            H2(ButtonPress, Button)
            H2(ButtonRelease, Button)
            H2(FocusIn, Focus)
            H2(FocusOut, Focus)
            case MapNotify:
            case KeyRelease:
            case UnmapNotify:
            case ConfigureNotify:
                break;
            case DestroyNotify:
                endrun();
                break;
            default:
                printf("Unsupport event %d\n", e.type);
        }
    }
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
    XFreePixmap(display, pixmap);
    XFreeCursor(display, cursor);
    XftDrawDestroy(drawable);
    XftFontClose(display, font);
    xcolor_free(&background);
    xcolor_free(&foreground);
    for (int i=0; i<256; i++) 
        xcolor_free(&color8[i]);
    printf("free specs\n");
    free(specs);
    close(zt.xfd);
}

void
xinit(void) {
    XSetWindowAttributes wa;
    XGCValues gcvalues;
    XineramaScreenInfo *si;
    int ns, i, px, py, ret;
    XEvent e;
    struct MyColor mc;

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
                  | FocusChangeMask
                  ;
    if (!XineramaIsActive(display)) {
        printf("Xinerama is not Active\n");
        zt.width = DisplayWidth(display, screen);
        zt.height = DisplayHeight(display, screen);
    } else {
        si = XineramaQueryScreens(display, &ns);
        xpointer(&px, &py);
        for (i=0; i<ns; i++) {
            if (px >= si[i].x_org &&
                px <= si[i].x_org+si[i].width &&
                py >= si[i].y_org &&
                py <= si[i].y_org+si[i].height)
                 break;
        }
        zt.width = si[i].width;
        zt.height = si[i].height;
        XFree(si);
    }

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

    font = XftFontOpenName(display, screen, FONT);
    ASSERT(font != NULL, "can't open font `%s`", FONT);

#define F(_s) printf("font "#_s": %d\n", font->_s)
    F(height);
    F(ascent);
    F(descent);
    F(max_advance_width);
#undef F
    fw = font->max_advance_width;
    fh = font->height;
    fb = font->height - font->descent;
    zt.col = zt.width / fw;
    zt.row = zt.height / fh;
    printf("size: %dx%d, %dx%d\n", zt.width, zt.height, zt.row, zt.col);
    printf("use font size: %dx%d\n", fw, fh);
    space_idx = XftCharIndex(display, font, ' ');

    i = sizeof(XftGlyphFontSpec) * zt.col;
    specs = malloc(i);
    ASSERT(specs != NULL, "");
    printf("allocate %s for specs\n", to_bytes(i));

    for (i=0; i<256; i++) {
        mc = c8_to_rgb(i);
        ASSERT(!xcolor_alloc(&color8[i],
            mc.rgb[0], mc.rgb[1], mc.rgb[2]), "i: %d", i);
    }
}

