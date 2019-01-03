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

#include "localasm.h"

static SLexConstantsWord localstrings[]=
{
	/* reg */
	{ "bcd", T_CHIP_BCD },
	{ "ldf", T_CHIP_LDF },
	{ "ldf10", T_CHIP_LDF10 },
	{ "shl", T_CHIP_SHL },
	{ "sknp", T_CHIP_SKNP },
	{ "skp", T_CHIP_SKP },
	{ "shr", T_CHIP_SHR },
	{ "wkp", T_CHIP_WKP },
	{ "and", T_CHIP_AND },
	{ "or", T_CHIP_OR },
	{ "sub", T_CHIP_SUB },
	{ "subn", T_CHIP_SUBN },
	{ "xor", T_CHIP_XOR },
	{ "drw", T_CHIP_DRW },
	{ "ld", T_CHIP_LD },
	{ "ldm", T_CHIP_LDM },
	{ "add", T_CHIP_ADD },
	{ "se", T_CHIP_SE },
	{ "sne", T_CHIP_SNE },
	{ "rnd", T_CHIP_RND },
	{ "scd", T_CHIP_SCRD },
	{ "jp", T_CHIP_JP },
	{ "call", T_CHIP_CALL },
	{ "cls", T_CHIP_CLS },
	{ "exit", T_CHIP_EXIT },
	{ "low", T_CHIP_LO },
	{ "high", T_CHIP_HI },
	{ "ret", T_CHIP_RET },
	{ "scr", T_CHIP_SCRR },
	{ "scl", T_CHIP_SCRL },

	/* Registers */
	{ "v0", T_CHIP_REG_V0 },
	{ "v1", T_CHIP_REG_V1 },
	{ "v2", T_CHIP_REG_V2 },
	{ "v3", T_CHIP_REG_V3 },
	{ "v4", T_CHIP_REG_V4 },
	{ "v5", T_CHIP_REG_V5 },
	{ "v6", T_CHIP_REG_V6 },
	{ "v7", T_CHIP_REG_V7 },
	{ "v8", T_CHIP_REG_V8 },
	{ "v9", T_CHIP_REG_V9 },
	{ "v10", T_CHIP_REG_V10 },
	{ "v11", T_CHIP_REG_V11 },
	{ "v12", T_CHIP_REG_V12 },
	{ "v13", T_CHIP_REG_V13 },
	{ "v14", T_CHIP_REG_V14 },
	{ "v15", T_CHIP_REG_V15 },
	{ "va", T_CHIP_REG_V10 },
	{ "vb", T_CHIP_REG_V11 },
	{ "vc", T_CHIP_REG_V12 },
	{ "vd", T_CHIP_REG_V13 },
	{ "ve", T_CHIP_REG_V14 },
	{ "vf", T_CHIP_REG_V15 },
	{ "dt", T_CHIP_REG_DT },
	{ "st", T_CHIP_REG_ST },
	{ "i", T_CHIP_REG_I },
	{ "(i)", T_CHIP_REG_I_IND },
	{ "rpl", T_CHIP_REG_RPL },

	{ NULL, 0 }
};

void	loclexer_Init(void)
{
	lex_ConstantsDefineWords(localstrings);
}
