#ifndef __CTRL_H__
#define __CTRL_H__

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
#define LF   0x0A
#define VT   0x0B
#define FF   0x0C
#define CR   0x0D
#define SO   0x0E
#define SI   0x0F
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
#define SUB  0x1A
#define ESC  0x1B
#define FS   0x1C
#define GS   0x1D
#define RS   0x1E
#define US   0x1F
#define DEL  0x7F

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
#define VTS  0x8A
#define PLD  0x8B
#define PLU  0x8C
#define RI   0x8D
#define SS2  0x8E
#define SS3  0x8F
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
#define SCI  0x9A
#define CSI  0x9B
#define ST   0x9C
#define OSC  0x9D
#define PM   0x9E
#define APC  0x9F

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

// esc type
#define ESCNF 0
#define ESCFP 1
#define ESCFE 2
#define ESCFS 3

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

#define ISCTRLC0(c)     ((c) <= 0x1F || (c) == 0x7F)
#define ISCTRLC1(c)     ((c) >= 0x80 && (c) <= 0x9F)
#define ISCTRL(c)       (ISCTRLC0(c) || ISCTRLC1(c))
#define C1ALT(c)        ((c) & 0xEF)

#define ESCERR            1000
#define ESCNOEND          1001
#define ESCNFNOEND        1002
#define ESCCSINOEND       1003
#define ESCOSCNOEND       1004
#define ESCDCSNOEND       1005

struct CtrlInfo {
    char *name, *desc;
};

struct Esc {
    int len;
    unsigned char *seq, type, esc, csi;
};

int esc_parse(unsigned char*, int, struct Esc*);
void esc_reset(struct Esc*);
int str_to_dec(char*, int*);
char* get_esc_str(struct Esc*, int);
int get_par_num(struct Esc*);
int get_int_par(struct Esc*, int, int*, int);
int get_str_par(struct Esc*, int, char**);
int find_osc_end(unsigned char*, int, int*);
int get_fp_esc_info(int, struct CtrlInfo**);
int get_nf_esc_info(int, struct CtrlInfo**);
int get_mode_info(int, struct CtrlInfo**);
int get_ctrl_info(unsigned char, struct CtrlInfo**);
int get_csi_info(unsigned char, struct CtrlInfo**);
int get_sgr_info(unsigned char, struct CtrlInfo**);
int get_esc_info(unsigned char, struct CtrlInfo**);

#endif
