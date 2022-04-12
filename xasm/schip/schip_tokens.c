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

#include "schip_tokens.h"

static SLexConstantsWord g_tokens[] = {
	/* reg */
	{ "BCD", T_CHIP_BCD },
	{ "LDF", T_CHIP_LDF },
	{ "LDF10", T_CHIP_LDF10 },
	{ "SHL", T_CHIP_SHL },
	{ "SKNP", T_CHIP_SKNP },
	{ "SKP", T_CHIP_SKP },
	{ "SHR", T_CHIP_SHR },
	{ "WKP", T_CHIP_WKP },
	{ "AND", T_CHIP_AND },
	{ "OR", T_CHIP_OR },
	{ "SUB", T_CHIP_SUB },
	{ "SUBN", T_CHIP_SUBN },
	{ "XOR", T_CHIP_XOR },
	{ "DRW", T_CHIP_DRW },
	{ "LD", T_CHIP_LD },
	{ "LDM", T_CHIP_LDM },
	{ "ADD", T_CHIP_ADD },
	{ "SE", T_CHIP_SE },
	{ "SNE", T_CHIP_SNE },
	{ "RND", T_CHIP_RND },
	{ "SCD", T_CHIP_SCRD },
	{ "JP", T_CHIP_JP },
	{ "CALL", T_CHIP_CALL },
	{ "CLS", T_CHIP_CLS },
	{ "EXIT", T_CHIP_EXIT },
	{ "LOW", T_CHIP_LO },
	{ "HIGH", T_CHIP_HI },
	{ "RET", T_CHIP_RET },
	{ "SCR", T_CHIP_SCRR },
	{ "SCL", T_CHIP_SCRL },

	/* REGISTERS */
	{ "V0", T_CHIP_REG_V0 },
	{ "V1", T_CHIP_REG_V1 },
	{ "V2", T_CHIP_REG_V2 },
	{ "V3", T_CHIP_REG_V3 },
	{ "V4", T_CHIP_REG_V4 },
	{ "V5", T_CHIP_REG_V5 },
	{ "V6", T_CHIP_REG_V6 },
	{ "V7", T_CHIP_REG_V7 },
	{ "V8", T_CHIP_REG_V8 },
	{ "V9", T_CHIP_REG_V9 },
	{ "V10", T_CHIP_REG_V10 },
	{ "V11", T_CHIP_REG_V11 },
	{ "V12", T_CHIP_REG_V12 },
	{ "V13", T_CHIP_REG_V13 },
	{ "V14", T_CHIP_REG_V14 },
	{ "V15", T_CHIP_REG_V15 },
	{ "VA", T_CHIP_REG_V10 },
	{ "VB", T_CHIP_REG_V11 },
	{ "VC", T_CHIP_REG_V12 },
	{ "VD", T_CHIP_REG_V13 },
	{ "VE", T_CHIP_REG_V14 },
	{ "VF", T_CHIP_REG_V15 },
	{ "DT", T_CHIP_REG_DT },
	{ "ST", T_CHIP_REG_ST },
	{ "I", T_CHIP_REG_I },
	{ "(I)", T_CHIP_REG_I_IND },
	{ "RPL", T_CHIP_REG_RPL },

	{ NULL, 0 }
};

void 
schip_DefineTokens(void) {
	lex_ConstantsDefineWords(g_tokens);
}
