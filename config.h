#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "tool.h"

#define TERM "xterm-256color"
#define BACKGROUND "gray20"
#define FOREGROUND "white"
#define LATENCY 33
#define ROW 24
#define COL 80 
#define USED_COLOR UBUNTU_COLOR // UTUNBU_COLOR or XTERM_COLOR

static char *font_list[] UNUSED = {
    "Sarasa Mono CL:pixelsize=20",
    "Noto Emoji:pixelsize=8",
    "Unifont:pixelsize=16",
};

#endif
