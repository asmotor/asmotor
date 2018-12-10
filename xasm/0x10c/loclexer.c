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
#include "localasm.h"
#include "lexer.h"
#include "loclexer.h"

static SLexerTokenDefinition localstrings[] =
{
	{ "add", T_0X10C_ADD },
	{ "and", T_0X10C_AND },
	{ "bor", T_0X10C_BOR },
	{ "div", T_0X10C_DIV },
	{ "ifb", T_0X10C_IFB },
	{ "ife", T_0X10C_IFE },
	{ "ifg", T_0X10C_IFG },
	{ "ifn", T_0X10C_IFN },
	{ "jsr", T_0X10C_JSR },
	{ "mod", T_0X10C_MOD },
	{ "mul", T_0X10C_MUL },
	{ "set", T_0X10C_SET },
	{ "shl", T_0X10C_SHL },
	{ "shr", T_0X10C_SHR },
	{ "sub", T_0X10C_SUB },
	{ "xor", T_0X10C_XOR },

	{ "a", T_REG_A },
	{ "b", T_REG_B },
	{ "c", T_REG_C },
	{ "x", T_REG_X },
	{ "y", T_REG_Y },
	{ "z", T_REG_Z },
	{ "i", T_REG_I },
	{ "j", T_REG_J },
	{ "pop", T_REG_POP },
	{ "[sp++]", T_REG_POP },
	{ "peek", T_REG_PEEK },
	{ "[sp]", T_REG_PEEK },
	{ "push", T_REG_PUSH },
	{ "[--sp]", T_REG_PEEK },
	{ "sp", T_REG_SP },
	{ "pc", T_REG_PC },
	{ "o", T_REG_O },

	{ NULL, 0 }
};

void loclexer_Init(void)
{
	lex_DefineTokens(localstrings);
}

