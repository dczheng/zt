#ifndef __CODE_H__
#define __CODE_H__

#include "zt.h"

/*
  References
  https://en.wikipedia.org/wiki/ANSI_escape_code
  https://en.wikipedia.org/wiki/C0_and_C1_control_codes
  https://vt100.net/docs
  https://vt100.net/docs/vt102-ug/contents.html
  https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
*/

// C0
#define NUL 0x00 // Null
#define SOH 0x01 // Start of Heading
#define STX 0x02 // Start of text
#define ETX 0x03 // End of text
#define EOT 0x04 // End of transmission
#define ENQ 0x05 // Enquiry
#define ACK 0x06 // Acknowledge
#define BEL 0x07 // Bell
#define BS  0x08 // Backspace
#define HT  0x09 // Horizontal aabulation
#define LF  0x0a // Line feed
#define VT  0x0b // Vertical tabulation
#define FF  0x0c // Form feed
#define CR  0x0d // Carriage return
#define SO  0x0e // Shift out
#define SI  0x0f // Shift in
#define DLE 0x10 // DataLink escape
#define DC1 0x11 // DeviceControl 1
#define DC2 0x12 // DeviceControl 2
#define DC3 0x13 // DeviceControl 3
#define DC4 0x14 // DeviceControl 4
#define NAK 0x15 // Negative acknowledge
#define SYN 0x16 // Synchronous idle
#define ETB 0x17 // End of transmission block
#define CAN 0x18 // Cancel
#define EM  0x19 // End of medium
#define SUB 0x1a // Substitute
#define ESC 0x1b // Escape
#define FS  0x1c // File separator
#define GS  0x1d // Group separator
#define RS  0x1e // Record separator
#define US  0x1f // Unit separator

// other
#define DEL  0x7f // Delete

// C1
#define PAD  0x80 // Padding character
#define HOP  0x81 // High octet preset
#define BPH  0x82 // Break permitted here
#define NBH  0x83 // No break here
#define IND  0x84 // Index
#define NEL  0x85 // Next line
#define SSA  0x86 // Start of selected area
#define ESA  0x87 // End of selected area
#define HTS  0x88 // Horizontal tabulationSet
#define HTJ  0x89 // Horizontal tabulation with justification
#define VTS  0x8a // Vertical tabulationSet
#define PLD  0x8b // Partial line down
#define PLU  0x8c // Partial line up
#define RI   0x8d // Reverse index
#define SS2  0x8e // Single shift 2
#define SS3  0x8f // Single shift 3
#define DCS  0x90 // Device control string
#define PU1  0x91 // Private use 1
#define PU2  0x92 // Private use 2
#define STS  0x93 // Transmit state
#define CCH  0x94 // Cancel character
#define MW   0x95 // Message waiting
#define SPA  0x96 // Start of protected area
#define EPA  0x97 // End of protected area
#define SOS  0x98 // Start of string
#define SGCI 0x99 // Single graphic character introducer
#define SCI  0x9a // Single character introducer
#define CSI  0x9b // Control sequence introducer
#define ST   0x9c // String terminator
#define OSC  0x9d // Operating system command
#define PM   0x9e // Privacy message
#define APC  0x9f // Application program command

// CSI
#define ICH     '@' // Insert Blank char
#define CUU     'A' // Cursor up
#define CUD     'B' // Cursor down
#define CUF     'C' // Cursor forward
#define CUB     'D' // Cursor backward
#define CNL     'E' // Cursor next line
#define CPL     'F' // Cursor previous line
#define CHA     'G' // Cursor horizontal absolute
#define CUP     'H' // Cursor position
#define CHT     'I' // Cursor forward tabulation
#define ED      'J' // Erase in display
#define EL      'K' // Erase in line
#define IL      'L' // Insert line
#define DL      'M' // Delete line
#define DCH     'P' // Delect char
#define SU      'S' // Scroll line up
#define SD      'T' // Scroll line down
#define ECH     'X' // Erase char
#define CBT     'Z' // Cursor backward tabulation
#define HPR     'a' // Horizontal position relative
#define REP     'b' // Repeat print
#define DA      'c' // Device attributes
#define VPA     'd' // Vertical position absolute
#define VPR     'e' // Vertical position relative
#define HVP     'f' // Horizontal and vertical position
#define TBC     'g' // Tab clear
#define SM      'h' // Set mode
#define MC      'i' // Media copy
#define RM      'l' // Reset mode
#define SGR     'm' // Select graphic rendition
#define DSR     'n' // Device status report
#define DECLL   'q' // Load LEDs
#define DECSTBM 'r' // Set top and bottom margins
#define DECSC   's' // Save cursor
#define WINMAN  't' // Window manipulation
#define DECRC   'u' // Restore cursor
#define HPA     '`' // Horizontal position absolute

