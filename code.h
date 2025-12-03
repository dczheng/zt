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

#define VT102 "\033[?6c"
#define PRIMARY_DA VT102

// C0
#define NUL 0x00 // ^@ Null
#define SOH 0x01 // ^A Start of Heading
#define STX 0x02 // ^B Start of text
#define ETX 0x03 // ^C End of text
#define EOT 0x04 // ^D End of transmission
#define ENQ 0x05 // ^E Enquiry
#define ACK 0x06 // ^F Acknowledge
#define BEL 0x07 // ^G Bell
#define BS  0x08 // ^H Backspace
#define HT  0x09 // ^I Horizontal tabulation
#define LF  0x0a // ^J Line feed
#define VT  0x0b // ^K Vertical tabulation
#define FF  0x0c // ^L Form feed
#define CR  0x0d // ^M Carriage return
#define SO  0x0e // ^N Shift out
#define SI  0x0f // ^O Shift in
#define DLE 0x10 // ^P DataLink escape
#define DC1 0x11 // ^Q DeviceControl 1
#define DC2 0x12 // ^R DeviceControl 2
#define DC3 0x13 // ^S DeviceControl 3
#define DC4 0x14 // ^T DeviceControl 4
#define NAK 0x15 // ^U Negative acknowledge
#define SYN 0x16 // ^V Synchronous idle
#define ETB 0x17 // ^W End of transmission block
#define CAN 0x18 // ^X Cancel
#define EM  0x19 // ^Y End of medium
#define SUB 0x1a // ^Z Substitute
#define ESC 0x1b // ^[ Escape
#define FS  0x1c // ^\ File separator
#define GS  0x1d // ^] Group separator
#define RS  0x1e // ^^ Record separator
#define US  0x1f // ^_ Unit separator
#define ISCTRL0(c) ((c) <= 0x1f)

// other
#define DEL 0x7f // ^? Delete

// C1, Fe ESC
#define PAD  0x80 // '@' Padding character
#define HOP  0x81 // 'A' High octet preset
#define BPH  0x82 // 'B' Break permitted here
#define NBH  0x83 // 'C' No break here
#define IND  0x84 // 'D' Index
#define NEL  0x85 // 'E' Next line
#define SSA  0x86 // 'F' Start of selected area
#define ESA  0x87 // 'G' End of selected area
#define HTS  0x88 // 'H' Horizontal tabulationSet
#define HTJ  0x89 // 'I' Horizontal tabulation with justification
#define VTS  0x8a // 'J' Vertical tabulationSet
#define PLD  0x8b // 'K' Partial line down
#define PLU  0x8c // 'L' Partial line up
#define RI   0x8d // 'M' Reverse index
#define SS2  0x8e // 'N' Single shift 2
#define SS3  0x8f // 'O' Single shift 3
#define DCS  0x90 // 'P' Device control string
#define PU1  0x91 // 'Q' Private use 1
#define PU2  0x92 // 'R' Private use 2
#define STS  0x93 // 'S' Transmit state
#define CCH  0x94 // 'T' Cancel character
#define MW   0x95 // 'U' Message waiting
#define SPA  0x96 // 'V' Start of protected area
#define EPA  0x97 // 'W' End of protected area
#define SOS  0x98 // 'X' Start of string
#define SGC  0x99 // 'Y' Single graphic character introducer
#define SCI  0x9a // 'Z' Single character introducer
#define CSI  0x9b // '[' Control sequence introducer
#define ST   0x9c // '\' String terminator
#define OSC  0x9d // ']' Operating system command
#define PM   0x9e // '^' Privacy message
#define APC  0x9f // '_' Application program command
#define ISCTRL1(c) ((c) >= 0x80 && (c) <= 0x9f)

#define ISCTRL(c)   (ISCTRL0(c) || ISCTRL1(c) || (c) == DEL)
#define FETOC1(c)   ((c) - '@' + 0x80)
#define C1TOFE(c)   ((c) - 0x80 + '@')

// nF ESC
#define NF_GZD4 '(' // Charset G0
#define NF_G1D4 ')' // Charset G1
#define NF_G2D4 '*' // Charset G2
#define NF_G3D4 '+' // Charset G3

// Fp ESC
#define FP_DECSC  '7' // DEC save cursor
#define FP_DECRC  '8' // DEC restore cursor

