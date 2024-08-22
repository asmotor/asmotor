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

#include "x65_errors.h"
#include "x65_options.h"
#include "x65_parse.h"
#include "x65_tokens.h"

typedef struct Parser {
	uint8_t baseOpcode;
	ECpu6502 cpu;
	EImmediateSize immSize;
	uint32_t allowedModes;
	bool (* handler)(uint8_t baseOpcode, SAddressingMode* addrMode);
} SParser;

#define CPU_6502 (MOPT_CPU_6502 | MOPT_CPU_65C02 | MOPT_CPU_65C02S | MOPT_CPU_65C816S | MOPT_CPU_4510 | MOPT_CPU_45GS02)
#define CPU_65C02 (MOPT_CPU_65C02 | MOPT_CPU_65C02S | MOPT_CPU_65C816S | MOPT_CPU_4510 | MOPT_CPU_45GS02)
#define CPU_65C02S (MOPT_CPU_65C02S | MOPT_CPU_4510 | MOPT_CPU_45GS02)
#define CPU_65C816S (MOPT_CPU_65C816S)
#define CPU_4510 (MOPT_CPU_4510 | MOPT_CPU_45GS02)

extern void
x65_OutputSU16Expression(SExpression* expr) {
	expr = expr_CheckRange(expr, -32768, 65535);
	if (expr == NULL)
		err_Error(ERROR_OPERAND_RANGE);
	expr = expr_And(expr, expr_Const(0xFFFF));

	sect_OutputExpr16(expr);
}

extern void
x65_OutputU16Expression(SExpression* expr) {
	expr = expr_CheckRange(expr, 0, 65535);
	if (expr == NULL)
		err_Error(ERROR_OPERAND_RANGE);
	expr = expr_And(expr, expr_Const(0xFFFF));

	sect_OutputExpr16(expr);
}

extern void
x65_OutputSU8Expression(SExpression* expr) {
	expr = expr_CheckRange(expr, -128, 255);
	if (expr == NULL)
		err_Error(ERROR_OPERAND_RANGE);
	expr = expr_And(expr, expr_Const(0xFF));

	sect_OutputExpr8(expr);
}

extern void
x65_OutputU8Expression(SExpression* expr) {
	expr = expr_CheckRange(expr, 0, 255);
	if (expr == NULL)
		err_Error(ERROR_OPERAND_RANGE);

	sect_OutputExpr8(expr);
}


extern void
x65_OutputLongInstruction(uint8_t opcode, SExpression* expr) {
	sect_OutputExpr32(expr_Or(expr_Asl(expr, expr_Const(8)), expr_Const(opcode)));
}


static bool
handleStandardAll(uint8_t baseOpcode, SAddressingMode* addrMode) {
	switch (addrMode->mode) {
		case MODE_IND_ZP_X:
			sect_OutputConst8(baseOpcode | (uint8_t) 0x00);
			x65_OutputU8Expression(addrMode->expr);
			return true;
		case MODE_816_DISP_S:
			sect_OutputConst8(baseOpcode | (uint8_t) 0x02);
			x65_OutputU8Expression(addrMode->expr);
			return true;
		case MODE_ZP:
			sect_OutputConst8(baseOpcode | (uint8_t) 0x04);
			x65_OutputU8Expression(addrMode->expr);
			return true;
		case MODE_816_LONG_IND_ZP:
			sect_OutputConst8(baseOpcode | (uint8_t) 0x06);
			x65_OutputU8Expression(addrMode->expr);
			return true;
		case MODE_IMM:
			sect_OutputConst8(baseOpcode | (uint8_t) 0x08);
			if (opt_Current->machineOptions->m16)
				x65_OutputSU16Expression(addrMode->expr);
			else
				x65_OutputSU8Expression(addrMode->expr);
			return true;
		case MODE_ABS:
			sect_OutputConst8(baseOpcode | (uint8_t) 0x0C);
			sect_OutputExpr16(addrMode->expr);
			return true;
		case MODE_816_LONG_ABS:
			x65_OutputLongInstruction(baseOpcode | (uint8_t) 0x0E, addrMode->expr);
			return true;
		case MODE_IND_ZP_Y:
			sect_OutputConst8(baseOpcode | (uint8_t) 0x10);
			x65_OutputU8Expression(addrMode->expr);
			return true;
		case MODE_IND_ZP:
			sect_OutputConst8((baseOpcode + 1) | (uint8_t) 0x10);
			x65_OutputU8Expression(addrMode->expr);
			return true;
		case MODE_4510_IND_ZP_Z:
			sect_OutputConst8(baseOpcode + (uint8_t) 0x11);
			x65_OutputSU8Expression(addrMode->expr);
			return true;
		case MODE_816_IND_DISP_S_Y: {
			if ((opt_Current->machineOptions->cpu & CPU_4510) && baseOpcode == 0x81)
				baseOpcode = 0x82;
			else if ((opt_Current->machineOptions->cpu & CPU_4510) && baseOpcode == 0xA1)
				baseOpcode = 0xE2;
			else
				baseOpcode |= 0x12;
			sect_OutputConst8(baseOpcode);
			x65_OutputU8Expression(addrMode->expr);
			return true;
		}
		case MODE_ZP_X:
		case MODE_ZP_Y:
			sect_OutputConst8(baseOpcode | (uint8_t) 0x14);
			x65_OutputU8Expression(addrMode->expr);
			return true;
		case MODE_816_LONG_IND_ZP_Y:
			sect_OutputConst8(baseOpcode | (uint8_t) 0x16);
			x65_OutputU8Expression(addrMode->expr);
			return true;
		case MODE_ABS_Y:
			sect_OutputConst8(baseOpcode | (uint8_t) 0x18);
			sect_OutputExpr16(addrMode->expr);
			return true;
		case MODE_ABS_X:
			sect_OutputConst8(baseOpcode | (uint8_t) 0x1C);
			sect_OutputExpr16(addrMode->expr);
			return true;
		case MODE_816_LONG_ABS_X:
			x65_OutputLongInstruction(baseOpcode | (uint8_t) 0x1E, addrMode->expr);
			return true;
		default:
			err_Error(MERROR_ILLEGAL_ADDRMODE);
			return true;
	}

}

