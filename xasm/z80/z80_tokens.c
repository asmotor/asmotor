/*  Copyright 2008-2017 Carsten Elton Sorensen

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

#include <string.h>

#include "xasm.h"
#include "lexer.h"
#include "lexer_constants.h"
#include "lexer_variadics.h"
#include "options.h"

#include "z80_tokens.h"
#include "z80_options.h"

static SLexConstantsWord g_tokens[] = {
	{ "ADC",	T_Z80_ADC	},
	{ "ADD",	T_Z80_ADD	},
	{ "AND",	T_Z80_AND	},
	{ "BIT",	T_Z80_BIT	},
	{ "CALL",	T_Z80_CALL	},
	{ "CCF",	T_Z80_CCF	},
	{ "CPL",	T_Z80_CPL	},
	{ "CP",		T_Z80_CP	},
	{ "CPD",	T_Z80_CPD	},
	{ "CPDR",	T_Z80_CPDR	},
	{ "CPI",	T_Z80_CPI	},
	{ "CPIR",	T_Z80_CPIR	},
	{ "DAA",	T_Z80_DAA	},
	{ "DEC",	T_Z80_DEC	},
	{ "DI",		T_Z80_DI	},
	{ "DJNZ",	T_Z80_DJNZ	},
	{ "EI",		T_Z80_EI	},
	{ "EX",		T_Z80_EX	},
	{ "EXX",	T_Z80_EXX	},
	{ "HALT",	T_Z80_HALT	},
	{ "IM",		T_Z80_IM	},
	{ "IN",		T_Z80_IN	},
	{ "INC",	T_Z80_INC	},
	{ "IND",	T_Z80_IND	},
	{ "INDR",	T_Z80_INDR	},
	{ "INI",	T_Z80_INI	},
	{ "INIR",	T_Z80_INIR	},
	{ "JP",		T_Z80_JP	},
	{ "JR",		T_Z80_JR	},
	{ "LD",		T_Z80_LD	},
	{ "LDD",	T_Z80_LDD	},
	{ "LDDR",	T_Z80_LDDR	},
	{ "LDI",	T_Z80_LDI	},
	{ "LDIR",	T_Z80_LDIR	},
	{ "LDH",	T_Z80_LDH	},
	{ "LDIO",	T_Z80_LDH	},
	{ "LDHL",	T_Z80_LDHL	},
	{ "NEG",	T_Z80_NEG	},
	{ "NOP",	T_Z80_NOP	},
	{ "OR",		T_Z80_OR	},
	{ "OTDR",	T_Z80_OTDR	},
	{ "OTIR",	T_Z80_OTIR	},
	{ "OUT",	T_Z80_OUT	},
	{ "OUTD",	T_Z80_OUTD	},
	{ "OUTI",	T_Z80_OUTI	},
	{ "POP",	T_Z80_POP	},
	{ "PUSH",	T_Z80_PUSH	},
	{ "RES",	T_Z80_RES	},
	{ "RET",	T_Z80_RET	},
	{ "RETI",	T_Z80_RETI	},
	{ "RETN",	T_Z80_RETN	},
	{ "RL",		T_Z80_RL	},
	{ "RLA",	T_Z80_RLA	},
	{ "RLC",	T_Z80_RLC	},
	{ "RLD",	T_Z80_RLD	},
	{ "RLCA",	T_Z80_RLCA	},
	{ "RR",		T_Z80_RR	},
	{ "RRA",	T_Z80_RRA	},
	{ "RRC",	T_Z80_RRC	},
	{ "RRD",	T_Z80_RRD	},
	{ "RRCA",	T_Z80_RRCA	},
	{ "RST",	T_Z80_RST	},
	{ "SBC",	T_Z80_SBC	},
	{ "SCF",	T_Z80_SCF	},

// HANDLED BY GLOBALLEX.C
// "SET"        ,       T_SYM_SET,

	{ "SLA",	T_Z80_SLA	},
	{ "SLL",	T_Z80_SLL	},
	{ "SRA",	T_Z80_SRA	},
	{ "SRL",	T_Z80_SRL	},
	{ "STOP",	T_Z80_STOP	},
	{ "SUB",	T_Z80_SUB	},
	{ "SWAP",	T_Z80_SWAP	},
	{ "XOR",	T_Z80_XOR	},

	{ "NZ",	T_CC_NZ	},
	{ "Z",	T_CC_Z	},
	{ "NC",	T_CC_NC	},
	{ "PO",	T_CC_PO	},
	{ "PE",	T_CC_PE	},
	{ "P",	T_CC_P	},
	{ "M",	T_CC_M	},
//      "C"             ,       T_MODE_C

	{ "[HL]",	T_MODE_HL_IND	},
	{ "(HL)",	T_MODE_HL_IND	},
	{ "[HL+]",	T_MODE_HL_INDINC	},
	{ "(HL+)",	T_MODE_HL_INDINC	},
	{ "[HL-]",	T_MODE_HL_INDDEC	},
	{ "(HL-)",	T_MODE_HL_INDDEC	},
	{ "HL",		T_MODE_HL		},
	{ "AF",		T_MODE_AF		},
	{ "AF'",	T_MODE_AF_ALT	},
	{ "[BC]",	T_MODE_BC_IND	},
	{ "(BC)",	T_MODE_BC_IND	},
	{ "BC",		T_MODE_BC		},
	{ "[DE]",	T_MODE_DE_IND	},
	{ "(DE)",	T_MODE_DE_IND	},
	{ "DE",		T_MODE_DE		},
	{ "[SP]",	T_MODE_SP_IND	},
	{ "(SP)",	T_MODE_SP_IND	},
	{ "SP",		T_MODE_SP		},
	{ "IX",		T_MODE_IX		},
	{ "IY",		T_MODE_IY		},
	{ "A",		T_MODE_A		},
	{ "B",		T_MODE_B		},
	{ "[C]",	T_MODE_C_IND	},
	{ "(C)",	T_MODE_C_IND	},
	{ "[$FF00+C]",T_MODE_GB_C_IND	},
	{ "($FF00+C)",T_MODE_GB_C_IND	},
	{ "C",		T_MODE_C		},
	{ "D",		T_MODE_D		},
	{ "E",		T_MODE_E		},
	{ "H",		T_MODE_H		},
	{ "L",		T_MODE_L		},
	{ "I",		T_MODE_I		},
	{ "R",		T_MODE_R		},

    { NULL, 0 }
};

static uint32_t
gameboyCharToInt(char ch) {
	for (uint32_t i = 0; i <= 3; ++i) {
		if (opt_Current->machineOptions->gameboyLiteralCharacters[i] == ch)
			return i;
	}

	return 0;
}


static uint32_t 
gameboyLiteralToInt(const char* s) {
	uint32_t result = 0;

	++s;
	while (*s != '\0') {
		uint32_t c = gameboyCharToInt(*s++);
		result = result * 2 + ((c & 1u) << 8u) + ((c & 2u) >> 1u);
	}

	return result;
}


static bool
parseGameboyLiteral(size_t size) {
    char dest[256];

	if (size >= 256)
		size = 255;

	lex_GetChars(dest, size);
    dest[size] = 0;
    lex_Current.value.integer = gameboyLiteralToInt(dest);

    return true;
}


static SVariadicWordDefinition gameboyLiteralWordDefinition = {
	parseGameboyLiteral, T_NUMBER
};


void
z80_DefineTokens(void) {
	/* Gameboy literals */

    z80_gameboyLiteralId = lex_VariadicCreateWord(&gameboyLiteralWordDefinition);
    lex_VariadicAddCharRange(z80_gameboyLiteralId, '`', '`', 0);
	lex_VariadicAddCharRangeRepeating(z80_gameboyLiteralId, '0', '3', 1);

	lex_ConstantsDefineWords(g_tokens);
}