#define ESC_IS_NF(c) ((c) >= 0x20 && (c) <= 0x2f)
#define ESC_IS_FP(c) ((c) >= 0x30 && (c) <= 0x3f)
#define ESC_IS_FE(c) ((c) >= 0x40 && (c) <= 0x5f)
#define ESC_IS_FS(c) ((c) >= 0x60 && (c) <= 0x7e)

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

// DEC private modes
// https://vt100.net/emu/dec_private_modes.html
#define DECCKM       1 // Cursor Keys Mode
#define DECANM       2 // ANSI/VT52 Mode
#define DECCOLM      3 // Column
#define DECSCLM      4 // Scrolling
#define DECSCNM      5 // Screen Mode (light or dark screen)
#define DECOM        6 // Origin Mode
#define DECAWM       7 // Auto Wrap Mode
#define DECARM       8 // Auto Repeat Mode
#define DECINLM      9 // Interlace Mode
#define DECEDM      10 // Editing Mode
#define DECLTM      11 // Line Transmit Mode
#define DECKANAM    12 // Katakana Shift Mode
#define DECSCFDM    13 // Space Compression/Field Delimiter Mode
#define DECTEM      14 // Transmit Execution Mode
#define DECEKEM     16 // Edit Key Execution Mode
#define DECPFF      18 // Print Form Feed
#define DECPEX      19 // Printer Extent
#define OV1         20 // Overstrike
#define BA1         21 // Local BASIC
#define BA2         22 // Host BASIC
#define PK1         23 // Programmable Keypad
#define AH1         24 // Auto Hardcopy
#define DECTCEM     25 // Text Cursor Enable Mode
#define DECPSP      27 // Proportional Spacing
#define DECPSM      29 // Pitch Select Mode
#define DECRLM      34 // Cursor Right to Left Mode
#define DECHEBM     35 // Hebrew (Keyboard) Mode
#define DECHEM      36 // Hebrew Encoding Mode
#define DECTEK      38 // Tektronix 4010/4014 Mode
#define DECCRNLM    40 // Carriage Return/New Line Mode
#define DECUPM      41 // Unidirectional Print Mode
#define DECNRCM     42 // National Replacement Character Set Mode
#define DECGEPM     43 // Graphics Expanded Print Mode
#define DECGPCM     44 // Graphics Print Color Mode
#define DECGPCS     45 // Graphics Print Color Syntax
#define DECGPBM     46 // Graphics Print Background Mode
#define DECGRPM     47 // Graphics Rotated Print Mode
#define DECTHAIM    49 // Thai Input Mode
#define DECTHAICM   50 // Thai Cursor Mode
#define DECBWRM     51 // Black/White Reversal Mode
#define DECOPM      52 // Origin Placement Mode
#define DEC131TM    53 // VT131 Transmit Mode
#define DECBPM      55 // Bold Page Mode
#define DECNAKB     57 // Greek/N-A Keyboard Mapping Mode
#define DECIPEM     58 // Enter IBM Proprinter Emulation Mode
#define DECKKDM     59 // Kanji/Katakana Display Mode
#define DECHCCM     60 // Horizontal Cursor Coupling
#define DECVCCM     61 // Vertical Cursor Coupling Mode
#define DECPCCM     64 // Page Cursor Coupling Mode
#define DECBCMM     65 // Business Color Matching Mode
#define DECNKM      66 // Numeric Keypad Mode
#define DECBKM      67 // Backarrow Key Mode
#define DECKBUM     68 // Keyboard Usage Mode
#define DECVSSM     69 // Vertical Split Screen Mode
#define DECLRMM     69 // Left Right Margin Mode
#define DECFPM      70 // Force Plot Mode
#define DECXRLM     73 // Transmission Rate Limiting
#define DECSDM      80 // Sixel Display Mode
#define DECKPM      81 // Key Position Mode
#define DECTHAISCM  90 // Thai Space Compensating Mode
#define DECNCSM     95 // No Clearing Screen on Column Change Mode
#define DECRLCM     96 // Right to Left Copy Mode
#define DECCRTSM    97 // CRT Save Mode
#define DECARSM     98 // Auto Resize Mode
#define DECMCM      99 // Modem Control Mode
#define DECAAM     100 // Auto Answerback Mode
#define DECCANSM   101 // Conceal Answerback Message Mode
#define DECNULM    102 // Ignore Null Mode
#define DECHDPXM   103 // Half Duplex Mode
#define DECESKM    104 // Secondary Keyboard Language Mode
#define DECOSCNM   106 // Overscan Mode
#define DECNUMLK   108 // NumLock Mode
#define DECCAPSLK  109 // Caps Lock Mode
#define DECKLHIM   110 //  Keyboard LEDs Host Indicator Mode
#define DECFWM     111 //  Framed Windows Mode
#define DECRPL     112 //  Review Previous Lines Mode
#define DECHWUM    113 //  Host Wake-Up Mode
#define DECATCUM   114 //  Alternate Text Color Underline Mode
#define DECATCBM   115 //  Alternate Text Color Blink Mode
#define DECBBSM    116 //  Bold and Blink Style Mode
#define DECECM     117 //  Erase Color Mode

