#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "tools.h"

#define TERM "xterm-256color"
#define BACKGROUND "gray20"
#define FOREGROUND "white"
#define LATENCY 1
#define ROW 24
#define COL 80 
#define USED_COLOR UBUNTU_COLOR // UTUNBU_COLOR or XTERM_COLOR

static struct {
    char *name;
    int pixelsize;
} font_list[] UNUSED = {
    {"Sarasa Mono CL", 26},
    {"Noto Emoji", 8},
    {"Unifont", 18},
};

#endif
