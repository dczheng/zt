#ifndef __CTRL_H__
#define __CTRL_H__

#include "util.h"

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
#define HPA      '`'

#define ESCNF 0
#define ESCFP 1
#define ESCFE 2
#define ESCFS 3

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
#define M_RBP       1000
#define M_RMBP      1002
#define M_EAMM      1003
#define M_SF        1004
#define M_MUTF8     1005
#define M_BP        2004
#define M_SS        1049
#define M_ER        1006 

#define ISCTRLC0(c)     ((c) <= 0x1F || (c) == 0x7F)
#define ISCTRLC1(c)     ((c) >= 0x80 && (c) <= 0x9F)
#define ISCTRL(c)       (ISCTRLC0(c) || ISCTRLC1(c))

struct CtrlInfo {
    char *name, *desc;
};

struct Esc {
    int len;
    unsigned char *seq, type, fe, csi;
};

int esc_parse(unsigned char*, int, struct Esc*);
void esc_reset(struct Esc*);
int str_to_dec(char*, int*);
char* get_esc_str(struct Esc*, int);
int csi_par_num(struct Esc*);
int csi_int_par(struct Esc*, int, int*, int);
int csi_str_par(struct Esc*, int, char**);

static inline int 
get_mode_info(int type, struct CtrlInfo **info) {
    static struct {
        int type;
        struct CtrlInfo info;
    } infos[] = {
        {DECCKM   ,  {"DECCKM"     , "Cursor keys"                          }},
        {DECANM   ,  {"DECANM"     , "ANSI"                                 }},
        {DECCOLM  ,  {"DECCOLM"    , "Column"                               }},
        {DECSCLM  ,  {"DECSCLM"    , "Scrolling"                            }},
        {DECSCNM  ,  {"DECSCNM"    , "Screen"                               }},
        {DECOM    ,  {"DECOM"      , "Origin"                               }},
        {DECAWM   ,  {"DECAWM"     , "Autowrap"                             }},
        {DECARM   ,  {"DECARM"     , "Autorepeat"                           }},
        {DECPFF   ,  {"DECPFF"     , "Print form Feed"                      }},
        {DECPEX   ,  {"DECPEX"     , "Printer Extent"                       }},
        {DECTCEM  ,  {"DECTCEM"    , "Text Cursor Enable"                   }},
        {DECRLM   ,  {"DECRLM"     , "Cursor Direction, Right to Left"      }},
        {DECHEBM  ,  {"DECHEBM"    , "Hebrew Keyboard Mapping"              }},
        {DECHEM   ,  {"DECHEM"     , "Hebrew Encoding Mode"                 }},
        {DECNRCM  ,  {"DECNRCM"    , "National Replacement Character Set"   }},
        {DECNAKB  ,  {"DECNAKB"    , "Greek keyboard Mapping"               }},
        {DECHCCM  ,  {"DECHCCM"    , "Horizontal Cursor Coupling"           }},
        {DECVCCM  ,  {"DECVCCM"    , "Vertical Cursor Coupling"             }},
        {DECPCCM  ,  {"DECPCCM"    , "Page Cursor Coupling"                 }},
        {DECNKM   ,  {"DECNKM"     , "Numeric Keypad"                       }},
        {DECBKM   ,  {"DECBKM"     , "Backarrow Key"                        }},
        {DECKBUM  ,  {"DECKBUM"    , "Keyboard Usage"                       }},
        {DECVSSM  ,  {"DECVSSM"    , "Vertical Split Screen"                }},
        {DECLRMM  ,  {"DECLRMM"    , "Vertical Split Screen"                }},
        {DECXRLM  ,  {"DECXRLM"    , "Transmit Rate Limiting"               }},
        {DECKPM   ,  {"DECKPM"     , "Key Position"                         }},
        {DECNCSM  ,  {"DECNCSM"    , "No Clearing Screen on Column Change"  }},
        {DECRLCM  ,  {"DECRLCM"    , "Cursor Right to Left"                 }},
        {DECCRTSM ,  {"DECCRTSM"   , "CRT Save"                             }},
        {DECARSM  ,  {"DECARSM"    , "Auto Resize"                          }},
        {DECMCM   ,  {"DECMCM"     , "Modem Control"                        }},
        {DECAAM   ,  {"DECAAM"     , "Auto Answerback"                      }},
        {DECCANSM ,  {"DECCANSM"   , "Conceal Answerback Message"           }},
        {DECNULM  ,  {"DECNULM"    , "Ignoring Null"                        }},
        {DECHDPXM ,  {"DECHDPXM"   , "Half-Duplex"                          }},
        {DECESKM  ,  {"DECESKM"    , "Secondary Keyboard Language"          }},
        {DECOSCNM ,  {"DECOSCNM"   , "Overscan"                             }},
        {M_SF     ,  {"M_SF"       , "Send Focus Events to TTY"             }},
        {M_BP     ,  {"M_BP"       , "Bracketed Paste"                      }},
        {M_SS     ,  {"M_SS"       , "Swap Screen"                          }},
        {M_RBP    ,  {"M_RBP"      , "Report Button Press"                  }},
        {M_RMBP   ,  {"M_RMBP"     , "Report Motion on Button Press"        }},
        {M_EAMM   ,  {"M_EAMM"     , "Enalbe All Mouse Motions"             }},
        {M_ER     ,  {"M_ER"       , "Extened Reporting"                    }},
        {M_SBC    ,  {"M_SBC"      , "Start Blinking Cursor"                }},
        {M_MUTF8  ,  {"M_MUTF8"    , "UTF-8 mouse"                          }},
    };
    for (int i=0; i<LEN(infos); i++)
        if (infos[i].type == type) {
            *info = &infos[i].info;
            return 0;
        }
    return 1;
}

