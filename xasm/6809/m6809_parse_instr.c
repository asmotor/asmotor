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

#include "expression.h"
#include "lexer.h"
#include "parse.h"
#include "errors.h"

#include "m6809_errors.h"
#include "m6809_parse.h"
#include "m6809_tokens.h"

#define MODE_P	(MODE_IMMEDIATE | MODE_ADDRESS | MODE_DIRECT | MODE_EXTENDED | MODE_ALL_INDEXED)

uint16_t g_dp_base = 0x0000;

typedef struct Parser {
    uint8_t baseOpcode;
    uint32_t allowedModes;
    bool (*handler)(uint8_t baseOpcode, SAddressingMode* addrMode);
} SParser;

static bool
emitOpcodeP(uint8_t baseOpcode, SAddressingMode* addrMode) {
	if (addrMode->mode == MODE_DIRECT) {
		sect_OutputConst8(baseOpcode | 0x10);
		sect_OutputExpr8(addrMode->expr);
		return true;
	} else if (addrMode->mode == MODE_EXTENDED) {
		sect_OutputConst8(baseOpcode | 0x30);
		sect_OutputExpr16(addrMode->expr);
		return true;
	} else if (addrMode->mode & MODE_ALL_INDEXED) {
		sect_OutputConst8(baseOpcode | 0x20);
		sect_OutputConst8(addrMode->indexed_post_byte);
		if (addrMode->mode & MODE_INDEXED_R_8BIT) {
			sect_OutputExpr8(addrMode->expr);
			return true;
		} else if (addrMode->mode & (MODE_INDEXED_R_16BIT | MODE_EXTENDED_INDIRECT)) {
			sect_OutputExpr16(addrMode->expr);
			return true;
		} else if (addrMode->mode & MODE_INDEXED_PC_8BIT) {
			sect_OutputExpr8(expr_PcRelative(addrMode->expr, -1));
			return true;
		} else if (addrMode->mode & MODE_INDEXED_PC_16BIT) {
			sect_OutputExpr16(expr_PcRelative(addrMode->expr, -2));
			return true;
		}
	}
	return false;
}

static bool
handleOpcodeP8(uint8_t baseOpcode, SAddressingMode* addrMode) {
	if (addrMode->mode == MODE_IMMEDIATE) {
		sect_OutputConst8(baseOpcode);
		sect_OutputExpr8(addrMode->expr);
		return true;
	}
	
	return emitOpcodeP(baseOpcode, addrMode);
}

static bool
handleOpcodeP16(uint8_t baseOpcode, SAddressingMode* addrMode) {
	if (addrMode->mode == MODE_IMMEDIATE) {
		sect_OutputConst8(baseOpcode);
		sect_OutputExpr16(addrMode->expr);
		return true;
	}
	
	return emitOpcodeP(baseOpcode, addrMode);
}

static bool
handleImplied(uint8_t baseOpcode, SAddressingMode* addrMode) {
    sect_OutputConst8(baseOpcode);
    return true;
}

static bool
handleANDCC(uint8_t baseOpcode, SAddressingMode* addrMode) {
    sect_OutputConst8(baseOpcode);
	sect_OutputExpr8(addrMode->expr);
    return true;
}

static SParser g_instructionHandlers[T_6809_NOP - T_6809_ABX + 1] = {
    { 0x3A, MODE_NONE, handleImplied },	/* ABX */
    { 0x89, MODE_P, handleOpcodeP8 },	/* ADCA */
    { 0xC9, MODE_P, handleOpcodeP8 },	/* ADCB */
    { 0x8B, MODE_P, handleOpcodeP8 },	/* ADDA */
    { 0xCB, MODE_P, handleOpcodeP8 },	/* ADDB */
    { 0xC3, MODE_P, handleOpcodeP16 },	/* ADDD */
    { 0x84, MODE_P, handleOpcodeP8 },	/* ANDA */
    { 0xC4, MODE_P, handleOpcodeP8 },	/* ANDB */
    { 0x1C, MODE_IMMEDIATE, handleANDCC },	/* ANDCC */
    { 0x12, MODE_NONE, handleImplied }, /* NOP */
};

bool
m6809_ParseIntegerInstruction(void) {
    if (T_6809_ABX <= lex_Context->token.id && lex_Context->token.id <= T_6809_NOP) {
        SAddressingMode addrMode;
        ETargetToken token = (ETargetToken) lex_Context->token.id;
        SParser* handler = &g_instructionHandlers[token - T_6809_ABX];

        parse_GetToken();
        if (m6809_ParseAddressingMode(&addrMode, handler->allowedModes)) {
			if (addrMode.mode == MODE_ADDRESS) {
				if (expr_IsConstant(addrMode.expr) && (addrMode.expr->value.integer & 0xFF00) == g_dp_base) {
					addrMode.mode = MODE_DIRECT;
				} else {
					addrMode.mode = MODE_EXTENDED;
				}
			}
            return handler->handler(handler->baseOpcode, &addrMode);
		} else {
            err_Error(MERROR_ILLEGAL_ADDRMODE);
		}
    }

    return false;
}
