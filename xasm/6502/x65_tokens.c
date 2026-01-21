/*  Copyright 2008-2026 Carsten Elton Sorensen and contributors

    This file is part of ASMotor.

    ASMotor is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ASMotor is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ASMotor.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stddef.h>

#include "lexer_constants.h"

#include "x65_tokens.h"

static SLexConstantsWord g_undocumentedInstructions0[] = {
    {"AAC", T_6502U_AAC},
    {"AAX", T_6502U_AAX},
    {"ARR", T_6502U_ARR},
    {"ASR", T_6502U_ASR},
    {"ATX", T_6502U_ATX},
    {"AXA", T_6502U_AXA},
    {"AXS", T_6502U_AXS},
    {"DCP", T_6502U_DCP},
    {"DOP", T_6502U_DOP},
    {"ISC", T_6502U_ISC},
    {"KIL", T_6502U_KIL},
    {"LAR", T_6502U_LAR},
    {"LAX", T_6502U_LAX},
    {"RLA", T_6502U_RLA},
    {"RRA", T_6502U_RRA},
    {"SLO", T_6502U_SLO},
    {"SRE", T_6502U_SRE},
    {"SXA", T_6502U_SXA},
    {"SYA", T_6502U_SYA},
    {"TOP", T_6502U_TOP},
    {"XAA", T_6502U_XAA},
    {"XAS", T_6502U_XAS},

    {NULL,  0          },
};

static SLexConstantsWord g_undocumentedInstructions1[] = {
    {"ANC", T_6502U_AAC},
    {"SAX", T_6502U_AAX},
    {"ARR", T_6502U_ARR},
    {"ASR", T_6502U_ASR},
    {"LXA", T_6502U_ATX},
    {"SHA", T_6502U_AXA},
    {"SBX", T_6502U_AXS},
    {"DCP", T_6502U_DCP},
    {"DOP", T_6502U_DOP},
    {"ISB", T_6502U_ISC},
    {"JAM", T_6502U_KIL},
    {"LAE", T_6502U_LAR},
    {"LAX", T_6502U_LAX},
    {"RLA", T_6502U_RLA},
    {"RRA", T_6502U_RRA},
    {"SLO", T_6502U_SLO},
    {"SRE", T_6502U_SRE},
    {"SHX", T_6502U_SXA},
    {"SHY", T_6502U_SYA},
    {"TOP", T_6502U_TOP},
    {"ANE", T_6502U_XAA},
    {"SHS", T_6502U_XAS},

    {NULL,  0          }
};

static SLexConstantsWord g_undocumentedInstructions2[] = {
    {"ANC", T_6502U_AAC},
    {"AXS", T_6502U_AAX},
    {"ARR", T_6502U_ARR},
    {"ALR", T_6502U_ASR},
    {"OAL", T_6502U_ATX},
    {"AXA", T_6502U_AXA},
    {"SAX", T_6502U_AXS},
    {"DCM", T_6502U_DCP},
    {"SKB", T_6502U_DOP},
    {"INS", T_6502U_ISC},
    {"HLT", T_6502U_KIL},
    {"LAS", T_6502U_LAR},
    {"LAX", T_6502U_LAX},
    {"RLA", T_6502U_RLA},
    {"RRA", T_6502U_RRA},
    {"ASO", T_6502U_SLO},
    {"LSE", T_6502U_SRE},
    {"XAS", T_6502U_SXA},
    {"SAY", T_6502U_SYA},
    {"SKW", T_6502U_TOP},
    {"XAA", T_6502U_XAA},
    {"TAS", T_6502U_XAS},

    {NULL,  0          }
};

static SLexConstantsWord g_tokens[] = {
    {"ADC", T_6502_ADC  },
    {"AND", T_6502_AND  },
    {"ASL", T_6502_ASL  },
    {"BIT", T_6502_BIT  },

    {"BPL", T_6502_BPL  },
    {"BMI", T_6502_BMI  },
    {"BVC", T_6502_BVC  },
    {"BVS", T_6502_BVS  },
    {"BCC", T_6502_BCC  },
    {"BLT", T_6502_BCC  },
    {"BCS", T_6502_BCS  },
    {"BGE", T_6502_BCS  },
    {"BNE", T_6502_BNE  },
    {"BEQ", T_6502_BEQ  },

    {"BRK", T_6502_BRK  },
    {"CMP", T_6502_CMP  },
    {"CPX", T_6502_CPX  },
    {"CPY", T_6502_CPY  },
    {"DEC", T_6502_DEC  },
    {"EOR", T_6502_EOR  },

    {"CLC", T_6502_CLC  },
    {"SEC", T_6502_SEC  },
    {"CLI", T_6502_CLI  },
    {"SEI", T_6502_SEI  },
    {"CLV", T_6502_CLV  },
    {"CLD", T_6502_CLD  },
    {"SED", T_6502_SED  },

    {"INC", T_6502_INC  },
    {"JMP", T_6502_JMP  },
    {"JSR", T_6502_JSR  },
    {"LDA", T_6502_LDA  },
    {"LDX", T_6502_LDX  },
    {"LDY", T_6502_LDY  },
    {"LSR", T_6502_LSR  },
    {"NOP", T_6502_NOP  },
    {"ORA", T_6502_ORA  },

    {"TAX", T_6502_TAX  },
    {"TXA", T_6502_TXA  },
    {"DEX", T_6502_DEX  },
    {"INX", T_6502_INX  },
    {"TAY", T_6502_TAY  },
    {"TYA", T_6502_TYA  },
    {"DEY", T_6502_DEY  },
    {"INY", T_6502_INY  },

    {"ROL", T_6502_ROL  },
    {"ROR", T_6502_ROR  },
    {"RTI", T_6502_RTI  },
    {"RTS", T_6502_RTS  },
    {"SBC", T_6502_SBC  },
    {"STA", T_6502_STA  },

    {"TXS", T_6502_TXS  },
    {"TSX", T_6502_TSX  },
    {"PHA", T_6502_PHA  },
    {"PLA", T_6502_PLA  },
    {"PHP", T_6502_PHP  },
    {"PLP", T_6502_PLP  },

    {"STX", T_6502_STX  },
    {"STY", T_6502_STY  },

    {"A",   T_6502_REG_A},
    {"X",   T_6502_REG_X},
    {"Y",   T_6502_REG_Y},

    {NULL,  0           }
};

static SLexConstantsWord* g_undocumentedInstructions[] = {NULL, &g_undocumentedInstructions0[0], &g_undocumentedInstructions1[0],
                                                          &g_undocumentedInstructions2[0]};

static SLexConstantsWord g_C02Instructions[] = {
    {"BRA",  T_65C02_BRA },
    {"DEA",  T_65C02_DEA },
    {"INA",  T_65C02_INA },
    {"PHX",  T_65C02_PHX },
    {"PHY",  T_65C02_PHY },
    {"PLX",  T_65C02_PLX },
    {"PLY",  T_65C02_PLY },
    {"STZ",  T_65C02_STZ },
    {"TRB",  T_65C02_TRB },
    {"TSB",  T_65C02_TSB },

 /* Rockwell + WDC */
    {"BBR",  T_65C02_BBR },
    {"BBR0", T_65C02_BBR0},
    {"BBR1", T_65C02_BBR1},
    {"BBR2", T_65C02_BBR2},
    {"BBR3", T_65C02_BBR3},
    {"BBR4", T_65C02_BBR4},
    {"BBR5", T_65C02_BBR5},
    {"BBR6", T_65C02_BBR6},
    {"BBR7", T_65C02_BBR7},

    {"BBS",  T_65C02_BBS },
    {"BBS0", T_65C02_BBS0},
    {"BBS1", T_65C02_BBS1},
    {"BBS2", T_65C02_BBS2},
    {"BBS3", T_65C02_BBS3},
    {"BBS4", T_65C02_BBS4},
    {"BBS5", T_65C02_BBS5},
    {"BBS6", T_65C02_BBS6},
    {"BBS7", T_65C02_BBS7},

    {"RMB",  T_65C02_RMB },
    {"RMB0", T_65C02_RMB0},
    {"RMB1", T_65C02_RMB1},
    {"RMB2", T_65C02_RMB2},
    {"RMB3", T_65C02_RMB3},
    {"RMB4", T_65C02_RMB4},
    {"RMB5", T_65C02_RMB5},
    {"RMB6", T_65C02_RMB6},
    {"RMB7", T_65C02_RMB7},

    {"SMB",  T_65C02_SMB },
    {"SMB0", T_65C02_SMB0},
    {"SMB1", T_65C02_SMB1},
    {"SMB2", T_65C02_SMB2},
    {"SMB3", T_65C02_SMB3},
    {"SMB4", T_65C02_SMB4},
    {"SMB5", T_65C02_SMB5},
    {"SMB6", T_65C02_SMB6},
    {"SMB7", T_65C02_SMB7},

 /* WDC */
    {"STP",  T_65C02_STP },
    {"WAI",  T_65C02_WAI },

    {NULL,   0           }
};

