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

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "errors.h"
#include "expression.h"
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
handle_Implicit(uint8_t baseOpcode, SAddressingMode* addrMode) {
	sect_OutputConst8(baseOpcode);
	return true;
}


static bool
handle_ASR(uint8_t baseOpcode, SAddressingMode* addrMode) {
	if (addrMode->mode == MODE_NONE || addrMode->mode == MODE_A) {
		sect_OutputConst8(baseOpcode);
	} else if (addrMode->mode == MODE_ABS) {
		baseOpcode += 0x01;
		sect_OutputConst8(baseOpcode);
		sect_OutputExpr16(addrMode->expr);
	} else {
		assert(false);
	}
	return true;
}
	

static bool
handle_ASW(uint8_t baseOpcode, SAddressingMode* addrMode) {
	if (addrMode->mode == MODE_ABS) {
		sect_OutputConst8(baseOpcode);
		sect_OutputExpr16(addrMode->expr);
	} else {
		assert(false);
	}
	return true;
}
	

static bool
handle_LDZ(uint8_t baseOpcode, SAddressingMode* addrMode) {
	switch (addrMode->mode) {
		case MODE_IMM:
			sect_OutputConst8(baseOpcode);
			x65_OutputSU8Expression(addrMode->expr);
			break;
		case MODE_ABS:
			sect_OutputConst8(baseOpcode | (uint8_t) 0x08);
			x65_OutputU16Expression(addrMode->expr);
			break;
		case MODE_ABS_X:
			sect_OutputConst8(baseOpcode | (uint8_t) 0x18);
			x65_OutputU16Expression(addrMode->expr);
			break;
		default:
			assert(false);
	}
	return true;
}
	

static bool
handle_CPZ(uint8_t baseOpcode, SAddressingMode* addrMode) {
	switch (addrMode->mode) {
		case MODE_IMM:
			sect_OutputConst8(baseOpcode);
			x65_OutputSU8Expression(addrMode->expr);
			break;
		case MODE_ZP:
			sect_OutputConst8(0xD4);
			x65_OutputSU8Expression(addrMode->expr);
			break;
		case MODE_ABS:
			sect_OutputConst8(0xDC);
			x65_OutputU16Expression(addrMode->expr);
			break;
		default:
			assert(false);
	}
	return true;
}
	

static bool
handle_INW_DEW(uint8_t baseOpcode, SAddressingMode* addrMode) {
	switch (addrMode->mode) {
		case MODE_ZP:
			sect_OutputConst8(baseOpcode);
			x65_OutputU8Expression(addrMode->expr);
			break;
		default:
			assert(false);
	}
	return true;
}
	

static bool
handle_PHW(uint8_t baseOpcode, SAddressingMode* addrMode) {
	switch (addrMode->mode) {
		case MODE_IMM:
			sect_OutputConst8(baseOpcode);
			x65_OutputU16Expression(addrMode->expr);
			break;
		case MODE_ABS:
			sect_OutputConst8(baseOpcode | 0x08);
			x65_OutputU16Expression(addrMode->expr);
			break;
		default:
			assert(false);
	}
	return true;
}
	

static bool
handle_ROW(uint8_t baseOpcode, SAddressingMode* addrMode) {
	switch (addrMode->mode) {
		case MODE_ABS:
			sect_OutputConst8(baseOpcode);
			x65_OutputU16Expression(addrMode->expr);
			break;
		default:
			assert(false);
	}
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


static SParser g_instructionHandlers[T_4510_TZA - T_4510_ASR + 1] = {
	{ 0x43, MODE_NONE | MODE_A | MODE_ABS, IMM_NONE, handle_ASR },			/* ASR */
	{ 0xCB, MODE_ABS, IMM_NONE, handle_ASW },			/* ASW */
	{ 0x02, MODE_NONE, IMM_NONE, handle_Implicit },		/* CLE */
	{ 0xC2, MODE_IMM | MODE_ZP | MODE_ABS, IMM_NONE, handle_CPZ },			/* CPZ */
	{ 0xC3, MODE_ZP, IMM_NONE, handle_INW_DEW },				/* DEW */
	{ 0x3B, MODE_NONE, IMM_NONE, handle_Implicit },		/* DEZ */
    { 0xE3, MODE_ZP, IMM_NONE, handle_INW_DEW },		/* INW */
    { 0x1B, MODE_NONE, IMM_NONE, handle_Implicit },		/* INZ */
    { 0x93, MODE_ABS,  IMM_NONE, handle_LongBranch },	/* LBCC */
    { 0xB3, MODE_ABS,  IMM_NONE, handle_LongBranch },	/* LBCS */
    { 0xF3, MODE_ABS,  IMM_NONE, handle_LongBranch },	/* LBEQ */
    { 0x33, MODE_ABS,  IMM_NONE, handle_LongBranch },	/* LBMI */
    { 0xD3, MODE_ABS,  IMM_NONE, handle_LongBranch },	/* LBNE */
    { 0x13, MODE_ABS,  IMM_NONE, handle_LongBranch },	/* LBPL */
    { 0x83, MODE_ABS,  IMM_NONE, handle_LongBranch },	/* LBRA */
    { 0x63, MODE_ABS,  IMM_NONE, handle_LongBranch },	/* LBSR */
    { 0x73, MODE_ABS,  IMM_NONE, handle_LongBranch },	/* LBVS */
    { 0xA3, MODE_IMM | MODE_ABS | MODE_ABS_X,  IMM_NONE, handle_LDZ },	/* LDZ */
	{ 0x5C, MODE_NONE, IMM_NONE, handle_Implicit },		/* MAP */
	{ 0x42, MODE_NONE, IMM_NONE, handle_Implicit },		/* NEG */
	{ 0xF4, MODE_IMM | MODE_ABS, IMM_NONE, handle_PHW },		/* PHW */
	{ 0xDB, MODE_NONE, IMM_NONE, handle_Implicit },		/* PHZ */
	{ 0xFB, MODE_NONE, IMM_NONE, handle_Implicit },		/* PLZ */
	{ 0xEB, MODE_ABS, IMM_NONE, handle_ROW },		/* ROW */
	{ 0x03, MODE_NONE, IMM_NONE, handle_Implicit },		/* SEE */
	{ 0x5B, MODE_NONE, IMM_NONE, handle_Implicit },		/* TAB */
	{ 0x4B, MODE_NONE, IMM_NONE, handle_Implicit },		/* TAZ */
	{ 0x7B, MODE_NONE, IMM_NONE, handle_Implicit },		/* TBA */
	{ 0x0B, MODE_NONE, IMM_NONE, handle_Implicit },		/* TSY */
	{ 0x2B, MODE_NONE, IMM_NONE, handle_Implicit },		/* TYS */
	{ 0x6B, MODE_NONE, IMM_NONE, handle_Implicit },		/* TZA */
};


bool
x65_Parse4510Instruction(void) {
	if (T_4510_ASR <= lex_Context->token.id && lex_Context->token.id <= T_4510_TZA) {
		if (opt_Current->machineOptions->cpu & (MOPT_CPU_4510 | MOPT_CPU_45GS02)) {
			SAddressingMode addrMode;
			ETargetToken token = (ETargetToken) lex_Context->token.id;
			SParser* handler = &g_instructionHandlers[token - T_4510_ASR];
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
