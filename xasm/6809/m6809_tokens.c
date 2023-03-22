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
	{ "NOP",	T_6809_NOP },

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

