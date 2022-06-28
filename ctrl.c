#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "ctrl.h"
#include "tool.h"

int
get_fp_esc_info(int type, struct CtrlInfo **info) {
    static struct {
        int type;
        struct CtrlInfo info;
    } infos[] = {
        {FP_DECPAM   ,  {"DECPAM"     , "Application Keypad"     }},
        {FP_DECPNM   ,  {"DECPNM"     , "Normal Keypad"          }},
        {FP_DECSC    ,  {"DECSC"      , "Save Cursor"            }},
        {FP_DECRC    ,  {"DECRC"      , "Restore Cursor"         }},
    };
    for (int i = 0; i < LEN(infos); i++)
        if (infos[i].type == type) {
            *info = &infos[i].info;
            return 0;
        }
    return 1;
}

int
get_nf_esc_info(int type, struct CtrlInfo **info) {
    static struct {
        int type;
        struct CtrlInfo info;
    } infos[] = {
        {NF_GZD4   ,  {"GZD4"     , "Set Charset G0"     }},
        {NF_G1D4   ,  {"G1D4"     , "Set Charset G1"     }},
        {NF_G2D4   ,  {"G2D4"     , "Set Charset G2"     }},
        {NF_G3D4   ,  {"G3D4"     , "Set Charset G3"     }},
    };
    for (int i = 0; i < LEN(infos); i++)
        if (infos[i].type == type) {
            *info = &infos[i].info;
            return 0;
        }
    return 1;
}

int 
get_mode_info(int type, struct CtrlInfo **info) {
    static struct {
        int type;
        struct CtrlInfo info;
    } infos[] = {
        {DECCKM     ,  {"DECCKM"     , "Cursor keys"                          }},
        {DECANM     ,  {"DECANM"     , "ANSI"                                 }},
        {DECCOLM    ,  {"DECCOLM"    , "Column"                               }},
        {DECSCLM    ,  {"DECSCLM"    , "Scrolling"                            }},
        {DECSCNM    ,  {"DECSCNM"    , "Screen"                               }},
        {DECOM      ,  {"DECOM"      , "Origin"                               }},
        {DECAWM     ,  {"DECAWM"     , "Autowrap"                             }},
        {DECARM     ,  {"DECARM"     , "Autorepeat"                           }},
        {DECPFF     ,  {"DECPFF"     , "Print form Feed"                      }},
        {DECPEX     ,  {"DECPEX"     , "Printer Extent"                       }},
        {DECTCEM    ,  {"DECTCEM"    , "Text Cursor Enable"                   }},
        {DECRLM     ,  {"DECRLM"     , "Cursor Direction, Right to Left"      }},
        {DECHEBM    ,  {"DECHEBM"    , "Hebrew Keyboard Mapping"              }},
        {DECHEM     ,  {"DECHEM"     , "Hebrew Encoding Mode"                 }},
        {DECNRCM    ,  {"DECNRCM"    , "National Replacement Character Set"   }},
        {DECNAKB    ,  {"DECNAKB"    , "Greek keyboard Mapping"               }},
        {DECHCCM    ,  {"DECHCCM"    , "Horizontal Cursor Coupling"           }},
        {DECVCCM    ,  {"DECVCCM"    , "Vertical Cursor Coupling"             }},
        {DECPCCM    ,  {"DECPCCM"    , "Page Cursor Coupling"                 }},
        {DECNKM     ,  {"DECNKM"     , "Numeric Keypad"                       }},
        {DECBKM     ,  {"DECBKM"     , "Backarrow Key"                        }},
        {DECKBUM    ,  {"DECKBUM"    , "Keyboard Usage"                       }},
        {DECVSSM    ,  {"DECVSSM"    , "Vertical Split Screen"                }},
        {DECLRMM    ,  {"DECLRMM"    , "Vertical Split Screen"                }},
        {DECXRLM    ,  {"DECXRLM"    , "Transmit Rate Limiting"               }},
        {DECKPM     ,  {"DECKPM"     , "Key Position"                         }},
        {DECNCSM    ,  {"DECNCSM"    , "No Clearing Screen on Column Change"  }},
        {DECRLCM    ,  {"DECRLCM"    , "Cursor Right to Left"                 }},
        {DECCRTSM   ,  {"DECCRTSM"   , "CRT Save"                             }},
        {DECARSM    ,  {"DECARSM"    , "Auto Resize"                          }},
        {DECMCM     ,  {"DECMCM"     , "Modem Control"                        }},
        {DECAAM     ,  {"DECAAM"     , "Auto Answerback"                      }},
        {DECCANSM   ,  {"DECCANSM"   , "Conceal Answerback Message"           }},
        {DECNULM    ,  {"DECNULM"    , "Ignoring Null"                        }},
        {DECHDPXM   ,  {"DECHDPXM"   , "Half-Duplex"                          }},
        {DECESKM    ,  {"DECESKM"    , "Secondary Keyboard Language"          }},
        {DECOSCNM   ,  {"DECOSCNM"   , "Overscan"                             }},
        {M_SF       ,  {"M_SF"       , "Send Focus Events to TTY"             }},
        {M_BP       ,  {"M_BP"       , "Bracketed Paste"                      }},
        {M_SBC      ,  {"M_SBC"      , "Start Blinking Cursor"                }},
        {M_MUTF8    ,  {"M_MUTF8"    , "UTF-8 mouse"                          }},
        {M_MP       ,  {"M_MP"       , "Report Button Press"                  }},
        {M_MMP      ,  {"M_MM "      , "Report Motion on Button Press"        }},
        {M_MMA      ,  {"M_MMA"      , "Enalbe All Mouse Motions"             }},
        {M_ME       ,  {"M_ME"       , "Extened Reporting"                    }},
        {M_ALTS     ,  {"M_ALTS"     , "Use Alternate Screen Buffer"          }},
        {M_SC       ,  {"M_SC"       , "Save Cursor"                          }},
        {M_SC_ALTS  ,  {"M_SC_ALTS"  , "M_SC and M_ALTS"                      }},
    };
    for (int i = 0; i < LEN(infos); i++)
        if (infos[i].type == type) {
            *info = &infos[i].info;
            return 0;
        }
    return 1;
}