static inline int
get_ctrl_info(unsigned char type, struct CtrlInfo **info) {
    static struct {
        unsigned char type; 
        struct CtrlInfo info;
    } infos [] = { 
        {NUL , {"NUL" , "Null"                                       }},
        {SOH , {"SOH" , "Start of Heading"                           }},
        {STX , {"STX" , "Start of Text"                              }},
        {ETX , {"ETX" , "End of Text"                                }},
        {EOT , {"EOT" , "End of Transmission"                        }},
        {ENQ , {"ENQ" , "Enquiry"                                    }},
        {ACK , {"ACK" , "Acknowledge"                                }},
        {BEL , {"BEL" , "Bell, Alert"                                }},
        {BS  , {"BS"  , "Backspace"                                  }},
        {HT  , {"HT"  , "Character Tabulation, Horizontal Tabulation"}},
        {LF  , {"LF"  , "Line Feed"                                  }},
        {VT  , {"VT"  , "Line Tabulation, Vertical Tabulation"       }},
        {FF  , {"FF"  , "Form Feed"                                  }},
        {CR  , {"CR"  , "Carriage Return"                            }},
        {SO  , {"SO"  , "Shift Out"                                  }},
        {SI  , {"SI"  , "Shift In"                                   }},
        {DLE , {"DLE" , "Data Link Escape"                           }},
        {DC1 , {"DC1" , "Device Control One"                         }},
        {DC2 , {"DC2" , "Device Control Two"                         }},
        {DC3 , {"DC3" , "Device Control Three"                       }},
        {DC4 , {"DC4" , "Device Control Four"                        }},
        {NAK , {"NAK" , "Negative Acknowledge"                       }},
        {SYN , {"SYN" , "Synchronous Idle"                           }},
        {ETB , {"ETB" , "End of Transmission Block"                  }},
        {CAN , {"CAN" , "Cancel"                                     }},
        {EM  , {"EM"  , "End of medium"                              }},
        {SUB , {"SUB" , "Substitute"                                 }},
        {ESC , {"ESC" , "Escape"                                     }},
        {FS  , {"FS"  , "File Separator"                             }},
        {GS  , {"GS"  , "Group Separator"                            }},
        {RS  , {"RS"  , "Record Separator"                           }},
        {US  , {"US"  , "Unit Separator"                             }},
        {DEL , {"DEL" , "Delete"                                     }},
        {PAD , {"PAD" , "Padding Character"                          }},
        {HOP , {"HOP" , "High Octet Preset"                          }},
        {BPH , {"BPH" , "Break Permitted Here"                       }},
        {NBH , {"NBH" , "No Break Here"                              }},
        {IND , {"IND" , "Index"                                      }},
        {NEL , {"NEL" , "Next Line"                                  }},
        {SSA , {"SSA" , "Start of Selected Area"                     }},
        {ESA , {"ESA" , "End of Selected Area"                       }},
        {HTS , {"HTS" , "Horizontal Tabulation Set"                  }},
        {HTJ , {"HTJ" , "Horizontal Tabulation With Justification"   }},
        {VTS , {"VTS" , "Vertical Tabulation Set"                    }},
        {PLD , {"PLD" , "Partial Line Down"                          }},
        {PLU , {"PLU" , "Partial Line Up"                            }},
        {RI  , {"RI"  , "Reverse Index"                              }},
        {SS2 , {"SS2" , "Single-Shift 2"                             }},
        {SS3 , {"SS3" , "Single-Shift 3"                             }},
        {DCS , {"DCS" , "Device Control String"                      }},
        {PU1 , {"PU1" , "Private Use 1"                              }},
        {PU2 , {"PU2" , "Private Use 2"                              }},
        {STS , {"STS" , "Set Transmit State"                         }},
        {CCH , {"CCH" , "Cancel character"                           }},
        {MW  , {"MW"  , "Message Waiting"                            }},
        {SPA , {"SPA" , "Start of Protected Area"                    }},
        {EPA , {"EPA" , "End of Protected Area"                      }},
        {SOS , {"SOS" , "Start of String"                            }},
        {SGCI, {"SGCI", "Single Graphic Character Introducer"        }},
        {SCI , {"SCI" , "Single Character Introducer"                }},
        {CSI , {"CSI" , "Control Sequence Introducer"                }},
        {ST  , {"ST"  , "String Terminator"                          }},
        {OSC , {"OSC" , "Operating System Command"                   }},
        {PM  , {"PM"  , "Privacy Message"                            }},
        {APC , {"APC" , "Application Program Command"                }},
    };

    for (int i=0; i<LEN(infos); i++)
        if (type == infos[i].type) {
            *info = &infos[i].info;
            return 0;
        }
    return 1;
}

