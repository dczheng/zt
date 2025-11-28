#ifndef __CTRL_H__
#define __CTRL_H__

#include "zt.h"

// C0
#define NUL  0x00
#define SOH  0x01
#define STX  0x02
#define ETX  0x03
#define EOT  0x04
#define ENQ  0x05
#define ACK  0x06
#define BEL  0x07
#define BS   0x08
#define HT   0x09
#define LF   0x0a
#define VT   0x0b
#define FF   0x0c
#define CR   0x0d
#define SO   0x0e
#define SI   0x0f
#define DLE  0x10
#define DC1  0x11
#define DC2  0x12
#define DC3  0x13
#define DC4  0x14
#define NAK  0x15
#define SYN  0x16
#define ETB  0x17
#define CAN  0x18
#define EM   0x19
#define SUB  0x1a
#define ESC  0x1b
#define FS   0x1c
#define GS   0x1d
#define RS   0x1e
#define US   0x1f
#define DEL  0x7f

// C1
#define PAD  0x80
#define HOP  0x81
#define BPH  0x82
#define NBH  0x83
#define IND  0x84
#define NEL  0x85
#define SSA  0x86
#define ESA  0x87
#define HTS  0x88
#define HTJ  0x89
#define VTS  0x8a
#define PLD  0x8b
#define PLU  0x8c
#define RI   0x8d
#define SS2  0x8e
#define SS3  0x8f
#define DCS  0x90
#define PU1  0x91
#define PU2  0x92
#define STS  0x93
#define CCH  0x94
#define MW   0x95
#define SPA  0x96
#define EPA  0x97
#define SOS  0x98
#define SGCI 0x99
#define SCI  0x9a
#define CSI  0x9b
#define ST   0x9c
#define OSC  0x9d
#define PM   0x9e
#define APC  0x9f

// CSI
#define ICH      '@'
#define CUU      'A'
#define CUD      'B'
#define CUF      'C'
#define CUB      'D'
#define CNL      'E'
#define CPL      'F'
#define CHA      'G'
#define CUP      'H'
#define CHT      'I'
#define ED       'J'
#define EL       'K'
#define IL       'L'
#define DL       'M'
#define DCH      'P'
#define SU       'S'
#define SD       'T'
#define ECH      'X'
#define CBT      'Z'
#define HPR      'a'
#define REP      'b'
#define DA       'c'
#define VPA      'd'
#define VPR      'e'
#define HVP      'f'
#define TBC      'g'
#define SM       'h'
#define MC       'i'
#define RM       'l'
#define SGR      'm'
#define DSR      'n'
#define DECLL    'q'
#define DECSTBM  'r'
#define DECSC    's'
#define DECRC    'u'
#define WINMAN   't'
#define HPA      '`'

// mode
#define DECCKM         1
#define DECANM         2
#define DECCOLM        3
#define DECSCLM        4
#define DECSCNM        5
#define DECOM          6
#define DECAWM         7
#define DECARM         8
#define DECPFF        18
#define DECPEX        19
#define DECTCEM       25
#define DECRLM        34
#define DECHEBM       35
#define DECHEM        36
#define DECNRCM       42
#define DECNAKB       57
#define DECHCCM       60
#define DECVCCM       61
#define DECPCCM       64
#define DECNKM        66
#define DECBKM        67
#define DECKBUM       68
#define DECVSSM       69
#define DECLRMM       69
#define DECXRLM       73
#define DECKPM        81
#define DECNCSM       95
#define DECRLCM       96
#define DECCRTSM      97
#define DECARSM       98
#define DECMCM        99
#define DECAAM       100
#define DECCANSM     101
#define DECNULM      102
#define DECHDPXM     103
#define DECESKM      104
#define DECOSCNM     106
#define M_SBC         12
#define M_MP        1000
#define M_MMP       1002
#define M_MMA       1003
#define M_SF        1004
#define M_MUTF8     1005
#define M_BP        2004
#define M_ALTS      1047
#define M_SC        1048
#define M_SC_ALTS   1049
#define M_ME        1006

// nf esc
#define NF_GZD4        '('
#define NF_G1D4        ')'
#define NF_G2D4        '*'
#define NF_G3D4        '+'

// fp esc
#define FP_DECPAM      '='
#define FP_DECPNM      '>'
#define FP_DECSC       '7'
#define FP_DECRC       '8'

// esc type
#define ESCNF 0
#define ESCFP 1
#define ESCFE 2
#define ESCFS 3