int
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

    for (int i = 0; i < LEN(infos); i++)
        if (type == infos[i].type) {
            *info = &infos[i].info;
            return 0;
        }
    return 1;
}

int 
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
        {WINMAN , {"WINMAN" , "Window Manipulation"             }},
    };
    for (int i = 0; i < LEN(infos); i++)
        if (infos[i].type == type) {
            *info = &infos[i].info;
            return 0;
        }
    return 1;
}

int
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

int
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

int
str_to_dec(char *p, int *r) {
    long ret;
    char *ed;

    *r = 0;
    if (p == NULL || !strlen(p))
        return EINVAL;

    ret = strtol(p, &ed, 10);
    if (ed[0])
        return EILSEQ;
    *r = ret;
    return 0;
}

int
range_search(unsigned char *seq, int len,
        unsigned char a, unsigned char b, int mode) {
    for (int i = 0; i < len; i++) {
        if ((!mode) && (seq[i] >= a && seq[i] <= b))
            return i;
        if ((mode) && (seq[i] < a && seq[i] > b))
            return i;
    }
    return -1;
}

int
search(unsigned char *seq, int len, int n, ...) {
    int i, j;
    va_list ap;
    unsigned char c;

    va_start(ap, n);
    for (i = 0; i < n; i++) {
        c = va_arg(ap, int);
        for (j = 0; j < len; j++)
            if (seq[j] == c)
                return j;
    }
    va_end(ap);
    return -1;
}

int
get_par_num(struct Esc *esc) {
    int i, n;

    for (i = 1, n = 0; i < esc->len-1; i++) {
        if (esc->seq[i] == ';')
            n++;
    }
    return n+1;
}

