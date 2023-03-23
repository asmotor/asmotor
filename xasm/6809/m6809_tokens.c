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

#include "m6809_tokens.h"

static SLexConstantsWord g_tokens[] = {
	{ "ABX",	T_6809_ABX },
	{ "ADCA",	T_6809_ADCA },
	{ "ADCB",	T_6809_ADCB },
	{ "ADDA",	T_6809_ADDA },
	{ "ADDB",	T_6809_ADDB },
	{ "ADDD",	T_6809_ADDD },
	{ "ANDA",	T_6809_ANDA },
	{ "ANDB",	T_6809_ANDB },
	{ "ANDCC",	T_6809_ANDCC },
	{ "ASL",	T_6809_ASL },
	{ "LSL",	T_6809_ASL },
	{ "ASLA",	T_6809_ASLA },
	{ "LSLA",	T_6809_ASLA },
	{ "ASLB",	T_6809_ASLB },
	{ "LSLB",	T_6809_ASLB },
	{ "ASR",	T_6809_ASR },
	{ "ASRA",	T_6809_ASRA },
	{ "ASRB",	T_6809_ASRB },
	{ "NOP",	T_6809_NOP },

	{ "BRA",	T_6809_BRA },
	{ "BRN",	T_6809_BRN },
	{ "BHI",	T_6809_BHI },
	{ "BLS",	T_6809_BLS },
	{ "BCC",	T_6809_BHS },
	{ "BHS",	T_6809_BHS },
	{ "BCS",	T_6809_BLO },
	{ "BLO",	T_6809_BLO },
	{ "BNE",	T_6809_BNE },
	{ "BEQ",	T_6809_BEQ },
	{ "BVC",	T_6809_BVC },
	{ "BVS",	T_6809_BVS },
	{ "BPL",	T_6809_BPL },
	{ "BMI",	T_6809_BMI },
	{ "BGE",	T_6809_BGE },
	{ "BLT",	T_6809_BLT },
	{ "BGT",	T_6809_BGT },
	{ "BLE",	T_6809_BLE },
	{ "BSR",	T_6809_BSR },

	{ "LBRA",	T_6809_LBRA },
	{ "LBRN",	T_6809_LBRN },
	{ "LBHI",	T_6809_LBHI },
	{ "LBLS",	T_6809_LBLS },
	{ "LBCC",	T_6809_LBHS },
	{ "LBHS",	T_6809_LBHS },
	{ "LBCS",	T_6809_LBLO },
	{ "LBLO",	T_6809_LBLO },
	{ "LBNE",	T_6809_LBNE },
	{ "LBEQ",	T_6809_LBEQ },
	{ "LBVC",	T_6809_LBVC },
	{ "LBVS",	T_6809_LBVS },
	{ "LBPL",	T_6809_LBPL },
	{ "LBMI",	T_6809_LBMI },
	{ "LBGE",	T_6809_LBGE },
	{ "LBLT",	T_6809_LBLT },
	{ "LBGT",	T_6809_LBGT },
	{ "LBLE",	T_6809_LBLE },
	{ "LBSR",	T_6809_LBSR },

	{ "BITA",	T_6809_BITA },
	{ "BITB",	T_6809_BITB },

	{ "CLR",	T_6809_CLR },
	{ "CLRA",	T_6809_CLRA },
	{ "CLRB",	T_6809_CLRB },

	{ "CMPA",	T_6809_CMPA },
	{ "CMPB",	T_6809_CMPB },
	{ "CMPD",	T_6809_CMPD },
	{ "CMPX",	T_6809_CMPX },
	{ "CMPY",	T_6809_CMPY },
	{ "CMPU",	T_6809_CMPU },
	{ "CMPS",	T_6809_CMPS },

	{ "COM",	T_6809_COM },
	{ "COMA",	T_6809_COMA },
	{ "COMB",	T_6809_COMB },
	{ "CWAI",	T_6809_CWAI },
	{ "DAA",	T_6809_DAA },

	{ "A",		T_6809_REG_A },
	{ "B",		T_6809_REG_B },
	{ "D",		T_6809_REG_D },
	{ "PCR",	T_6809_REG_PCR },
	{ "X",		T_6809_REG_X },
	{ "Y",		T_6809_REG_Y },
	{ "U",		T_6809_REG_U },
	{ "S",		T_6809_REG_S },

	{ NULL, 0 }
};

void
m6809_DefineTokens(void) {
	lex_ConstantsDefineWords(g_tokens);
}

