#ifndef __CONFIG_H__
#define __CONFIG_H__

#define VTIDEN "\033[?6c"
#define TERM "xterm-256color"
#define BACKGROUND "gray20"
#define FOREGROUND "white"
#define LATENCY 33
#define ROW 24
#define COL 80 
#define TABSPACE 4
#define USED_COLOR UBUNTU_COLOR // UTUNBU_COLOR or XTERM_COLOR

static char *font_list[] UNUSED = {
    "Ubuntu Mono:pixelsize=20",
    "Noto Sans SC:pixelsize=18",
    "Noto Emoji:pixelsize=8",
    "Unifont:pixelsize=16",
};

#endif