int
get_par(struct Esc *esc, int idx, int *a, int *b) {
    int i, n;

    *a = *b = 1;
    for (i = 1, n = 0; i < esc->len-1; i++) {
        if (esc->seq[i] != ';')
            continue;
        if (n == idx) {
            *b = i-1;
            break;
        }
        *a = i+1;
        n++;
    }

    if (i == esc->len-1) {
        if (n == idx)
            *b = esc->len-2;
        else
            return 1;
    }

    if (*a > *b) {
        *a = -1;
        *b = -1;
    }

    return 0;
}

int
get_str_par(struct Esc *esc, int idx, char **p) {
    static char buf[BUFSIZ];
    int a, b;
    size_t n;

    if (get_par(esc, idx, &a, &b))
        return EDOM;

    if (a<0) {
        *p = NULL;
        return 0;
    }

    n = b-a+1;
    if (n > sizeof(buf)-1)
        return ENOMEM;

    memcpy(buf, esc->seq+a, n);
    buf[n] = '\0';
    *p = buf;
    return 0;

}

int
get_int_par(struct Esc *esc, int idx, int *v, int v0) {
    int ret;
    char *p;

    ret = get_str_par(esc, idx, &p);
    if (ret)
        return ret;

    if (p == NULL) {
        *v = v0;
        return 0;
    }
    return str_to_dec(p, v);
}

void // debug
csi_dump(struct Esc *esc) {
    int i, r, rr, a, b;
    
     if (esc->csi != SGR)
        return;
     dump(esc->seq, esc->len);
     for (i = 0; i < get_par_num(esc); i++) {
         if ((rr = get_int_par(esc, i, &r, 1)))
             printf("%s\n", strerror(rr));
         else
             printf("%d\n", r);
         if (!get_par(esc, i, &a, &b)) {
             if (a<0)
                 printf("-\n");
             else
                 dump(esc->seq+a, b-a+1);
         }
     }
    // printf("%s\n", get_esc_str(esc, 1));
}

int
find_osc_end(unsigned char *seq, int len, int *n) {
    if ((*n = search(seq, len, 2, BEL, ST)) < 0)
        return ESCOSCNOEND;
    return 0;
}

int
find_csi_end(unsigned char *seq, int len, int *n) {
    if ((*n = range_search(seq, len, 0x40, 0x7E, 0)) < 0)
        return ESCCSINOEND;
    return 0;
}

int
find_dcs_end(unsigned char *seq, int len, int *n) {
    if ((*n = search(seq, len, 1, ST-0x80+0x40)) < 0)
        return ESCDCSNOEND;
    return 0;
}

int
find_nfesc_end(unsigned char *seq, int len, int *n) {
    if ((*n = range_search(seq, len, 0x30, 0x7E, 0)) < 0)
        return ESCNFNOEND;
    return 0;
}

int
esc_parse(unsigned char *seq, int len, struct Esc *esc) {
    unsigned char c;
    int n, ret;

    esc->type = -1;
    esc->seq = seq;
    ASSERT(len >= 0, "");
    if (!len)
        return ESCNOEND;

    //dump(seq, len);
    c = seq[0];

    if (c < 0x20 || c > 0x7E)
        return ESCERR;
    esc->len = 1;

    if (c >= 0x20 && c <= 0x2F) {
        esc->type = ESCNF;
        esc->esc = c;
        if ((ret = find_nfesc_end(seq+1, len-1, &n)))
            return ret;
        esc->len += n+1;
    }

    if (c >= 0x40 && c <= 0x5F) {
        esc->type = ESCFE;
        esc->esc = c - 0x40 + 0x80;
        switch (esc->esc) {
            case CSI:
                if ((ret = find_csi_end(seq+1, len-1, &n)))
                    return ret;
                esc->len += n+1;
                esc->csi = seq[esc->len-1];
                //csi_dump(esc);
                break;

            case OSC:
                if ((ret = find_osc_end(seq+1, len-1, &n)))
                    return ret;
                esc->len += n+1;
                //dump(esc->seq, esc->len-1);
                break;
            case DCS:
                if ((ret = find_dcs_end(seq+1, len-1, &n)))
                    return ret;
                esc->len += n+1;
                break;
        }
    }

    if (c >= 0x60 && c <= 0x7E) {
        esc->type = ESCFS;
        esc->esc = c;
    }

    if (c >= 0x30 && c <= 0x3F) {
        esc->type = ESCFP;
        esc->esc = c;
    }

    return 0;
}