// mode

#define DECCKM         1 // Cursor keys
#define DECANM         2 // Keyboard action mode or for vt52
#define DECCOLM        3 // 80 column mode
#define DECSCLM        4 // Jump (fast) scroll
#define DECSCNM        5 // Normal video mode
#define DECOM          6 // Normal cursor mode
#define DECAWM         7 // No auto-wrap mode
#define DECARM         8 // No auto-repeat mode
#define DECPFF        18 // Do not print form feed
#define DECPEX        19 // Limit print to scrolling region
#define DECTCEM       25 // Hide cursor
#define DECRLM        34 // Cursor direction right to left
#define DECHEBM       35 // Hebrew keyboard mapping
#define DECHEM        36 // Hebrew encoding mode
#define DECNRCM       42 // National replacement charset
#define DECGEPM       43 // Graphic expanded print mode
#define DECGPCM       44 // Graphic print color mode
#define DECGPCS       45 // Graphic print color syntax
#define DECGRPM       47 // Graphic print rotated print mode
#define DECNAKB       57 // Greek keyboard mapping
#define DECHCCM       60 // Horizontal cursor coupling
#define DECVCCM       61 // Vertical cursor coupling
#define DECPCCM       64 // Page cursor coupling
#define DECNKM        66 // Numeric keypad mode
#define DECBKM        67 // Backarrow key sends delete
#define DECKBUM       68 // Keyboard usage
#define DECLRMM       69 // Left and right margin mode
#define DECXRLM       73 // Transmit rate limiting
#define DECSDM        80 // Sixel diaplay mode
#define DECKPM        81 // Key position
#define DECNCSM       95 // No clearing screen on column change
#define DECRLCM       96 // Cursor right to left
#define DECCRTSM      97 // CRT save
#define DECARSM       98 // Auto resize
#define DECMCM        99 // Modem control
#define DECAAM       100 // Auto answer back
#define DECCANSM     101 // Conceal answer back message
#define DECNULM      102 // Ignoring null
#define DECHDPXM     103 // Half duplex
#define DECESKM      104 // Secondary keyboard language
#define DECOSCNM     106 // Overscan
#define MODE_SBC      12 // Start blinking cursor
#define MODE_FFE    1004 // FocusIn/FocusOut events
#define MODE_MPR    1000 // Mouse on button press and release
#define MODE_MP     1002 // Motion on button press
#define MODE_AMM    1003 // All mouse motions
#define MODE_MUTF8  1005 // UTF8 mouse
#define MODE_SMM    1006 // SGR mouse mode
#define MODE_SC     1048 // Save cursor
#define MODE_ALT    1047 // Alternate screen buffer
#define MODE_SC_ALT 1049 // Save cursor and alternate screen buffer
#define MODE_UM     1015 // urxvt mouse
#define MODE_BP     2004 // Bracketed paste
#define MODE_SO     2026 // Synchronized output

// nf esc
#define NF_GZD4 '(' // Charset G0
#define NF_G1D4 ')' // Charset G1
#define NF_G2D4 '*' // Charset G2
#define NF_G3D4 '+' // Charset G3

// fp esc
#define FP_DECPAM '='
#define FP_DECPNM '>'
#define FP_DECSC  '7'
#define FP_DECRC  '8'

#define ESC_IS_NF(c) ((c) >= 0x20 && (c) <= 0x2f)
#define ESC_IS_FP(c) ((c) >= 0x30 && (c) <= 0x3f)
#define ESC_IS_FE(c) ((c) >= 0x40 && (c) <= 0x5f)
#define ESC_IS_FS(c) ((c) >= 0x60 && (c) <= 0x7e)

