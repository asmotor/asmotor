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

#include "errors.h"
#include "lexer.h"
#include "options.h"
#include "parse.h"
#include "section.h"
#include "types.h"

#include "x65_errors.h"
#include "x65_options.h"
#include "x65_parse.h"
#include "x65_tokens.h"

typedef struct Parser {
    uint8_t baseOpcode;
    uint32_t allowedModes;
	EImmediateSize immSize;
    bool (* handler)(uint8_t baseOpcode, SAddressingMode* addrMode);
} SParser;


static bool
handle_Implicit(uint8_t baseOpcode, SAddressingMode* addrMode) {
	sect_OutputConst8(baseOpcode);
	return true;
}
	

static bool
handle_LongBranch(uint8_t baseOpcode, SAddressingMode* addrMode) {
    sect_OutputConst8(baseOpcode);

    SExpression* expression = expr_PcRelative(addrMode->expr, -2);
    expression = expr_CheckRange(expression, -32768, 32767);
    if (expression == NULL) {
        err_Error(ERROR_OPERAND_RANGE);
        return true;
    } else {
        sect_OutputExpr16(expression);
    }

    return true;
}


static SParser g_instructionHandlers[T_4510_TYS - T_4510_CLE + 1] = {
	{ 0x02, MODE_NONE, IMM_NONE, handle_Implicit },		/* CLE */
    { 0x1B, MODE_NONE, IMM_NONE, handle_Implicit },		/* INZ */
    { 0x13, MODE_ABS,  IMM_NONE, handle_LongBranch },	/* LBPL */
	{ 0x03, MODE_NONE, IMM_NONE, handle_Implicit },		/* SEE */
	{ 0x0B, MODE_NONE, IMM_NONE, handle_Implicit },		/* TSY */
	{ 0x2B, MODE_NONE, IMM_NONE, handle_Implicit },		/* TYS */
};


bool
x65_Parse4510Instruction(void) {
	if (T_4510_CLE <= lex_Context->token.id && lex_Context->token.id <= T_4510_TYS) {
		if (opt_Current->machineOptions->cpu & (MOPT_CPU_4510 | MOPT_CPU_45GS02)) {
			SAddressingMode addrMode;
			ETargetToken token = (ETargetToken) lex_Context->token.id;
			SParser* handler = &g_instructionHandlers[token - T_4510_CLE];
			uint32_t allowedModes = handler->allowedModes;

			parse_GetToken();
			if (x65_ParseAddressingMode(&addrMode, allowedModes, handler->immSize) && (addrMode.mode & allowedModes))
				return handler->handler(handler->baseOpcode, &addrMode);
			else
				err_Error(MERROR_ILLEGAL_ADDRMODE);
		} else {
			err_Error(MERROR_INSTRUCTION_NOT_SUPPORTED);
		}
	}
	return false;
}