static bool
handleStandardAbsY7(uint8_t baseOpcode, SAddressingMode* addrMode) {
	switch (addrMode->mode) {
		case MODE_IND_ZP_X:
			sect_OutputConst8(baseOpcode | (uint8_t) (0 << 2));
			x65_OutputU8Expression(addrMode->expr);
			return true;
		case MODE_IMM:
			sect_OutputConst8(baseOpcode | (uint8_t) (2 << 2));
			x65_OutputSU8Expression(addrMode->expr);
			return true;
		case MODE_IND_ZP_Y:
			sect_OutputConst8(baseOpcode | (uint8_t) (4 << 2));
			x65_OutputU8Expression(addrMode->expr);
			return true;
		case MODE_ABS_Y:
			sect_OutputConst8(baseOpcode | (uint8_t) (7 << 2));
			sect_OutputExpr16(addrMode->expr);
			return true;
		case MODE_ZP:
			sect_OutputConst8(baseOpcode | (uint8_t) (1 << 2));
			x65_OutputU8Expression(addrMode->expr);
			return true;
		case MODE_ABS:
			sect_OutputConst8(baseOpcode | (uint8_t) (3 << 2));
			sect_OutputExpr16(addrMode->expr);
			return true;
		case MODE_ZP_X:
		case MODE_ZP_Y:
			sect_OutputConst8(baseOpcode | (uint8_t) (5 << 2));
			x65_OutputU8Expression(addrMode->expr);
			return true;
		default:
			err_Error(MERROR_ILLEGAL_ADDRMODE);
			return true;
	}
}

static bool
handleStandardImm0(uint8_t baseOpcode, SAddressingMode* addrMode) {
	switch (addrMode->mode) {
		case MODE_IMM:
			sect_OutputConst8(baseOpcode | (uint8_t) (0 << 2));
			if (opt_Current->machineOptions->x16)
				x65_OutputSU16Expression(addrMode->expr);
			else
				x65_OutputSU8Expression(addrMode->expr);
			return true;
		case MODE_ZP:
			sect_OutputConst8(baseOpcode | (uint8_t) (1 << 2));
			x65_OutputU8Expression(addrMode->expr);
			return true;
		case MODE_ABS:
			sect_OutputConst8(baseOpcode | (uint8_t) (3 << 2));
			sect_OutputExpr16(addrMode->expr);
			return true;
		case MODE_ZP_X:
		case MODE_ZP_Y:
			sect_OutputConst8(baseOpcode | (uint8_t) (5 << 2));
			x65_OutputU8Expression(addrMode->expr);
			return true;
		case MODE_4510_ABS_X:
		case MODE_4510_ABS_Y:
			baseOpcode = ((baseOpcode & 0x02) << 3) | (baseOpcode & 0x80) | 0x0B;
			sect_OutputConst8(baseOpcode);
			sect_OutputExpr16(addrMode->expr);
			return true;
		case MODE_ABS_X:
		case MODE_ABS_Y:
			sect_OutputConst8(baseOpcode | (uint8_t) (7 << 2));
			sect_OutputExpr16(addrMode->expr);
			return true;
		default:
			err_Error(MERROR_ILLEGAL_ADDRMODE);
			return true;
	}
}

static bool
handleStandardRotate(uint8_t baseOpcode, SAddressingMode* addrMode) {
	switch (addrMode->mode) {
		case MODE_IMM: {
			if (opt_Current->machineOptions->synthesized) {
				if (expr_IsConstant(addrMode->expr)) {
					for (int i = 0; i < addrMode->expr->value.integer; ++i)
						sect_OutputConst8(baseOpcode | (uint8_t) (2 << 2));
				} else {
					err_Error(ERROR_EXPR_CONST);
				}
			} else {
				err_Error(MERROR_SYNTHESIZED);
			}
			return true;
		}
		case MODE_A:
			sect_OutputConst8(baseOpcode | (uint8_t) (2 << 2));
			return true;
		case MODE_ZP:
			sect_OutputConst8(baseOpcode | (uint8_t) (1 << 2));
			x65_OutputU8Expression(addrMode->expr);
			return true;
		case MODE_ABS:
			sect_OutputConst8(baseOpcode | (uint8_t) (3 << 2));
			sect_OutputExpr16(addrMode->expr);
			return true;
		case MODE_ZP_X:
			sect_OutputConst8(baseOpcode | (uint8_t) (5 << 2));
			x65_OutputU8Expression(addrMode->expr);
			return true;
		case MODE_ABS_X:
			sect_OutputConst8(baseOpcode | (uint8_t) (7 << 2));
			sect_OutputExpr16(addrMode->expr);
			return true;
		default:
			err_Error(MERROR_ILLEGAL_ADDRMODE);
			return true;
	}
}