static inline int 
get_csi_info(unsigned char type, struct CtrlInfo **info) {
    static struct {
        unsigned char type;
        struct CtrlInfo info;
    } infos[] = {
        {ICH    , {"ICH"    , "Insert Blank Char"               }},
        {CUU    , {"CUU"    , "Cursor Up"                       }},
        {CUD    , {"CUD"    , "Cursor Down"                     }},
        {CUF    , {"CUF"    , "Cursor Forward"                  }},
        {CUB    , {"CUB"    , "Cursor Backward"                 }},
        {CNL    , {"CNL"    , "Cursor Next Line"                }},
        {CPL    , {"CPL"    , "Cursor Previous Line"            }},
        {CHA    , {"CHA"    , "Cursor Horizontal Absolute"      }},
        {CUP    , {"CUP"    , "Cursor Position"                 }},
        {CHT    , {"CHT"    , "Cursor Forward Tabulation"       }},
        {ED     , {"ED"     , "Erase in Display"                }},
        {EL     , {"EL"     , "Erase in Line"                   }},
        {IL     , {"IL"     , "Insert Line"                     }},
        {DL     , {"DL"     , "Delete Line"                     }},
        {DCH    , {"DCH"    , "Delect Char"                     }},
        {SU     , {"SU"     , "Scroll Line Up"                  }},
        {SD     , {"SD"     , "Scroll Line Down"                }},
        {ECH    , {"ECH"    , "Erase Char"                      }},
        {CBT    , {"CBT"    , "Cursor Backward Tabulation"      }},
        {REP    , {"REP"    , "Repeat Print"                    }},
        {DA     , {"DA"     , "Device Attributes"               }},
        {HVP    , {"HVP"    , "Horizontal and Vertical Position"}},
        {TBC    , {"TBC"    , "Tab Clear"                       }},
        {SM     , {"SM"     , "Set Mode"                        }},
        {MC     , {"MC"     , "Media Copy"                      }},
        {RM     , {"RM"     , "Reset Mode"                      }},
        {SGR    , {"SGR"    , "Select Graphic Rendition"        }},
        {DSR    , {"DSR"    , "Device Status Report"            }},
        {DECLL  , {"DECLL"  , "Load LEDs"                       }},
        {DECSTBM, {"DECSTBM", "Set Top and Bottom Margins"      }},
        {DECSC  , {"DECSC"  , "Save Cursor"                     }},
        {DECRC  , {"DECRC"  , "Restore Cursor"                  }},
        {VPA    , {"VPA"    , "Vertical Line Position Absolute" }},
        {VPR    , {"VPR"    , "Vertical Position Relative"      }},
        {HPA    , {"HPA"    , "Horizontal Position Absolute"    }},
        {HPR    , {"HPR"    , "Horizontal Position Relative"    }},
    };
    for (int i=0; i<LEN(infos); i++)
        if (infos[i].type == type) {
            *info = &infos[i].info;
            return 0;
        }
    return 1;
}