#define ESCNF_MIN   0x20
#define ESCNF_MAX   0x2f
#define ESCFP_MIN   0x30
#define ESCFP_MAX   0x3f
#define ESCFE_MIN   0x40
#define ESCFE_MAX   0x5f
#define ESCFS_MIN   0x60
#define ESCFS_MAX   0x7e

#define ESCNF_END_MIN  0x30
#define ESCNF_END_MAX  0x7e
#define CSI_MIN 0x40
#define CSI_MAX 0x7e

#define ISCTRLC0(c) ((c) <= 0x1f || (c) == 0x7f)
#define ISCTRLC1(c) ((c) >= 0x80 && (c) <= 0x9f)
#define ISCTRL(c)   (ISCTRLC0(c) || ISCTRLC1(c))
#define ATOC1(c)    ((c) - 0x40 + 0x80)
#define C1TOA(c)    ((c) - 0x80 + 0x40)

static uint8_t osc_end_codes[] __unused = {
    BEL, ST, C1TOA(ST), ESC
};

static uint8_t dcs_end_codes[] __unused = {
    C1TOA(ST), ESC
};

struct ctrl_desc_t {
    int value;
    char *name, *desc;
};

#define _ADD(value, desc) {value,  #value, desc}
static struct ctrl_desc_t fp_esc_desc_table[] __unused = {
    _ADD(FP_DECPAM, "ApplicationKeypad"),
    _ADD(FP_DECPNM, "NormalKeypad"),
    _ADD(FP_DECSC , "SaveCursor"),
    _ADD(FP_DECRC , "RestoreCursor"),
};

static struct ctrl_desc_t nf_esc_desc_table[] __unused = {
    _ADD(NF_GZD4, "CharsetG0"),
    _ADD(NF_G1D4, "CharsetG1"),
    _ADD(NF_G2D4, "CharsetG2"),
    _ADD(NF_G3D4, "CharsetG3"),
};

static struct ctrl_desc_t mode_desc_table[] __unused = {
    _ADD(DECCKM   , "Cursorkeys"),
    _ADD(DECANM   , "ANSI"),
    _ADD(DECCOLM  , "Column"),
    _ADD(DECSCLM  , "Scrolling"),
    _ADD(DECSCNM  , "Screen"),
    _ADD(DECOM    , "Origin"),
    _ADD(DECAWM   , "Autowrap"),
    _ADD(DECARM   , "Autorepeat"),
    _ADD(DECPFF   , "PrintFormFeed"),
    _ADD(DECPEX   , "PrinterExtent"),
    _ADD(DECTCEM  , "TextCursorEnable"),
    _ADD(DECRLM   , "CursorDirectionRightToLeft"),
    _ADD(DECHEBM  , "HebrewKeyboardMapping"),
    _ADD(DECHEM   , "HebrewEncodingMode"),
    _ADD(DECNRCM  , "NationalReplacementCharacterSet"),
    _ADD(DECNAKB  , "GreekKeyboardMapping"),
    _ADD(DECHCCM  , "HorizontalCursorCoupling"),
    _ADD(DECVCCM  , "VerticalCursorCoupling"),
    _ADD(DECPCCM  , "PageCursorCoupling"),
    _ADD(DECNKM   , "NumericKeypad"),
    _ADD(DECBKM   , "BackarrowKey"),
    _ADD(DECKBUM  , "KeyboardUsage"),
    _ADD(DECVSSM  , "VerticalSplitScreen"),
    _ADD(DECLRMM  , "VerticalSplitScreen"),
    _ADD(DECXRLM  , "TransmitRateLimiting"),
    _ADD(DECKPM   , "KeyPosition"),
    _ADD(DECNCSM  , "NoClearingScreenOnColumnChange"),
    _ADD(DECRLCM  , "CursorRightToLeft"),
    _ADD(DECCRTSM , "CRTSave"),
    _ADD(DECARSM  , "AutoResize"),
    _ADD(DECMCM   , "ModemControl"),
    _ADD(DECAAM   , "AutoAnswerBack"),
    _ADD(DECCANSM , "ConcealAnswerbackMessage"),
    _ADD(DECNULM  , "IgnoringNull"),
    _ADD(DECHDPXM , "HalfDuplex"),
    _ADD(DECESKM  , "SecondaryKeyboardLanguage"),
    _ADD(DECOSCNM , "Overscan"),
    _ADD(M_SF     , "SendFocusEventsToTTY"),
    _ADD(M_BP     , "BracketedPaste"),
    _ADD(M_SBC    , "StartBlinkingCursor"),
    _ADD(M_MUTF8  , "UTF8Mouse"),
    _ADD(M_MP     , "ReportButtonPress"),
    _ADD(M_MMP    , "ReportMotionOnButtonPress"),
    _ADD(M_MMA    , "EnalbeAllMouseMotions"),
    _ADD(M_ME     , "ExtenedReporting"),
    _ADD(M_ALTS   , "UseAlternateScreenBuffer"),
    _ADD(M_SC     , "SaveCursor"),
    _ADD(M_SC_ALTS, "SaveCursorAlt"),
};