static bool
handleBranch(uint8_t baseOpcode, SAddressingMode* addrMode) {
	sect_OutputConst8(baseOpcode);

	SExpression* expression = expr_PcRelative(addrMode->expr, -1);
	expression = expr_CheckRange(expression, -128, 127);
	if (expression == NULL) {
		err_Error(ERROR_OPERAND_RANGE);
		return true;
	} else {
		sect_OutputExpr8(expression);
	}

	return true;
}

static bool
handleBIT(uint8_t baseOpcode, SAddressingMode* addrMode) {
	if ((addrMode->mode & (MODE_IMM | MODE_ZP_X | MODE_ABS_X)) && opt_Current->machineOptions->cpu == MOPT_CPU_6502) {
		err_Error(MERROR_INSTRUCTION_NOT_SUPPORTED);
		return true;
	}

	if (addrMode->mode == MODE_IMM) {
		sect_OutputConst8(0x89);
		if (opt_Current->machineOptions->m16)
			x65_OutputU16Expression(addrMode->expr);
		else
			x65_OutputU8Expression(addrMode->expr);
		return true;
	}

	return handleStandardAll(baseOpcode, addrMode);
}

static bool
handleINCDEC(uint8_t baseOpcode, SAddressingMode* addrMode) {
	if ((addrMode->mode & MODE_A) && opt_Current->machineOptions->cpu == MOPT_CPU_6502) {
		err_Error(MERROR_INSTRUCTION_NOT_SUPPORTED);
		return true;
	}

	if (addrMode->mode == MODE_A) {
		/* Convert base opcode to INC/DEC A which live in a completely place in the matrix */
		baseOpcode = (baseOpcode & 0x3F) ^ 0x38;
		sect_OutputConst8(baseOpcode);
		return true;
	}

	return handleStandardAll(baseOpcode, addrMode);
}

static bool
handleImplied(uint8_t baseOpcode, SAddressingMode* addrMode) {
	sect_OutputConst8(baseOpcode);
	return true;
}

static bool
handleJMP(uint8_t baseOpcode, SAddressingMode* addrMode) {
	if (addrMode->mode == MODE_816_LONG_ABS) {
		x65_OutputLongInstruction(baseOpcode + 0x10, addrMode->expr);
		return true;
	}

	if (addrMode->mode == MODE_IND_ABS_X) {
		if (opt_Current->machineOptions->cpu == MOPT_CPU_6502) {
			err_Error(MERROR_INSTRUCTION_NOT_SUPPORTED);
			return true;
		}
		baseOpcode += 0x30;
	} else if (addrMode->mode == MODE_IND_ABS) {
		baseOpcode += 0x20;
	} else if (addrMode->mode == MODE_816_LONG_IND_ABS) {
		baseOpcode += 0x90;
	}

	sect_OutputConst8(baseOpcode);
	sect_OutputExpr16(addrMode->expr);
	return true;
}

static bool
handleJSR(uint8_t baseOpcode, SAddressingMode* addrMode) {
	if (addrMode->mode == MODE_816_LONG_ABS) {
		x65_OutputLongInstruction(0x22, addrMode->expr);
		return true;
	} else if (addrMode->mode == MODE_IND_ABS_X) {
		if (opt_Current->machineOptions->cpu & CPU_65C816S) {
			baseOpcode = 0xFC;
		} else if (opt_Current->machineOptions->cpu & CPU_4510) {
			baseOpcode = 0x23;
		} else {
			err_Error(MERROR_INSTRUCTION_NOT_SUPPORTED);
			return false;
		}
	} else if (addrMode->mode == MODE_IND_ABS) {
		if ((opt_Current->machineOptions->cpu & CPU_4510) == 0) {
			err_Error(MERROR_INSTRUCTION_NOT_SUPPORTED);
			return false;
		}
		baseOpcode = 0x22;
	}

	sect_OutputConst8(baseOpcode);
	sect_OutputExpr16(addrMode->expr);
	return true;
}

static bool
handleBRK(uint8_t baseOpcode, SAddressingMode* addrMode) {
	sect_OutputConst8(baseOpcode);
	if (addrMode->mode == MODE_IMM)
		x65_OutputSU8Expression(addrMode->expr);
	return true;
}