static inline int
get_sgr_info(unsigned char type, struct CtrlInfo **info) {
    static struct CtrlInfo infos[108] = {
        {"0"  , "Reset"                                                      },
        {"1"  , "Bold"                                                       },
        {"2"  , "Faint"                                                      },
        {"3"  , "Italic"                                                     },
        {"4"  , "Underline"                                                  },
        {"5"  , "Slow Blink"                                                 },
        {"6"  , "Rapid Blink"                                                },
        {"7"  , "Reverse Video or Invert"                                    },
        {"8"  , "Conceal"                                                    },
        {"9"  , "Crossed Out"                                                },
        {"10" , "Primary Font"                                               },
        {"11" , "Alternative Font"                                           },
        {"12" , "Alternative Font"                                           },
        {"13" , "Alternative Font"                                           },
        {"14" , "Alternative Font"                                           },
        {"15" , "Alternative Font"                                           },
        {"16" , "Alternative Font"                                           },
        {"17" , "Alternative Font"                                           },
        {"18" , "Alternative Font"                                           },
        {"19" , "Alternative Font"                                           },
        {"20" , "Fraktur"                                                    },
        {"21" , "Double Underlined"                                          },
        {"22" , "Normal Intensity"                                           },
        {"23" , "Neither Italic, nor Blackletter"                            },
        {"24" , "Not Underlined"                                             },
        {"25" , "Not Blinking"                                               },
        {"26" , "Proportional Spacing"                                       },
        {"27" , "Not Reversed"                                               },
        {"28" , "Reveal"                                                     },
        {"29" , "Not Crossed Out"                                            },
        {"30" , "Set Foreground Color"                                       },
        {"31" , "Set Foreground Color"                                       },
        {"32" , "Set Foreground Color"                                       },
        {"33" , "Set Foreground Color"                                       },
        {"34" , "Set Foreground Color"                                       },
        {"35" , "Set Foreground Color"                                       },
        {"36" , "Set Foreground Color"                                       },
        {"37" , "Set Foreground Color"                                       },
        {"38" , "Set Foreground Color"                                       },
        {"39" , "Default Foreground Color"                                   },
        {"40" , "Set Background Color"                                       },
        {"41" , "Set Background Color"                                       },
        {"42" , "Set Background Color"                                       },
        {"43" , "Set Background Color"                                       },
        {"44" , "Set Background Color"                                       },
        {"45" , "Set Background Color"                                       },
        {"46" , "Set Background Color"                                       },
        {"47" , "Set Background Color"                                       },
        {"48" , "Set Background Color"                                       },
        {"49" , "Default Background Color"                                   },
        {"50" , "Disable Proportional Spacing"                               },
        {"51" , "Framed"                                                     },
        {"52" , "Encircled"                                                  },
        {"53" , "Overlined"                                                  },
        {"54" , "Neither Framed nor Encircled"                               },
        {"55" , "Not Overlined"                                              },
        {"56" , "Unknown"                                                    },
        {"57" , "Unknown"                                                    },
        {"58" , "Set Underline Color"                                        },
        {"59" , "Default Underline Color"                                    },
        {"60" , "Ideogram Underline or Right Side Line"                      },
        {"61" , "Ideogram Double Underline, or Double Line on the Right Side"},
        {"62" , "Ideogram Overline or Left Side Line"                        },
        {"63" , "Ideogram Double Overline, or Double Line on the Left Side"  },
        {"64" , "Ideogram Stress Marking"                                    },
        {"65" , "No Ideogram Attributes"                                     },
        {"66" , "Unknown"                                                    },
        {"67" , "Unknown"                                                    },
        {"68" , "Unknown"                                                    },
        {"69" , "Unknown"                                                    },
        {"70" , "Unknown"                                                    },
        {"71" , "Unknown"                                                    },
        {"72" , "Unknown"                                                    },
        {"73" , "Superscript"                                                },
        {"74" , "Subscript"                                                  },
        {"75" , "Neither Superscript nor Subscript"                          },
        {"76" , "Unknown"                                                    },
        {"77" , "Unknown"                                                    },
        {"78" , "Unknown"                                                    },
        {"79" , "Unknown"                                                    },
        {"80" , "Unknown"                                                    },
        {"81" , "Unknown"                                                    },
        {"82" , "Unknown"                                                    },
        {"83" , "Unknown"                                                    },
        {"84" , "Unknown"                                                    },
        {"85" , "Unknown"                                                    },
        {"86" , "Unknown"                                                    },
        {"87" , "Unknown"                                                    },
        {"88" , "Unknown"                                                    },
        {"89" , "Unknown"                                                    },
        {"90" , "Set Bright Foreground Color"                                },
        {"91" , "Set Bright Foreground Color"                                },
        {"92" , "Set Bright Foreground Color"                                },
        {"93" , "Set Bright Foreground Color"                                },
        {"94" , "Set Bright Foreground Color"                                },
        {"95" , "Set Bright Foreground Color"                                },
        {"96" , "Set Bright Foreground Color"                                },
        {"97" , "Set Bright Foreground Color"                                },
        {"98" , "Unknown"                                                    },
        {"99" , "Unknown"                                                    },
        {"100", "Set Bright Background Color"                                },
        {"101", "Set Bright Background Color"                                },
        {"102", "Set Bright Background Color"                                },
        {"103", "Set Bright Background Color"                                },
        {"104", "Set Bright Background Color"                                },
        {"105", "Set Bright Background Color"                                },
        {"106", "Set Bright Background Color"                                },
        {"107", "Set Bright Background Color"                                },
    };
    if (type <= 107) {
        *info = &infos[type];
        return 0;
    }
    return 1;
}

static inline int
get_esc_info(unsigned char type, struct CtrlInfo **info) {
    static struct CtrlInfo infos[] = {
        {"NF", "nF Escape"},
        {"FP", "Fp Escape"},
        {"FE", "Fe Escape"},
        {"FS", "Fs Escape"},
    };
    if (type <= 3) {
        *info = &infos[type];
        return 0;
    }
    return 1;
}

#endif