static SLexConstantsWord g_65816Instructions[] = {
    {"BRL",  T_65816_BRL  },
    {"COP",  T_65816_COP  },
    {"JML",  T_65816_JML  },
    {"JSL",  T_65816_JSL  },
    {"MVN",  T_65816_MVN  },
    {"MVP",  T_65816_MVP  },
    {"PEA",  T_65816_PEA  },
    {"PEI",  T_65816_PEI  },
    {"PER",  T_65816_PER  },
    {"PHB",  T_65816_PHB  },
    {"PHD",  T_65816_PHD  },
    {"PHK",  T_65816_PHK  },
    {"PLB",  T_65816_PLB  },
    {"PLD",  T_65816_PLD  },
    {"REP",  T_65816_REP  },
    {"RTL",  T_65816_RTL  },
    {"SEP",  T_65816_SEP  },
    {"TCD",  T_65816_TCD  },
    {"TCS",  T_65816_TCS  },
    {"TDC",  T_65816_TDC  },
    {"TSC",  T_65816_TSC  },
    {"TXY",  T_65816_TXY  },
    {"TYX",  T_65816_TYX  },
    {"WDM",  T_65816_WDM  },
    {"XBA",  T_65816_XBA  },
    {"XCE",  T_65816_XCE  },

    {"S",    T_65816_REG_S},

    {"BITS", T_65816_BITS },

    {NULL,   0            }
};

