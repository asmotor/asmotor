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
#include "localasm.h"
#include "options.h"
#include "locopt.h"

static SLexerTokenDefinition localstrings[]=
{
	{ "adc",	T_Z80_ADC	},
	{ "add",	T_Z80_ADD	},
	{ "and",	T_Z80_AND	},
	{ "bit",	T_Z80_BIT	},
	{ "call",	T_Z80_CALL	},
	{ "ccf",	T_Z80_CCF	},
	{ "cpl",	T_Z80_CPL	},
	{ "cp",		T_Z80_CP	},
	{ "cpd",	T_Z80_CPD	},
	{ "cpdr",	T_Z80_CPDR	},
	{ "cpi",	T_Z80_CPI	},
	{ "cpir",	T_Z80_CPIR	},
	{ "daa",	T_Z80_DAA	},
	{ "dec",	T_Z80_DEC	},
	{ "di",		T_Z80_DI	},
	{ "djnz",	T_Z80_DJNZ	},
	{ "ei",		T_Z80_EI	},
	{ "ex",		T_Z80_EX	},
	{ "exx",	T_Z80_EXX	},
	{ "halt",	T_Z80_HALT	},
	{ "im",		T_Z80_IM	},
	{ "in",		T_Z80_IN	},
	{ "inc",	T_Z80_INC	},
	{ "ind",	T_Z80_IND	},
	{ "indr",	T_Z80_INDR	},
	{ "ini",	T_Z80_INI	},
	{ "inir",	T_Z80_INIR	},
	{ "jp",		T_Z80_JP	},
	{ "jr",		T_Z80_JR	},
	{ "ld",		T_Z80_LD	},
	{ "ldd",	T_Z80_LDD	},
	{ "lddr",	T_Z80_LDDR	},
	{ "ldi",	T_Z80_LDI	},
	{ "ldir",	T_Z80_LDIR	},
	{ "ldh",	T_Z80_LDH	},
	{ "ldio",	T_Z80_LDH	},
	{ "ldhl",	T_Z80_LDHL	},
	{ "neg",	T_Z80_NEG	},
	{ "nop",	T_Z80_NOP	},
	{ "or",		T_Z80_OR	},
	{ "otdr",	T_Z80_OTDR	},
	{ "otir",	T_Z80_OTIR	},
	{ "out",	T_Z80_OUT	},
	{ "outd",	T_Z80_OUTD	},
	{ "outi",	T_Z80_OUTI	},
	{ "pop",	T_Z80_POP	},
	{ "push",	T_Z80_PUSH	},
	{ "res",	T_Z80_RES	},
	{ "ret",	T_Z80_RET	},
	{ "reti",	T_Z80_RETI	},
	{ "retn",	T_Z80_RETN	},
	{ "rl",		T_Z80_RL	},
	{ "rla",	T_Z80_RLA	},
	{ "rlc",	T_Z80_RLC	},
	{ "rld",	T_Z80_RLD	},
	{ "rlca",	T_Z80_RLCA	},
	{ "rr",		T_Z80_RR	},
	{ "rra",	T_Z80_RRA	},
	{ "rrc",	T_Z80_RRC	},
	{ "rrd",	T_Z80_RRD	},
	{ "rrca",	T_Z80_RRCA	},
	{ "rst",	T_Z80_RST	},
	{ "sbc",	T_Z80_SBC	},
	{ "scf",	T_Z80_SCF	},

// Handled by globallex.c
// "set"        ,       T_POP_SET,

	{ "sla",	T_Z80_SLA	},
	{ "sll",	T_Z80_SLL	},
	{ "sra",	T_Z80_SRA	},
	{ "srl",	T_Z80_SRL	},
	{ "stop",	T_Z80_STOP	},
	{ "sub",	T_Z80_SUB	},
	{ "swap",	T_Z80_SWAP	},
	{ "xor",	T_Z80_XOR	},

	{ "nz",	T_CC_NZ	},
	{ "z",	T_CC_Z	},
	{ "nc",	T_CC_NC	},
	{ "po",	T_CC_PO	},
	{ "pe",	T_CC_PE	},
	{ "p",	T_CC_P	},
	{ "m",	T_CC_M	},
//      "c"             ,       T_MODE_C

	{ "[hl]",	T_MODE_HL_IND	},
	{ "(hl)",	T_MODE_HL_IND	},
	{ "[hl+]",	T_MODE_HL_INDINC	},
	{ "(hl+)",	T_MODE_HL_INDINC	},
	{ "[hl-]",	T_MODE_HL_INDDEC	},
	{ "(hl-)",	T_MODE_HL_INDDEC	},
	{ "hl",		T_MODE_HL		},
	{ "af",		T_MODE_AF		},
	{ "af'",	T_MODE_AF_ALT	},
	{ "[bc]",	T_MODE_BC_IND	},
	{ "(bc)",	T_MODE_BC_IND	},
	{ "bc",		T_MODE_BC		},
	{ "[de]",	T_MODE_DE_IND	},
	{ "(de)",	T_MODE_DE_IND	},
	{ "de",		T_MODE_DE		},
	{ "[sp]",	T_MODE_SP_IND	},
	{ "(sp)",	T_MODE_SP_IND	},
	{ "sp",		T_MODE_SP		},
	{ "ix",		T_MODE_IX		},
	{ "iy",		T_MODE_IY		},
	{ "a",		T_MODE_A		},
	{ "b",		T_MODE_B		},
	{ "[c]",	T_MODE_C_IND	},
	{ "(c)",	T_MODE_C_IND	},
	{ "[$ff00+c]",T_MODE_GB_C_IND	},
	{ "($ff00+c)",T_MODE_GB_C_IND	},
	{ "c",		T_MODE_C		},
	{ "d",		T_MODE_D		},
	{ "e",		T_MODE_E		},
	{ "h",		T_MODE_H		},
	{ "l",		T_MODE_L		},
	{ "i",		T_MODE_I		},
	{ "r",		T_MODE_R		},

    { NULL, 0 }
};

static uint32_t gbgfx2bin(char ch)
{
	for(uint32_t i = 0; i <= 3; ++i)
	{
		if (opt_Current->machineOptions->GameboyChar[i] == ch)
			return i;
	}

	return 0;
}


static uint32_t ascii2bin(char* s)
{
	uint32_t result = 0;

	++s;
	while(*s != '\0')
	{
		uint32_t c = gbgfx2bin(*s++);
		result = result * 2 + ((c & 1u) << 8u) + ((c & 2u) >> 1u);
	}

	return result;
}


static bool ParseGameboyNumber(size_t size)
{
    char dest[256];

	if(size >= 256)
		size = 255;

	lex_GetChars(dest, size);
    dest[size] = 0;
    lex_Current.value.integer = ascii2bin(dest);

    return true;
}


static SVariadicWordDefinition	tNumberToken=
{
	ParseGameboyNumber,
	T_NUMBER
};


void	loclexer_Init(void)
{
	/* Gameboy constants */

    g_GameboyLiteralId = lex_VariadicCreateWord(&tNumberToken);
    lex_VariadicAddCharRange(g_GameboyLiteralId, '`', '`', 0);
	lex_VariadicAddCharRangeRepeating(g_GameboyLiteralId, '0', '3', 1);

	lex_DefineTokens(localstrings);
}