void
esc_reset(struct Esc *esc) {
    bzero(esc, sizeof(struct Esc));
    esc->len = 0;
    esc->seq = NULL;
}

char* // debug
get_esc_str(struct Esc *esc, int desc) {
    struct CtrlInfo *info, *info2;
    int i, pos, n, ret;
    char *p;
    static char buf[BUFSIZ];

#define MYPRINT(fmt, ...) \
    pos += snprintf(buf+pos, sizeof(buf)-pos, fmt, ##__VA_ARGS__)

    pos = 0;
    buf[0] = 0;
    if (!esc->len)
        return buf;

    if (get_esc_info(esc->type, &info)) {
        MYPRINT("Unknown esc type");
        return buf;
    }
    
    MYPRINT("ESC %s ", info->name);
    switch (esc->type) {
        case ESCFE: goto escfe;
        case ESCNF: goto escnf;
        case ESCFS: goto escfs;
        case ESCFP: goto escfp;
        default: ASSERT(0, "can't be");
    }

escfe:
    if (get_ctrl_info(esc->esc, &info)) {
        MYPRINT("error fe esc type");
        return buf;
    }
    MYPRINT("%s ", info->name);

    switch (esc->esc) {
        case CSI:
            if (get_csi_info(esc->csi, &info)) {
                MYPRINT("%c ", esc->csi);
                desc = 0;
            } else {
                MYPRINT("%s ", info->name);
            }

            if (esc->csi == RM ||  esc->csi == SM) {
                if (esc->seq[1] != '?')
                    MYPRINT("unknown ");
                else
                    for (i = 0; i < get_par_num(esc); i++) {
                        if (get_str_par(esc, i, &p) || p == NULL) {
                            MYPRINT("unknown ");
                            continue;
                        }
                        if (i == 0)
                            ret = str_to_dec(p+1, &n);
                        else
                            ret = str_to_dec(p, &n);

                        if (ret || (ret = get_mode_info(n, &info2))) {
                            MYPRINT("unknown ");
                            continue;
                        }
                        MYPRINT("%s ", info2->name);
                        if (desc)
                            MYPRINT("%s ", info2->desc);
                    }
            }

            MYPRINT("[");
            n = get_par_num(esc);
            for (i = 0; i < n; i++) {
                if (get_str_par(esc, i, &p) || p == NULL)
                    MYPRINT("%s", "-");
                else
                    MYPRINT("%s", p);
                if (i < n-1)
                    MYPRINT(",");
            }
            MYPRINT("] ");
            break;

        case OSC:
            break;

    }
    if (desc)
        MYPRINT("`%s`", info->desc);
    return buf;

escnf:
    if (get_nf_esc_info(esc->esc, &info))
        MYPRINT("0x%X (%c)", esc->esc, esc->esc);
    else {
        MYPRINT("%s ", info->name);
        if (desc)
            MYPRINT("`%s`", info->desc);
    }
    return buf;

escfp:
    if (get_fp_esc_info(esc->esc, &info))
        MYPRINT("0x%X (%c)", esc->esc, esc->esc);
    else {
        MYPRINT("%s ", info->name);
        if (desc)
            MYPRINT("`%s`", info->desc);
    }
    return buf;

escfs:
    MYPRINT("0x%X (%c)", esc->esc, esc->esc);
    return buf;

#undef MYPRINT
}
