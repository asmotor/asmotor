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

#include "xasm.h"
#include "lexer_constants.h"

#include "0x10c_tokens.h"

static SLexConstantsWord
s_tokens[] = {
	{ "ADD", T_0X10C_ADD },
	{ "AND", T_0X10C_AND },
	{ "BOR", T_0X10C_BOR },
	{ "DIV", T_0X10C_DIV },
	{ "IFB", T_0X10C_IFB },
	{ "IFE", T_0X10C_IFE },
	{ "IFG", T_0X10C_IFG },
	{ "IFN", T_0X10C_IFN },
	{ "JSR", T_0X10C_JSR },
	{ "MOD", T_0X10C_MOD },
	{ "MUL", T_0X10C_MUL },
	// { "SET", T_SYM_SET },
	{ "SHL", T_0X10C_SHL },
	{ "SHR", T_0X10C_SHR },
	{ "SUB", T_0X10C_SUB },
	{ "XOR", T_0X10C_XOR },

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
loclexer_Init(void) {
	lex_ConstantsDefineWords(s_tokens);
}

