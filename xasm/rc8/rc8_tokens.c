/*  Copyright 2008-2022 Carsten Elton Sorensen and contributors

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

#include "xasm.h"
#include "lexer_constants.h"

#include "rc8_tokens.h"

static SLexConstantsWord 
g_tokens[] = {
	{ "ADD",    T_RC8_ADD    },
	{ "AND",    T_RC8_AND    },
	{ "CMP",    T_RC8_CMP    },
	{ "DI",     T_RC8_DI     },
	{ "DJ", 	T_RC8_DJ     },
	{ "EI",     T_RC8_EI     },
	{ "EXG",    T_RC8_EXG    },
	{ "EXT",    T_RC8_EXT    },
	{ "J",      T_RC8_J      },
	{ "JAL",    T_RC8_JAL    },
	{ "LCO",    T_RC8_LCO    },
	{ "LCR",    T_RC8_LCR    },
	{ "LD",     T_RC8_LD     },
	{ "LIO",    T_RC8_LIO    },
	{ "LS",     T_RC8_LS     },
	{ "NEG",    T_RC8_NEG    },
	{ "NOP",    T_RC8_NOP    },
	{ "NOT",    T_RC8_NOT    },
	{ "OR",     T_RC8_OR     },
	{ "POP",    T_RC8_POP    },
	{ "POPA",   T_RC8_POPA   },
	{ "PUSH",   T_RC8_PUSH   },
	{ "PUSHA",  T_RC8_PUSHA  },
	{ "RETI",   T_RC8_RETI   },
	{ "RS",     T_RC8_RS     },
	{ "RSA",    T_RC8_RSA    },
	{ "SUB",    T_RC8_SUB    },
	{ "SWAP",   T_RC8_SWAP   },
	{ "SWAPA",  T_RC8_SWAPA  },
	{ "SYS",    T_RC8_SYS    },
	{ "TST",    T_RC8_TST    },
	{ "XOR",    T_RC8_XOR    },

	{ "F", T_RC8_REG_F },
	{ "T", T_RC8_REG_T },
	{ "B", T_RC8_REG_B },
	{ "C", T_RC8_REG_C },
	{ "D", T_RC8_REG_D },
	{ "E", T_RC8_REG_E },
	{ "H", T_RC8_REG_H },
	{ "L", T_RC8_REG_L },
	{ "FT", T_RC8_REG_FT },
	{ "BC", T_RC8_REG_BC },
	{ "DE", T_RC8_REG_DE },
	{ "HL", T_RC8_REG_HL },
	{ "(C)", T_RC8_REG_C_IND },
	{ "(FT)", T_RC8_REG_FT_IND },
	{ "(BC)", T_RC8_REG_BC_IND },
	{ "(DE)", T_RC8_REG_DE_IND },
	{ "(HL)", T_RC8_REG_HL_IND },

	{ "EQ",		T_RC8_CC_EQ},
	{ "NE",		T_RC8_CC_NE},
	{ "LE",		T_RC8_CC_LE},
	{ "GT",		T_RC8_CC_GT},
	{ "LT",		T_RC8_CC_LT},
	{ "GE",		T_RC8_CC_GE},
	{ "LEU",	T_RC8_CC_LEU},
	{ "GTU",	T_RC8_CC_GTU},
	{ "LTU",	T_RC8_CC_LTU},
	{ "GEU",	T_RC8_CC_GEU},

	{ "Z",		T_RC8_CC_EQ },
	{ "NZ",		T_RC8_CC_NE },

	{ "NC",		T_RC8_CC_GEU},

	{ NULL, 0 }
};

void 
rc8_DefineTokens(void) {
	lex_ConstantsDefineWords(g_tokens);
}