static bool
handleDOP(uint8_t baseOpcode, SAddressingMode* addrMode) {
	switch (addrMode->mode) {
		case MODE_IMM:
			sect_OutputConst8(0x80);
			x65_OutputSU8Expression(addrMode->expr);
			return true;
		case MODE_ZP:
			sect_OutputConst8(0x04);
			x65_OutputU8Expression(addrMode->expr);
			return true;
		case MODE_ZP_X:
			sect_OutputConst8(0x14);
			x65_OutputU8Expression(addrMode->expr);
			return true;
		default:
			return false;
	}
}

static bool
handleRTS(uint8_t baseOpcode, SAddressingMode* addrMode) {
	switch (addrMode->mode) {
		case MODE_IMM:
			if (opt_Current->machineOptions->cpu & CPU_4510) {
				sect_OutputConst8(0x62);
				x65_OutputU8Expression(addrMode->expr);
				return true;
			} else {
				err_Error(MERROR_INSTRUCTION_NOT_SUPPORTED);
				return true;
			}
		case MODE_NONE:
			sect_OutputConst8(baseOpcode);
			return true;
		default:
			break;
	}
	return false;
}

static bool
handleSTZ(uint8_t baseOpcode, SAddressingMode* addrMode) {
	switch (addrMode->mode) {
		case MODE_ZP:
			sect_OutputConst8(0x64);
			x65_OutputU8Expression(addrMode->expr);
			return true;
		case MODE_ZP_X:
			sect_OutputConst8(0x74);
			x65_OutputU8Expression(addrMode->expr);
			return true;
		case MODE_ABS:
			sect_OutputConst8(0x9C);
			sect_OutputExpr16(addrMode->expr);
			return true;
		case MODE_ABS_X:
			sect_OutputConst8(0x9E);
			sect_OutputExpr16(addrMode->expr);
			return true;
		default:
			return false;
	}
}

static bool
handleBITBranch_C02(uint8_t baseOpcode, SAddressingMode* addrMode) {
	SExpression* opcode;
	opcode = expr_Or(expr_Const(baseOpcode), expr_Asl(expr_CheckRange(addrMode->expr, 0, 7), expr_Const(4)));

	sect_OutputExpr8(opcode);
	x65_OutputU8Expression(addrMode->expr2);

	SExpression* expression = expr_PcRelative(addrMode->expr3, -1);
	expression = expr_CheckRange(expression, -128, 127);
	if (expression == NULL) {
		err_Error(ERROR_OPERAND_RANGE);
	} else {
		sect_OutputExpr8(expression);
	}

	return true;
}

static bool
handleBITxBranch_C02(uint8_t baseOpcode, SAddressingMode* addrMode) {
	sect_OutputConst8(baseOpcode);
	x65_OutputU8Expression(addrMode->expr);
	SExpression* expression = expr_PcRelative(addrMode->expr2, -1);
	expression = expr_CheckRange(expression, -128, 127);
	if (expression == NULL) {
		err_Error(ERROR_OPERAND_RANGE);
	} else {
		sect_OutputExpr8(expression);
	}

	return true;
}

static bool
handleBIT_C02(uint8_t baseOpcode, SAddressingMode* addrMode) {
	SExpression* opcode;
	opcode = expr_Or(expr_Const(baseOpcode), expr_Asl(expr_CheckRange(addrMode->expr, 0, 7), expr_Const(4)));

	sect_OutputExpr8(opcode);
	x65_OutputU8Expression(addrMode->expr2);
	return true;
}

static bool
handleBITx_C02(uint8_t baseOpcode, SAddressingMode* addrMode) {
	sect_OutputConst8(baseOpcode);
	x65_OutputU8Expression(addrMode->expr);
	return true;
}


