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

#include "x65_tokens.h"

static SLexConstantsWord g_undocumentedInstructions0[] = {
	{ "AAC", T_6502U_AAC },
	{ "AAX", T_6502U_AAX },
	{ "ARR", T_6502U_ARR },
	{ "ASR", T_6502U_ASR },
	{ "ATX", T_6502U_ATX },
	{ "AXA", T_6502U_AXA },
	{ "AXS", T_6502U_AXS },
	{ "DCP", T_6502U_DCP },
	{ "DOP", T_6502U_DOP },
	{ "ISC", T_6502U_ISC },
	{ "KIL", T_6502U_KIL },
	{ "LAR", T_6502U_LAR },
	{ "LAX", T_6502U_LAX },
	{ "RLA", T_6502U_RLA },
	{ "RRA", T_6502U_RRA },
	{ "SLO", T_6502U_SLO },
	{ "SRE", T_6502U_SRE },
	{ "SXA", T_6502U_SXA },
	{ "SYA", T_6502U_SYA },
	{ "TOP", T_6502U_TOP },
	{ "XAA", T_6502U_XAA },
	{ "XAS", T_6502U_XAS },
	
	{ NULL, 0 }
};

static SLexConstantsWord g_undocumentedInstructions1[] = {
	{ "ANC", T_6502U_AAC },
	{ "SAX", T_6502U_AAX },
	{ "ARR", T_6502U_ARR },
	{ "ASR", T_6502U_ASR },
	{ "LXA", T_6502U_ATX },
	{ "SHA", T_6502U_AXA },
	{ "SBX", T_6502U_AXS },
	{ "DCP", T_6502U_DCP },
	{ "DOP", T_6502U_DOP },
	{ "ISB", T_6502U_ISC },
	{ "JAM", T_6502U_KIL },
	{ "LAE", T_6502U_LAR },
	{ "LAX", T_6502U_LAX },
	{ "RLA", T_6502U_RLA },
	{ "RRA", T_6502U_RRA },
	{ "SLO", T_6502U_SLO },
	{ "SRE", T_6502U_SRE },
	{ "SHX", T_6502U_SXA },
	{ "SHY", T_6502U_SYA },
	{ "TOP", T_6502U_TOP },
	{ "ANE", T_6502U_XAA },
	{ "SHS", T_6502U_XAS },
	
	{ NULL, 0 }
};

static SLexConstantsWord g_undocumentedInstructions2[] = {
	{ "ANC", T_6502U_AAC },
	{ "AXS", T_6502U_AAX },
	{ "ARR", T_6502U_ARR },
	{ "ALR", T_6502U_ASR },
	{ "OAL", T_6502U_ATX },
	{ "AXA", T_6502U_AXA },
	{ "SAX", T_6502U_AXS },
	{ "DCM", T_6502U_DCP },
	{ "SKB", T_6502U_DOP },
	{ "INS", T_6502U_ISC },
	{ "HLT", T_6502U_KIL },
	{ "LAS", T_6502U_LAR },
	{ "LAX", T_6502U_LAX },
	{ "RLA", T_6502U_RLA },
	{ "RRA", T_6502U_RRA },
	{ "ASO", T_6502U_SLO },
	{ "LSE", T_6502U_SRE },
	{ "XAS", T_6502U_SXA },
	{ "SAY", T_6502U_SYA },
	{ "SKW", T_6502U_TOP },
	{ "XAA", T_6502U_XAA },
	{ "TAS", T_6502U_XAS },
	
	{ NULL, 0 }
};

static SLexConstantsWord g_tokens[] = {
	{ "ADC",	T_6502_ADC },
	{ "AND",	T_6502_AND },
	{ "ASL",	T_6502_ASL },
	{ "BIT",	T_6502_BIT },

	{ "BPL",	T_6502_BPL },
	{ "BMI",	T_6502_BMI },
	{ "BVC",	T_6502_BVC },
	{ "BVS",	T_6502_BVS },
	{ "BCC",	T_6502_BCC },
	{ "BCS",	T_6502_BCS },
	{ "BNE",	T_6502_BNE },
	{ "BEQ",	T_6502_BEQ },

	{ "BRK",	T_6502_BRK },
	{ "CMP",	T_6502_CMP },
	{ "CPX",	T_6502_CPX },
	{ "CPY",	T_6502_CPY },
	{ "DEC",	T_6502_DEC },
	{ "EOR",	T_6502_EOR },

	{ "CLC",	T_6502_CLC },
	{ "SEC",	T_6502_SEC },
	{ "CLI",	T_6502_CLI },
	{ "SEI",	T_6502_SEI },
	{ "CLV",	T_6502_CLV },
	{ "CLD",	T_6502_CLD },
	{ "SED",	T_6502_SED },

	{ "INC",	T_6502_INC },
	{ "JMP",	T_6502_JMP },
	{ "JSR",	T_6502_JSR },
	{ "LDA",	T_6502_LDA },
	{ "LDX",	T_6502_LDX },
	{ "LDY",	T_6502_LDY },
	{ "LSR",	T_6502_LSR },
	{ "NOP",	T_6502_NOP },
	{ "ORA",	T_6502_ORA },

	{ "TAX",	T_6502_TAX },
	{ "TXA",	T_6502_TXA },
	{ "DEX",	T_6502_DEX },
	{ "INX",	T_6502_INX },
	{ "TAY",	T_6502_TAY },
	{ "TYA",	T_6502_TYA },
	{ "DEY",	T_6502_DEY },
	{ "INY",	T_6502_INY },

	{ "ROL",	T_6502_ROL },
	{ "ROR",	T_6502_ROR },
	{ "RTI",	T_6502_RTI },
	{ "RTS",	T_6502_RTS },
	{ "SBC",	T_6502_SBC },
	{ "STA",	T_6502_STA },

	{ "TXS",	T_6502_TXS },
	{ "TSX",	T_6502_TSX },
	{ "PHA",	T_6502_PHA },
	{ "PLA",	T_6502_PLA },
	{ "PHP",	T_6502_PHP },
	{ "PLP",	T_6502_PLP },

	{ "STX",	T_6502_STX },
	{ "STY",	T_6502_STY },

	{ "A",	T_6502_REG_A },
	{ "X",	T_6502_REG_X },
	{ "Y",	T_6502_REG_Y },

	{ NULL, 0 }
};

static SLexConstantsWord* g_undocumentedInstructions[] = {
	NULL,
	&g_undocumentedInstructions0[0],
	&g_undocumentedInstructions1[0],
	&g_undocumentedInstructions2[0]
};

void
x65_DefineTokens(void) {
	lex_ConstantsDefineWords(g_tokens);
}

SLexConstantsWord*
x65_GetUndocumentedInstructions(int n) {
	return g_undocumentedInstructions[n];
}