#define ISCTRLC0(c) ((c) <= 0x1f)
#define ISCTRLC1(c) ((c) >= 0x80 && (c) <= 0x9f)
#define ISCTRL(c)   (ISCTRLC0(c) || ISCTRLC1(c) || (c) == 0x7f)
#define ATOC1(c)    ((c) - 0x40 + 0x80)
#define C1TOA(c)    ((c) - 0x80 + 0x40)

#define VT102 "\033[?6c"

static uint8_t csi_ending[] __unused = { 0x40, 0x7e };
static uint8_t nf_ending[] __unused = { 0x30, 0x7e };
static uint8_t dcs_ending[] __unused = { C1TOA(ST), ESC };
static uint8_t osc_ending[] __unused = { BEL, ST, C1TOA(ST), ESC };

/*
SGR
   0 Reset
   1 Bold
   2 Faint
   3 Italic
   4 Underline
   5 Slow blink
   6 Rapid blink
   7 Reverse videoor invert
   8 Conceal
   9 Crossed out
  10 Primary font
  11-19 Alt font
  20 Fraktur
  21 Double underlined
  22 Normal intensity
  23 Neither italic nor black letter
  24 Not underlined
  25 Not blinking
  26 Proportional spacing
  27 Not reversed
  28 Reveal
  29 Not crossed out
  30 Foreground black
  31 Foreground red
  32 Foreground green
  33 Foreground yellow
  34 Foreground blue
  35 Foreground magenta
  36 Foreground cyan
  37 Foreground white
  38 Foreground RGB
  39 Foreground reset
  40 Background black
  41 Background red
  42 Background green
  43 Background yellow
  44 Background blue
  45 Background magenta
  46 Background cyan
  47 Background white
  48 Background RGB
  49 Background reset
  50 Disable proportional spacing
  51 Framed
  52 Encircled
  53 Overlined
  54 Neither framed nor encircled
  55 Not overlined
  56-57 Unknown
  58 Underline color
  59 Underline color reset
  60 Ideogram underline
  61 Ideogram double underline
  62 Ideogram overline
  63 Ideogram double over line
  64 Ideogram stress marking
  65 No ideogram attributes
  66-72 Unknown
  73 Superscript
  74 Subscript
  75 Neither superscript nor subscript
  76-89 Unknown
  90 Foreground bright black
  91 Foreground bright red
  92 Foreground bright green
  93 Foreground bright yellow
  94 Foreground bright blue
  95 Foreground bright magenta
  96 Foreground bright cyan
  97 Foreground bright white
  98-99 Unknown
 100 Background bright black
 101 Background bright red
 102 Background bright green
 103 Background bright yellow
 104 Background bright blue
 105 Background bright magenta
 106 Background bright cyan
 107 Background bright white
*/

static inline char*
ctrl_name(uint8_t c) {
#define _case(a) case a: return #a;
    switch(c){
    _case(NUL);
    _case(SOH);
    _case(STX);
    _case(ETX);
    _case(EOT);
    _case(ENQ);
    _case(ACK);
    _case(BEL);
    _case(BS);
    _case(HT);
    _case(LF);
    _case(VT);
    _case(FF);
    _case(CR);
    _case(SO);
    _case(SI);
    _case(DLE);
    _case(DC1);
    _case(DC2);
    _case(DC3);
    _case(DC4);
    _case(NAK);
    _case(SYN);
    _case(ETB);
    _case(CAN);
    _case(EM);
    _case(SUB);
    _case(ESC);
    _case(FS);
    _case(GS);
    _case(RS);
    _case(US);
    _case(DEL);
    _case(PAD);
    _case(HOP);
    _case(BPH);
    _case(NBH);
    _case(IND);
    _case(NEL);
    _case(SSA);
    _case(ESA);
    _case(HTS);
    _case(HTJ);
    _case(VTS);
    _case(PLD);
    _case(PLU);
    _case(RI);
    _case(SS2);
    _case(SS3);
    _case(DCS);
    _case(PU1);
    _case(PU2);
    _case(STS);
    _case(CCH);
    _case(MW);
    _case(SPA);
    _case(EPA);
    _case(SOS);
    _case(SGCI);
    _case(SCI);
    _case(CSI);
    _case(ST);
    _case(OSC);
    _case(PM);
    _case(APC);
    }
#undef _case
    return "XXX";
}

#endif