static uint8_t csi_ending[] __unused = { 0x40, 0x7e };
static uint8_t nf_ending[] __unused = { 0x30, 0x7e };
static uint8_t dcs_ending[] __unused = { C1TOFE(ST), ESC };
static uint8_t osc_ending[] __unused = { BEL, ST, C1TOFE(ST), ESC };

/*
 SGR
      0 Reset
      1 Bold
      2 Faint
      3 Italic
      4 Underline
      5 Slow blink
      6 Rapid blink
      7 Reverse video or invert
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
  30-37 Foreground color
     38 Foreground RGB
     39 Foreground reset
  40-47 Background color
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
  90-97 Bright foreground color
  98-99 Unknown
100-107 Bright background color
*/

static inline char*
ctrl_str(void *buf, int n) {
    int i, p = 0;
    static char s[BUFSIZ*3+1];
    uint8_t *b = buf;

    for (i = 0; i < n; i++) {
        if (isprint(b[i])) {
            p += snprintf(s+p, sizeof(s)-p, "%c", b[i]);
            continue;
        }
        if (!ISCTRL(b[i])) {
            p += snprintf(s+p, sizeof(s)-p, "%02x", b[i]);
            continue;
        }
#define _case(a) case a: p += snprintf(s+p, sizeof(s)-p, "%s", #a); break;
        switch(b[i]){
        _case(NUL); _case(SOH); _case(STX); _case(ETX); _case(EOT); _case(ENQ);
        _case(ACK); _case(BEL); _case(BS);  _case(HT);  _case(LF);  _case(VT);
        _case(FF);  _case(CR);  _case(SO);  _case(SI);  _case(DLE); _case(DC1);
        _case(DC2); _case(DC3); _case(DC4); _case(NAK); _case(SYN); _case(ETB);
        _case(CAN); _case(EM);  _case(SUB); _case(ESC); _case(FS);  _case(GS);
        _case(RS);  _case(US);  _case(DEL); _case(PAD); _case(HOP); _case(BPH);
        _case(NBH); _case(IND); _case(NEL); _case(SSA); _case(ESA); _case(HTS);
        _case(HTJ); _case(VTS); _case(PLD); _case(PLU); _case(RI);  _case(SS2);
        _case(SS3); _case(DCS); _case(PU1); _case(PU2); _case(STS); _case(CCH);
        _case(MW);  _case(SPA); _case(EPA); _case(SOS); _case(SGC); _case(SCI);
        _case(CSI); _case(ST);  _case(OSC); _case(PM);  _case(APC);
        }
#undef _case
    }
    s[p] = 0;
    return s;
}

// Table 5-13: https://vt100.net/docs/vt102-ug/chapter5.html#T5-13
// https://en.wikipedia.org/wiki/DEC_Special_Graphics
// https://en.wikipedia.org/wiki/ISO/IEC_2022
// 0x5f - 0x7e
#define GZD4_MIN 0x5f
#define GZD4_MAX 0x7e
static char *gzd4[] __unused = {
                                       " ", // 0x5f
    "◆", "▒", "␉", "␌", "␍", "␊", "°", "±", // 0x60 - 0x67
    "␤", "␋", "┘", "┐", "┌", "└", "┼", "⎺", // 0x68 - 0x6f
    "⎻", "─", "⎼", "⎽", "├", "┤", "┴", "┬", // 0x70 - 0x77
    "│", "≤", "≥", "π", "≠", "£", "·",      // 0x78 - 0x7e
};

#endif