static SParser g_instructionHandlers[T_65C02_SMB7 - T_6502_ADC + 1] = {
	{ 0x61, CPU_6502, IMM_ACC, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_ZP_X | MODE_IND_ZP_Y | MODE_IND_ZP | MODE_816_DISP_S | MODE_816_LONG_IND_ZP | MODE_816_LONG_ABS | MODE_816_IND_DISP_S_Y | MODE_816_LONG_IND_ZP_Y | MODE_816_LONG_ABS_X | MODE_4510_IND_ZP_Z, handleStandardAll },	/* ADC */
	{ 0x21, CPU_6502, IMM_ACC, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_ZP_X | MODE_IND_ZP_Y | MODE_IND_ZP | MODE_816_DISP_S | MODE_816_LONG_IND_ZP | MODE_816_LONG_ABS | MODE_816_IND_DISP_S_Y | MODE_816_LONG_IND_ZP_Y | MODE_816_LONG_ABS_X | MODE_4510_IND_ZP_Z, handleStandardAll },	/* AND */
	{ 0x02, CPU_6502, IMM_8_BIT, MODE_IMM | MODE_A | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X, handleStandardRotate },	/* ASL */
	{ 0x20, CPU_6502, IMM_ACC, MODE_ZP | MODE_ABS | MODE_IMM | MODE_ZP_X | MODE_ABS_X, handleBIT },	/* BIT */
	{ 0x10, CPU_6502, IMM_NONE, MODE_ABS, handleBranch },	/* BPL */
	{ 0x30, CPU_6502, IMM_NONE, MODE_ABS, handleBranch },	/* BMI */
	{ 0x50, CPU_6502, IMM_NONE, MODE_ABS, handleBranch },	/* BVC */
	{ 0x70, CPU_6502, IMM_NONE, MODE_ABS, handleBranch },	/* BVS */
	{ 0x90, CPU_6502, IMM_NONE, MODE_ABS, handleBranch },	/* BCC */
	{ 0xB0, CPU_6502, IMM_NONE, MODE_ABS, handleBranch },	/* BCS */
	{ 0xD0, CPU_6502, IMM_NONE, MODE_ABS, handleBranch },	/* BNE */
	{ 0xF0, CPU_6502, IMM_NONE, MODE_ABS, handleBranch },	/* BEQ */
	{ 0x00, CPU_6502, IMM_8_BIT, MODE_NONE | MODE_IMM, handleBRK },	/* BRK */
	{ 0xC1, CPU_6502, IMM_ACC, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_ZP_X | MODE_IND_ZP_Y | MODE_IND_ZP | MODE_816_DISP_S | MODE_816_LONG_IND_ZP | MODE_816_LONG_ABS | MODE_816_IND_DISP_S_Y | MODE_816_LONG_IND_ZP_Y | MODE_816_LONG_ABS_X | MODE_4510_IND_ZP_Z, handleStandardAll },	/* CMP */
	{ 0xE0, CPU_6502, IMM_INDEX, MODE_IMM | MODE_ZP | MODE_ABS, handleStandardImm0 },	/* CPX */
	{ 0xC0, CPU_6502, IMM_INDEX, MODE_IMM | MODE_ZP | MODE_ABS, handleStandardImm0 },	/* CPY */
	{ 0xC2, CPU_6502, IMM_NONE, MODE_ZP | MODE_ABS | MODE_ZP_X | MODE_ABS_X | MODE_A, handleINCDEC },	/* DEC */
	{ 0x41, CPU_6502, IMM_ACC, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_ZP_X | MODE_IND_ZP_Y | MODE_IND_ZP | MODE_816_DISP_S | MODE_816_LONG_IND_ZP | MODE_816_LONG_ABS | MODE_816_IND_DISP_S_Y | MODE_816_LONG_IND_ZP_Y | MODE_816_LONG_ABS_X | MODE_4510_IND_ZP_Z, handleStandardAll },	/* EOR */
	{ 0x18, CPU_6502, IMM_NONE, MODE_NONE, handleImplied },	/* CLC */
	{ 0x38, CPU_6502, IMM_NONE, MODE_NONE, handleImplied },	/* SEC */
	{ 0x58, CPU_6502, IMM_NONE, MODE_NONE, handleImplied },	/* CLI */
	{ 0x78, CPU_6502, IMM_NONE, MODE_NONE, handleImplied },	/* SEI */
	{ 0xB8, CPU_6502, IMM_NONE, MODE_NONE, handleImplied },	/* CLV */
	{ 0xD8, CPU_6502, IMM_NONE, MODE_NONE, handleImplied },	/* CLD */
	{ 0xF8, CPU_6502, IMM_NONE, MODE_NONE, handleImplied },	/* SED */
	{ 0xE2, CPU_6502, IMM_NONE, MODE_ZP | MODE_ABS | MODE_ZP_X | MODE_ABS_X | MODE_A, handleINCDEC },	/* INC */
	{ 0x4C, CPU_6502, IMM_NONE, MODE_ABS | MODE_IND_ABS | MODE_IND_ABS_X | MODE_816_LONG_ABS | MODE_816_LONG_IND_ABS, handleJMP },	/* JMP */
	{ 0x20, CPU_6502, IMM_NONE, MODE_ABS | MODE_816_LONG_ABS | MODE_IND_ABS_X | MODE_IND_ABS | MODE_IND_ABS_X, handleJSR },	/* JSR */
	{ 0xA1, CPU_6502, IMM_ACC, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_ZP_X | MODE_IND_ZP_Y | MODE_IND_ZP | MODE_816_DISP_S | MODE_816_LONG_IND_ZP | MODE_816_LONG_ABS | MODE_816_IND_DISP_S_Y | MODE_816_LONG_IND_ZP_Y | MODE_816_LONG_ABS_X |  MODE_4510_IND_ZP_Z, handleStandardAll },	/* LDA */
	{ 0xA2, CPU_6502, IMM_INDEX, MODE_IMM | MODE_ZP | MODE_ABS | MODE_ZP_Y | MODE_ABS_Y, handleStandardImm0 },	/* LDX */
	{ 0xA0, CPU_6502, IMM_INDEX, MODE_IMM | MODE_ZP | MODE_ABS | MODE_ZP_X | MODE_ABS_X, handleStandardImm0 },	/* LDY */
	{ 0x42, CPU_6502, IMM_8_BIT, MODE_IMM | MODE_A | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X, handleStandardRotate },	/* LSR */
	{ 0xEA, CPU_6502, IMM_NONE, MODE_NONE, handleImplied },	/* NOP */
	{ 0x01, CPU_6502, IMM_ACC, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_ZP_X | MODE_IND_ZP_Y | MODE_IND_ZP | MODE_816_DISP_S | MODE_816_LONG_IND_ZP | MODE_816_LONG_ABS | MODE_816_IND_DISP_S_Y | MODE_816_LONG_IND_ZP_Y | MODE_816_LONG_ABS_X | MODE_4510_IND_ZP_Z, handleStandardAll },	/* ORA */
	{ 0xAA, CPU_6502, IMM_NONE, MODE_NONE, handleImplied },	/* TAX */
	{ 0x8A, CPU_6502, IMM_NONE, MODE_NONE, handleImplied },	/* TXA */
	{ 0xCA, CPU_6502, IMM_NONE, MODE_NONE, handleImplied },	/* DEX */
	{ 0xE8, CPU_6502, IMM_NONE, MODE_NONE, handleImplied },	/* INX */
	{ 0xA8, CPU_6502, IMM_NONE, MODE_NONE, handleImplied },	/* TAY */
	{ 0x98, CPU_6502, IMM_NONE, MODE_NONE, handleImplied },	/* TYA */
	{ 0x88, CPU_6502, IMM_NONE, MODE_NONE, handleImplied },	/* DEY */
	{ 0xC8, CPU_6502, IMM_NONE, MODE_NONE, handleImplied },	/* INY */
	{ 0x22, CPU_6502, IMM_8_BIT, MODE_IMM | MODE_A | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X, handleStandardRotate },	/* ROL */
	{ 0x62, CPU_6502, IMM_8_BIT, MODE_IMM | MODE_A | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X, handleStandardRotate },	/* ROR */
	{ 0x40, CPU_6502, IMM_NONE, MODE_NONE, handleImplied },	/* RTI */
	{ 0x60, CPU_6502, IMM_NONE, MODE_NONE | MODE_IMM, handleRTS },	/* RTS */
	{ 0xE1, CPU_6502, IMM_ACC, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_ZP_X | MODE_IND_ZP_Y | MODE_IND_ZP | MODE_816_DISP_S | MODE_816_LONG_IND_ZP | MODE_816_LONG_ABS | MODE_816_IND_DISP_S_Y | MODE_816_LONG_IND_ZP_Y | MODE_816_LONG_ABS_X | MODE_4510_IND_ZP_Z, handleStandardAll },	/* SBC */
	{ 0x81, CPU_6502, IMM_NONE, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_ZP_X | MODE_IND_ZP_Y | MODE_IND_ZP | MODE_816_DISP_S | MODE_816_LONG_IND_ZP | MODE_816_LONG_ABS | MODE_816_IND_DISP_S_Y | MODE_816_LONG_IND_ZP_Y | MODE_816_LONG_ABS_X | MODE_4510_IND_ZP_Z, handleStandardAll },	/* STA */
	{ 0x9A, CPU_6502, IMM_NONE, MODE_NONE, handleImplied },	/* TXS */
	{ 0xBA, CPU_6502, IMM_NONE, MODE_NONE, handleImplied },	/* TSX */
	{ 0x48, CPU_6502, IMM_NONE, MODE_NONE, handleImplied },	/* PHA */
	{ 0x68, CPU_6502, IMM_NONE, MODE_NONE, handleImplied },	/* PLA */
	{ 0x08, CPU_6502, IMM_NONE, MODE_NONE, handleImplied },	/* PHP */
	{ 0x28, CPU_6502, IMM_NONE, MODE_NONE, handleImplied },	/* PLP */
	{ 0x82, CPU_6502, IMM_NONE, MODE_ZP | MODE_ABS | MODE_ZP_Y | MODE_4510_ABS_Y, handleStandardImm0 },	/* STX */
	{ 0x80, CPU_6502, IMM_NONE, MODE_ZP | MODE_ABS | MODE_ZP_X | MODE_4510_ABS_X, handleStandardImm0 },	/* STY */

	/* Undocumented instructions */

	{ 0x0B, CPU_6502, IMM_8_BIT, MODE_IMM, handleStandardImm0 },	/* AAC */
	{ 0x83, CPU_6502, IMM_8_BIT, MODE_ZP | MODE_ZP_Y | MODE_IND_ZP_X | MODE_ABS, handleStandardAll },	/* AAX */
	{ 0x6B, CPU_6502, IMM_8_BIT, MODE_IMM, handleStandardImm0 },	/* ARR */
	{ 0x4B, CPU_6502, IMM_8_BIT, MODE_IMM, handleStandardImm0 },	/* ASR */
	{ 0xAB, CPU_6502, IMM_8_BIT, MODE_IMM, handleStandardImm0 },	/* ATX */
	{ 0x83, CPU_6502, IMM_8_BIT, MODE_ABS_Y | MODE_IND_ZP_Y, handleStandardAbsY7 },	/* AXA */
	{ 0xCB, CPU_6502, IMM_8_BIT, MODE_IMM, handleStandardImm0 },	/* AXS */
	{ 0xC3, CPU_6502, IMM_8_BIT, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_ZP_X | MODE_IND_ZP_Y, handleStandardAll },	/* DCP */
	{ 0x00, CPU_6502, IMM_8_BIT, MODE_ZP | MODE_ZP_X | MODE_IMM, handleDOP },	/* DOP */
	{ 0xE3, CPU_6502, IMM_8_BIT, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_ZP_X | MODE_IND_ZP_Y, handleStandardAll },	/* ISC */
	{ 0x02, CPU_6502, IMM_8_BIT, MODE_NONE, handleImplied },	/* KIL */
	{ 0xA3, CPU_6502, IMM_8_BIT, MODE_ABS_Y, handleStandardAll },	/* LAR */
	{ 0xA3, CPU_6502, IMM_8_BIT, MODE_ZP | MODE_ZP_Y | MODE_ABS | MODE_ABS_Y | MODE_IND_ZP_X | MODE_IND_ZP_Y, handleStandardAbsY7 },	/* LAX */
	{ 0x43, CPU_6502, IMM_8_BIT, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_ZP_X | MODE_IND_ZP_Y, handleStandardAll },	/* RLA */
	{ 0x63, CPU_6502, IMM_8_BIT, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_ZP_X | MODE_IND_ZP_Y, handleStandardAll },	/* RRA */
	{ 0x03, CPU_6502, IMM_8_BIT, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_ZP_X | MODE_IND_ZP_Y, handleStandardAll },	/* SLO */
	{ 0x43, CPU_6502, IMM_8_BIT, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_ZP_X | MODE_IND_ZP_Y, handleStandardAll },	/* SRE */
	{ 0x82, CPU_6502, IMM_8_BIT, MODE_ABS_Y, handleStandardAbsY7 },	/* SXA */
	{ 0x80, CPU_6502, IMM_8_BIT, MODE_ABS_X, handleStandardAll },	/* SYA */
	{ 0x00, CPU_6502, IMM_8_BIT, MODE_ABS | MODE_ABS_X, handleStandardAll },	/* TOP */
	{ 0x8B, CPU_6502, IMM_8_BIT, MODE_IMM, handleStandardImm0 },	/* XAA */
	{ 0x83, CPU_6502, IMM_8_BIT, MODE_ABS_Y, handleStandardAll },	/* XAS */

	/* 65C02 */
	{ 0x80, CPU_65C02, IMM_NONE, MODE_ABS, handleBranch },	/* BRA */
	{ 0x3A, CPU_65C02, IMM_NONE, MODE_NONE, handleImplied },			/* DEA */
	{ 0x1A, CPU_65C02, IMM_NONE, MODE_NONE, handleImplied },			/* INA */
	{ 0xDA, CPU_65C02, IMM_NONE, MODE_NONE, handleImplied },			/* PHX */
	{ 0x5A, CPU_65C02, IMM_NONE, MODE_NONE, handleImplied },			/* PHY */
	{ 0xFA, CPU_65C02, IMM_NONE, MODE_NONE, handleImplied },			/* PLX */
	{ 0x7A, CPU_65C02, IMM_NONE, MODE_NONE, handleImplied },			/* PLY */
	{ 0x00, CPU_65C02, IMM_NONE, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X, handleSTZ },	/* STZ */
	{ 0x10, CPU_65C02, IMM_NONE, MODE_ZP | MODE_ABS, handleStandardAll },	/* TRB */
	{ 0x00, CPU_65C02, IMM_NONE, MODE_ZP | MODE_ABS, handleStandardAll },	/* TSB */

	/* WDC */
	{ 0xDB, CPU_65C02, IMM_NONE, MODE_NONE, handleImplied },			/* STP */
	{ 0xCB, CPU_65C02, IMM_NONE, MODE_NONE, handleImplied },			/* WAI */

	/* Rockwell + WDC */
	{ 0x0F, CPU_65C02S, IMM_8_BIT, MODE_BIT_ZP_ABS, handleBITBranch_C02 },	/* BBR */
	{ 0x0F, CPU_65C02S, IMM_8_BIT, MODE_ZP_ABS, handleBITxBranch_C02 },		/* BBR0 */
	{ 0x1F, CPU_65C02S, IMM_8_BIT, MODE_ZP_ABS, handleBITxBranch_C02 },		/* BBR1 */
	{ 0x2F, CPU_65C02S, IMM_8_BIT, MODE_ZP_ABS, handleBITxBranch_C02 },		/* BBR2 */
	{ 0x3F, CPU_65C02S, IMM_8_BIT, MODE_ZP_ABS, handleBITxBranch_C02 },		/* BBR3 */
	{ 0x4F, CPU_65C02S, IMM_8_BIT, MODE_ZP_ABS, handleBITxBranch_C02 },		/* BBR4 */
	{ 0x5F, CPU_65C02S, IMM_8_BIT, MODE_ZP_ABS, handleBITxBranch_C02 },		/* BBR5 */
	{ 0x6F, CPU_65C02S, IMM_8_BIT, MODE_ZP_ABS, handleBITxBranch_C02 },		/* BBR6 */
	{ 0x7F, CPU_65C02S, IMM_8_BIT, MODE_ZP_ABS, handleBITxBranch_C02 },		/* BBR7 */

	{ 0x8F, CPU_65C02S, IMM_8_BIT, MODE_BIT_ZP_ABS, handleBITBranch_C02 },	/* BBS */
	{ 0x8F, CPU_65C02S, IMM_8_BIT, MODE_ZP_ABS, handleBITxBranch_C02 },		/* BBS0 */
	{ 0x9F, CPU_65C02S, IMM_8_BIT, MODE_ZP_ABS, handleBITxBranch_C02 },		/* BBS1 */
	{ 0xAF, CPU_65C02S, IMM_8_BIT, MODE_ZP_ABS, handleBITxBranch_C02 },		/* BBS2 */
	{ 0xBF, CPU_65C02S, IMM_8_BIT, MODE_ZP_ABS, handleBITxBranch_C02 },		/* BBS3 */
	{ 0xCF, CPU_65C02S, IMM_8_BIT, MODE_ZP_ABS, handleBITxBranch_C02 },		/* BBS4 */
	{ 0xDF, CPU_65C02S, IMM_8_BIT, MODE_ZP_ABS, handleBITxBranch_C02 },		/* BBS5 */
	{ 0xEF, CPU_65C02S, IMM_8_BIT, MODE_ZP_ABS, handleBITxBranch_C02 },		/* BBS6 */
	{ 0xFF, CPU_65C02S, IMM_8_BIT, MODE_ZP_ABS, handleBITxBranch_C02 },		/* BBS7 */

	{ 0x07, CPU_65C02S, IMM_8_BIT, MODE_BIT_ZP, handleBIT_C02 },		/* RMB */
	{ 0x07, CPU_65C02S, IMM_8_BIT, MODE_ZP, handleBITx_C02 },		/* RMB0 */
	{ 0x17, CPU_65C02S, IMM_8_BIT, MODE_ZP, handleBITx_C02 },		/* RMB1 */
	{ 0x27, CPU_65C02S, IMM_8_BIT, MODE_ZP, handleBITx_C02 },		/* RMB2 */
	{ 0x37, CPU_65C02S, IMM_8_BIT, MODE_ZP, handleBITx_C02 },		/* RMB3 */
	{ 0x47, CPU_65C02S, IMM_8_BIT, MODE_ZP, handleBITx_C02 },		/* RMB4 */
	{ 0x57, CPU_65C02S, IMM_8_BIT, MODE_ZP, handleBITx_C02 },		/* RMB5 */
	{ 0x67, CPU_65C02S, IMM_8_BIT, MODE_ZP, handleBITx_C02 },		/* RMB6 */
	{ 0x77, CPU_65C02S, IMM_8_BIT, MODE_ZP, handleBITx_C02 },		/* RMB7 */

	{ 0x87, CPU_65C02S, IMM_8_BIT, MODE_BIT_ZP, handleBIT_C02 },	/* SMB */
	{ 0x87, CPU_65C02S, IMM_8_BIT, MODE_ZP, handleBITx_C02 },		/* SMB0 */
	{ 0x97, CPU_65C02S, IMM_8_BIT, MODE_ZP, handleBITx_C02 },		/* SMB1 */
	{ 0xA7, CPU_65C02S, IMM_8_BIT, MODE_ZP, handleBITx_C02 },		/* SMB2 */
	{ 0xB7, CPU_65C02S, IMM_8_BIT, MODE_ZP, handleBITx_C02 },		/* SMB3 */
	{ 0xC7, CPU_65C02S, IMM_8_BIT, MODE_ZP, handleBITx_C02 },		/* SMB4 */
	{ 0xD7, CPU_65C02S, IMM_8_BIT, MODE_ZP, handleBITx_C02 },		/* SMB5 */
	{ 0xE7, CPU_65C02S, IMM_8_BIT, MODE_ZP, handleBITx_C02 },		/* SMB6 */
	{ 0xF7, CPU_65C02S, IMM_8_BIT, MODE_ZP, handleBITx_C02 },		/* SMB7 */
};


extern bool
x65_HandleToken(ETargetToken token, uint32_t allowedModes) {
	assert(T_6502_ADC <= token && token <= T_65C02_SMB7);

	SParser* handler = &g_instructionHandlers[token - T_6502_ADC];

	if (handler->cpu & opt_Current->machineOptions->cpu) {
		SAddressingMode addrMode;
		allowedModes &= handler->allowedModes & opt_Current->machineOptions->allowedModes;
		if (x65_ParseAddressingMode(&addrMode, allowedModes, handler->immSize) && (addrMode.mode & allowedModes))
			return handler->handler(handler->baseOpcode, &addrMode);
		else
			err_Error(MERROR_ILLEGAL_ADDRMODE);
	} else {
		err_Error(MERROR_INSTRUCTION_NOT_SUPPORTED);
	}

	return true;
}


bool
x65_ParseIntegerInstruction(void) {
	if (T_6502_ADC <= lex_Context->token.id && lex_Context->token.id <= T_65C02_SMB7) {
		ETargetToken token = (ETargetToken) lex_Context->token.id;
		parse_GetToken();
		return x65_HandleToken(token, 0xFFFFFFFFu);
	}

	return false;
}
