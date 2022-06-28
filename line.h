#ifndef __LINE_H__
#define __LINE_H__

#include "stdint.h"
#include "color.h"

typedef uint32_t MyRune;
struct MyChar {
    MyRune c;
    char width;
    struct MyColor fg, bg;
    unsigned int flag;
};

void linit(void);
void lclean(void);
void ldirty_reset(void);
void lresize(void);
void lclear(int, int, int, int);
void lclear_all();
void lerase(int, int, int);
void lscroll_up(int, int);
void lscroll_down(int, int);
void lnew(void);
void lwrite(MyRune);
void linsert(int);
void ldelete(int);
void lmoveto(int, int);
void lsettb(int, int);
void ltab(int);
void ltab_clear(void);
void lrepeat_last(int);
void twrite(char*, int);
void linsert_blank(int);
void ldelete_char(int);
void lcursor(int);
void ldirty_all(void);

#endif