static SLexConstantsWord g_4510Instructions[] = {
    {"ASR",   T_4510_ASR    },
    {"ASW",   T_4510_ASW    },
    {"BSR",   T_4510_BSR    },
    {"CLE",   T_4510_CLE    },
    {"CPZ",   T_4510_CPZ    },
    {"DEW",   T_4510_DEW    },
    {"DEZ",   T_4510_DEZ    },
    {"EOM",   T_6502_NOP    },
    {"INW",   T_4510_INW    },
    {"INZ",   T_4510_INZ    },
    {"LBCC",  T_4510_LBCC   },
    {"LBLT",  T_4510_LBCC   },
    {"LBCS",  T_4510_LBCS   },
    {"LBGE",  T_4510_LBCS   },
    {"LBEQ",  T_4510_LBEQ   },
    {"LBMI",  T_4510_LBMI   },
    {"LBNE",  T_4510_LBNE   },
    {"LBPL",  T_4510_LBPL   },
    {"LBRA",  T_4510_LBRA   },
    {"LBVC",  T_4510_LBVC   },
    {"LBVS",  T_4510_LBVS   },
    {"LDZ",   T_4510_LDZ    },
    {"MAP",   T_4510_MAP    },
    {"NEG",   T_4510_NEG    },
    {"PHW",   T_4510_PHW    },
    {"PHZ",   T_4510_PHZ    },
    {"PLZ",   T_4510_PLZ    },
    {"ROW",   T_4510_ROW    },
    {"SEE",   T_4510_SEE    },
    {"TAB",   T_4510_TAB    },
    {"TAZ",   T_4510_TAZ    },
    {"TBA",   T_4510_TBA    },
    {"TSY",   T_4510_TSY    },
    {"TYS",   T_4510_TYS    },
    {"TZA",   T_4510_TZA    },

    {"ADCQ",  T_45GS02_ADCQ },
    {"ANDQ",  T_45GS02_ANDQ },
    {"ASLQ",  T_45GS02_ASLQ },
    {"ASRQ",  T_45GS02_ASRQ },
    {"BITQ",  T_45GS02_BITQ },
    {"CMPQ",  T_45GS02_CMPQ },
    {"DEQ",   T_45GS02_DEQ  },
    {"EORQ",  T_45GS02_EORQ },
    {"INQ",   T_45GS02_INQ  },
    {"LDQ",   T_45GS02_LDQ  },
    {"LSRQ",  T_45GS02_LSRQ },
    {"ORQ",   T_45GS02_ORQ  },
    {"ROLQ",  T_45GS02_ROLQ },
    {"RORQ",  T_45GS02_RORQ },
    {"SBCQ",  T_45GS02_SBCQ },
    {"STQ",   T_45GS02_STQ  },

    {"SETBP", T_45GS02_SETBP},

    {"Z",     T_4510_REG_Z  },
    {"Q",     T_45GS02_REG_Q},
    {"SP",    T_65816_REG_S },

    {NULL,    0             }
};

void
x65_DefineTokens(void) {
	lex_ConstantsDefineWords(g_tokens);
	lex_ConstantsDefineWords(g_C02Instructions);
	lex_ConstantsDefineWords(g_65816Instructions);
}

SLexConstantsWord*
x65_GetUndocumentedInstructions(int n) {
	return g_undocumentedInstructions[n];
}

SLexConstantsWord*
x65_Get4510Instructions(void) {
	return g_4510Instructions;
}
