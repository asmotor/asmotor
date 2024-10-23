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
#include "lexer_context.h"
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
handle_PcRelative(uint8_t baseOpcode, SAddressingMode* addrMode) {
	sect_OutputConst8(baseOpcode);
	sect_OutputExpr16(expr_PcRelative(addrMode->expr, -1));
	return true;
}


static bool
handle_COP(uint8_t baseOpcode, SAddressingMode* addrMode) {
	sect_OutputConst8(baseOpcode);
	sect_OutputExpr8(expr_CheckRange(addrMode->expr, 0, 0xFF));
	return true;
}


static bool
handle_Jump(uint8_t baseOpcode, SAddressingMode* addrMode) {
	x65_OutputLongInstruction(baseOpcode, addrMode->expr);
	return true;
}


static bool
handle_MOVE(uint8_t baseOpcode, SAddressingMode* addrMode) {
	sect_OutputConst8(baseOpcode);
	sect_OutputExpr8(addrMode->expr);
	sect_OutputExpr8(addrMode->expr2);
	return true;
}


static bool
handle_Standard(uint8_t baseOpcode, SAddressingMode* addrMode) {
	switch (addrMode->mode) {
		case MODE_IMM:
			sect_OutputConst8(baseOpcode);
			sect_OutputExpr8(expr_CheckRange(addrMode->expr, -128, 0xFF));
			return true;
		case MODE_ABS:
		case MODE_IND_ABS:
			sect_OutputConst8(baseOpcode);
			x65_OutputU16Expression(expr_CheckRange(addrMode->expr, 0, 0xFFFF));
			return true;
		default:
			return false;
	}
}


static bool
handle_Implicit(uint8_t baseOpcode, SAddressingMode* addrMode) {
	sect_OutputConst8(baseOpcode);
	return true;
}
	

static SParser g_instructionHandlers[T_65816_XCE - T_65816_BRL + 1] = {
	{ 0x82, MODE_ABS, IMM_NONE, handle_PcRelative },	/* BRL */
	{ 0x02, MODE_IMM, IMM_8_BIT, handle_COP },	/* COP */
	{ 0x5C, MODE_ABS, IMM_NONE, handle_Jump },	/* JML */
	{ 0x22, MODE_ABS, IMM_NONE, handle_Jump },	/* JSL */
	{ 0x54, MODE_IMM_IMM, IMM_8_BIT, handle_MOVE },	/* MVN */
	{ 0x44, MODE_IMM_IMM, IMM_8_BIT, handle_MOVE },	/* MVP */
	{ 0xF4, MODE_ABS, IMM_NONE, handle_Standard },	/* PEA */
	{ 0xD4, MODE_IND_ABS, IMM_NONE, handle_Standard },	/* PEI */
	{ 0x62, MODE_ABS, IMM_NONE, handle_PcRelative },	/* PER */
	{ 0x8B, MODE_NONE, IMM_NONE, handle_Implicit },	/* PHB */
	{ 0x0B, MODE_NONE, IMM_NONE, handle_Implicit },	/* PHD */
	{ 0x4B, MODE_NONE, IMM_NONE, handle_Implicit },	/* PHK */
	{ 0xAB, MODE_NONE, IMM_NONE, handle_Implicit },	/* PLB */
	{ 0x2B, MODE_NONE, IMM_NONE, handle_Implicit },	/* PLD */
	{ 0xC2, MODE_IMM, IMM_8_BIT, handle_Standard },	/* REP */
	{ 0x6B, MODE_NONE, IMM_NONE, handle_Implicit },	/* RTL */
	{ 0xE2, MODE_IMM, IMM_8_BIT, handle_Standard },	/* SEP */
	{ 0x5B, MODE_NONE, IMM_NONE, handle_Implicit },	/* TCD */
	{ 0x1B, MODE_NONE, IMM_NONE, handle_Implicit },	/* TCS */
	{ 0x7B, MODE_NONE, IMM_NONE, handle_Implicit },	/* TDC */
	{ 0x3B, MODE_NONE, IMM_NONE, handle_Implicit },	/* TSC */
	{ 0x9B, MODE_NONE, IMM_NONE, handle_Implicit },	/* TXY */
	{ 0xBB, MODE_NONE, IMM_NONE, handle_Implicit },	/* TYX */
	{ 0x42, MODE_NONE, IMM_NONE, handle_Implicit },	/* WDM */
	{ 0xEB, MODE_NONE, IMM_NONE, handle_Implicit },	/* XBA */
	{ 0xFB, MODE_NONE, IMM_NONE, handle_Implicit },	/* XCE */
};


bool
x65_Parse65816Instruction(void) {
	if (T_65816_BRL <= lex_Context->token.id && lex_Context->token.id <= T_65816_XCE) {
		if (opt_Current->machineOptions->cpu & MOPT_CPU_65C816S) {
			SAddressingMode addrMode;
			ETargetToken token = (ETargetToken) lex_Context->token.id;
			SParser* handler = &g_instructionHandlers[token - T_65816_BRL];
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