static struct ctrl_desc_t ctrl_desc_table[] __unused = {
    _ADD(NUL , "Null"),
    _ADD(SOH , "StartOfHeading"),
    _ADD(STX , "StartOfText"),
    _ADD(ETX , "EndOfText"),
    _ADD(EOT , "EndOfTransmission"),
    _ADD(ENQ , "Enquiry"),
    _ADD(ACK , "Acknowledge"),
    _ADD(BEL , "Bell"),
    _ADD(BS  , "Backspace"),
    _ADD(HT  , "HorizontalTabulation"),
    _ADD(LF  , "LineFeed"),
    _ADD(VT  , "VerticalTabulation"),
    _ADD(FF  , "FormFeed"),
    _ADD(CR  , "CarriageReturn"),
    _ADD(SO  , "ShiftOut"),
    _ADD(SI  , "ShiftIn"),
    _ADD(DLE , "DataLinkEscape"),
    _ADD(DC1 , "DeviceControlOne"),
    _ADD(DC2 , "DeviceControlTwo"),
    _ADD(DC3 , "DeviceControlThree"),
    _ADD(DC4 , "DeviceControlFour"),
    _ADD(NAK , "NegativeAcknowledge"),
    _ADD(SYN , "SynchronousIdle"),
    _ADD(ETB , "EndOfTransmissionBlock"),
    _ADD(CAN , "Cancel"),
    _ADD(EM  , "EndOfmedium"),
    _ADD(SUB , "Substitute"),
    _ADD(ESC , "Escape"),
    _ADD(FS  , "FileSeparator"),
    _ADD(GS  , "GroupSeparator"),
    _ADD(RS  , "RecordSeparator"),
    _ADD(US  , "UnitSeparator"),
    _ADD(DEL , "Delete"),
    _ADD(PAD , "PaddingCharacter"),
    _ADD(HOP , "HighOctetPreset"),
    _ADD(BPH , "BreakPermittedHere"),
    _ADD(NBH , "NoBreakHere"),
    _ADD(IND , "Index"),
    _ADD(NEL , "NextLine"),
    _ADD(SSA , "StartOfSelectedArea"),
    _ADD(ESA , "EndOfSelectedArea"),
    _ADD(HTS , "HorizontalTabulationSet"),
    _ADD(HTJ , "HorizontalTabulationWithJustification"),
    _ADD(VTS , "VerticalTabulationSet"),
    _ADD(PLD , "PartialLineDown"),
    _ADD(PLU , "PartialLineUp"),
    _ADD(RI  , "ReverseIndex"),
    _ADD(SS2 , "SingleShift2"),
    _ADD(SS3 , "SingleShift3"),
    _ADD(DCS , "DeviceControlString"),
    _ADD(PU1 , "PrivateUse1"),
    _ADD(PU2 , "PrivateUse2"),
    _ADD(STS , "TransmitState"),
    _ADD(CCH , "CancelCharacter"),
    _ADD(MW  , "MessageWaiting"),
    _ADD(SPA , "StartOfProtectedArea"),
    _ADD(EPA , "EndOfProtectedArea"),
    _ADD(SOS , "StartOfString"),
    _ADD(SGCI, "SingleGraphicCharacterIntroducer"),
    _ADD(SCI , "SingleCharacterIntroducer"),
    _ADD(CSI , "ControlSequenceIntroducer"),
    _ADD(ST  , "StringTerminator"),
    _ADD(OSC , "OperatingSystemCommand"),
    _ADD(PM  , "PrivacyMessage"),
    _ADD(APC , "ApplicationProgramCommand"),
};

