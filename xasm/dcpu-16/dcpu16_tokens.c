/*  Copyright 2008-2021 Carsten Elton Sorensen and contributors

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

#include "dcpu16_tokens.h"

static SLexConstantsWord g_tokens[] = {
	{ "ADD", T_DCPU16_ADD },
	{ "AND", T_DCPU16_AND },
	{ "BOR", T_DCPU16_BOR },
	{ "DIV", T_DCPU16_DIV },
	{ "IFB", T_DCPU16_IFB },
	{ "IFE", T_DCPU16_IFE },
	{ "IFG", T_DCPU16_IFG },
	{ "IFN", T_DCPU16_IFN },
	{ "JSR", T_DCPU16_JSR },
	{ "MOD", T_DCPU16_MOD },
	{ "MUL", T_DCPU16_MUL },
	// { "SET", T_SYM_SET },
	{ "SHL", T_DCPU16_SHL },
	{ "SHR", T_DCPU16_SHR },
	{ "SUB", T_DCPU16_SUB },
	{ "XOR", T_DCPU16_XOR },

	{ "A", T_REG_A },
	{ "B", T_REG_B },
	{ "C", T_REG_C },
	{ "X", T_REG_X },
	{ "Y", T_REG_Y },
	{ "Z", T_REG_Z },
	{ "I", T_REG_I },
	{ "J", T_REG_J },
	{ "POP", T_REG_POP },
	{ "[SP++]", T_REG_POP },
	{ "PEEK", T_REG_PEEK },
	{ "[SP]", T_REG_PEEK },
	{ "PUSH", T_REG_PUSH },
	{ "[--SP]", T_REG_PEEK },
	{ "SP", T_REG_SP },
	{ "PC", T_REG_PC },
	{ "O", T_REG_O },

	{ NULL, 0 }
};

void
x10c_DefineTokens(void) {
	lex_ConstantsDefineWords(g_tokens);
}