static struct ctrl_desc_t csi_desc_table[] __unused = {
    _ADD(ICH    , "InsertBlankChar"),
    _ADD(CUU    , "CursorUp"),
    _ADD(CUD    , "CursorDown"),
    _ADD(CUF    , "CursorForward"),
    _ADD(CUB    , "CursorBackward"),
    _ADD(CNL    , "CursorNextLine"),
    _ADD(CPL    , "CursorPreviousLine"),
    _ADD(CHA    , "CursorHorizontalAbsolute"),
    _ADD(CUP    , "CursorPosition"),
    _ADD(CHT    , "CursorForwardTabulation"),
    _ADD(ED     , "EraseInDisplay"),
    _ADD(EL     , "EraseInLine"),
    _ADD(IL     , "InsertLine"),
    _ADD(DL     , "DeleteLine"),
    _ADD(DCH    , "DelectChar"),
    _ADD(SU     , "ScrollLineUp"),
    _ADD(SD     , "ScrollLineDown"),
    _ADD(ECH    , "EraseChar"),
    _ADD(CBT    , "CursorBackwardTabulation"),
    _ADD(REP    , "RepeatPrint"),
    _ADD(DA     , "DeviceAttributes"),
    _ADD(HVP    , "HorizontalAndVerticalPosition"),
    _ADD(TBC    , "TabClear"),
    _ADD(SM     , "SetMode"),
    _ADD(MC     , "MediaCopy"),
    _ADD(RM     , "ResetMode"),
    _ADD(SGR    , "SelectGraphicRendition"),
    _ADD(DSR    , "DeviceStatusReport"),
    _ADD(DECLL  , "LoadLEDs"),
    _ADD(DECSTBM, "SetTopAndBottomMargins"),
    _ADD(DECSC  , "SaveCursor"),
    _ADD(DECRC  , "RestoreCursor"),
    _ADD(VPA    , "VerticalLinePositionAbsolute"),
    _ADD(VPR    , "VerticalPositionRelative"),
    _ADD(HPA    , "HorizontalPositionAbsolute"),
    _ADD(HPR    , "HorizontalPositionRelative"),
    _ADD(WINMAN , "WindowManipulation"),
};

static struct ctrl_desc_t sgr_desc_table[] __unused = {
    _ADD(0  , "Reset"),
    _ADD(1  , "Bold"),
    _ADD(2  , "Faint"),
    _ADD(3  , "Italic"),
    _ADD(4  , "Underline"),
    _ADD(5  , "SlowBlink"),
    _ADD(6  , "RapidBlink"),
    _ADD(7  , "ReverseVideoorInvert"),
    _ADD(8  , "Conceal"),
    _ADD(9  , "CrossedOut"),
    _ADD(10 , "PrimaryFont"),
    _ADD(11 , "AltFont1"),
    _ADD(12 , "AltFont2"),
    _ADD(13 , "AltFont3"),
    _ADD(14 , "AltFont4"),
    _ADD(15 , "AltFont5"),
    _ADD(16 , "AltFont6"),
    _ADD(17 , "AltFont7"),
    _ADD(18 , "AltFont8"),
    _ADD(19 , "AltFont9"),
    _ADD(20 , "Fraktur"),
    _ADD(21 , "DoubleUnderlined"),
    _ADD(22 , "NormalIntensity"),
    _ADD(23 , "NeitherItalicNorBlackletter"),
    _ADD(24 , "NotUnderlined"),
    _ADD(25 , "NotBlinking"),
    _ADD(26 , "ProportionalSpacing"),
    _ADD(27 , "NotReversed"),
    _ADD(28 , "Reveal"),
    _ADD(29 , "NotCrossedOut"),
    _ADD(30 , "FgBlack"),
    _ADD(31 , "FgRed"),
    _ADD(32 , "FgGreen"),
    _ADD(33 , "FgYellow"),
    _ADD(34 , "FgBlue"),
    _ADD(35 , "FgMagenta"),
    _ADD(36 , "FgCyan"),
    _ADD(37 , "FgWhite"),
    _ADD(38 , "FgRGB"),
    _ADD(39 , "FgReset"),
    _ADD(40 , "BgBlack"),
    _ADD(41 , "BgRed"),
    _ADD(42 , "BgGreen"),
    _ADD(43 , "BgYellow"),
    _ADD(44 , "BgBlue"),
    _ADD(45 , "BgMagenta"),
    _ADD(46 , "BgCyan"),
    _ADD(47 , "BgWhite"),
    _ADD(48 , "BgRGB"),
    _ADD(49 , "BgReset"),
    _ADD(50 , "DisableProportionalSpacing"),
    _ADD(51 , "Framed"),
    _ADD(52 , "Encircled"),
    _ADD(53 , "Overlined"),
    _ADD(54 , "NeitherFramedNorEncircled"),
    _ADD(55 , "NotOverlined"),
    _ADD(56 , "Unknown"),
    _ADD(57 , "Unknown"),
    _ADD(58 , "UnderlineColor"),
    _ADD(59 , "UnderlineColorReset"),
    _ADD(60 , "IdeogramUnderline"),
    _ADD(61 , "IdeogramDoubleUnderline"),
    _ADD(62 , "IdeogramOverline"),
    _ADD(63 , "IdeogramDoubleOverline"),
    _ADD(64 , "IdeogramStressMarking"),
    _ADD(65 , "NoIdeogramAttributes"),
    _ADD(66 , "Unknown"),
    _ADD(67 , "Unknown"),
    _ADD(68 , "Unknown"),
    _ADD(69 , "Unknown"),
    _ADD(70 , "Unknown"),
    _ADD(71 , "Unknown"),
    _ADD(72 , "Unknown"),
    _ADD(73 , "Superscript"),
    _ADD(74 , "Subscript"),
    _ADD(75 , "NeitherSuperscriptNorSubscript"),
    _ADD(76 , "Unknown"),
    _ADD(77 , "Unknown"),
    _ADD(78 , "Unknown"),
    _ADD(79 , "Unknown"),
    _ADD(80 , "Unknown"),
    _ADD(81 , "Unknown"),
    _ADD(82 , "Unknown"),
    _ADD(83 , "Unknown"),
    _ADD(84 , "Unknown"),
    _ADD(85 , "Unknown"),
    _ADD(86 , "Unknown"),
    _ADD(87 , "Unknown"),
    _ADD(88 , "Unknown"),
    _ADD(89 , "Unknown"),
    _ADD(90 , "FgBrightBlack"),
    _ADD(91 , "FgBrightRed"),
    _ADD(92 , "FgBrightGreen"),
    _ADD(93 , "FgBrightYellow"),
    _ADD(94 , "FgBrightBlue"),
    _ADD(95 , "FgBrightMagenta"),
    _ADD(96 , "FgBrightCyan"),
    _ADD(97 , "FgBrightWhite"),
    _ADD(98 , "Unknown"),
    _ADD(99 , "Unknown"),
    _ADD(100, "BgBrightBlack"),
    _ADD(101, "BgBrightRed"),
    _ADD(102, "BgBrightGreen"),
    _ADD(103, "BgBrightYellow"),
    _ADD(104, "BgBrightBlue"),
    _ADD(105, "BgBrightMagenta"),
    _ADD(106, "BgBrightCyan"),
    _ADD(107, "BgBrightWhite"),
};
#undef _ADD

#define _ADD(value, desc) {value,  #value + 3, desc}
static struct ctrl_desc_t esc_desc_table[] __unused = {
    _ADD(ESCNF, "nF Esc"),
    _ADD(ESCFP, "Fp Esc"),
    _ADD(ESCFE, "Fe Esc"),
    _ADD(ESCFS, "Fs Esc"),
};
#undef _ADD

static inline int
_ctrl_desc(struct ctrl_desc_t *table, int len,
    struct ctrl_desc_t *item, int value) {
    ZERO(*item);
    for (int i = 0; i < len; i++)
        if (table[i].value == value) {
            *item = table[i];
            return 0;
        }
    return ENOENT;
}

#define _CTRL_DESC(table, item, value) \
    _ctrl_desc(table, LEN(table), item, value)

#define fp_esc_desc(desc, value) _CTRL_DESC(fp_esc_desc_table, desc, value)
#define nf_esc_desc(desc, value) _CTRL_DESC(nf_esc_desc_table, desc, value)
#define mode_desc(desc, value)   _CTRL_DESC(mode_desc_table, desc, value)
#define ctrl_desc(desc, value)   _CTRL_DESC(ctrl_desc_table, desc, value)
#define csi_desc(desc, value)    _CTRL_DESC(csi_desc_table, desc, value)
#define sgr_desc(desc, value)    _CTRL_DESC(sgr_desc_table, desc, value)
#define esc_desc(desc, value)    _CTRL_DESC(esc_desc_table, desc, value)

static inline int
esc_type(uint8_t c) {
    if (c >= ESCNF_MIN && c <= ESCNF_MAX)
        return ESCNF;
    if (c >= ESCFP_MIN && c <= ESCFP_MAX)
        return ESCFP;
    if (c >= ESCFE_MIN && c <= ESCFE_MAX)
        return ESCFE;
    if (c >= ESCFS_MIN && c <= ESCFS_MAX)
        return ESCFS;
    return -1;
}

#endif
