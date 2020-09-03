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

#include <assert.h>

#include "expression.h"
#include "errors.h"
#include "lexer.h"
#include "options.h"
#include "parse_expression.h"
#include "parse.h"

#include "rc8_errors.h"
#include "rc8_options.h"
#include "rc8_parse.h"
#include "rc8_tokens.h"

#define MODE_REG_F	        0x0000001
#define MODE_REG_T	        0x0000002
#define MODE_REG_B	        0x0000004
#define MODE_REG_C	        0x0000008
#define MODE_REG_D	        0x0000010
#define MODE_REG_E	        0x0000020
#define MODE_REG_H	        0x0000040
#define MODE_REG_L	        0x0000080
#define MODE_REG_FT	        0x0000100
#define MODE_REG_BC	        0x0000200
#define MODE_REG_DE	        0x0000400
#define MODE_REG_HL	        0x0000800
#define MODE_IND_C	        0x0001000
#define MODE_IND_FT	        0x0002000
#define MODE_IND_BC	        0x0004000
#define MODE_IND_DE	        0x0008000
#define MODE_IND_HL	        0x0010000
#define MODE_IMM            0x0020000
#define MODE_ADDR           0x0040000
#define MODE_IND_BC_L       0x0080000
#define MODE_IND_BC_H       0x0100000
#define MODE_IND_BC_E       0x0200000
#define MODE_IND_BC_D       0x0400000
#define MODE_IND_BC_T       0x0800000
#define MODE_IND_BC_F       0x1000000
#define MODE_REGISTER_MASK  0x2000000
#define MODE_NONE           0x4000000

#define MODE_REG_8BIT (MODE_REG_F | MODE_REG_T | MODE_REG_B | MODE_REG_C | MODE_REG_D | MODE_REG_E | MODE_REG_H | MODE_REG_L)
#define MODE_REG_8BIT_FBCDEHL (MODE_REG_F | MODE_REG_B | MODE_REG_C | MODE_REG_D | MODE_REG_E | MODE_REG_H | MODE_REG_L)
#define MODE_REG_8BIT_BCDEHL (MODE_REG_B | MODE_REG_C | MODE_REG_D | MODE_REG_E | MODE_REG_H | MODE_REG_L)
#define MODE_REG_16BIT (MODE_REG_FT | MODE_REG_BC | MODE_REG_DE | MODE_REG_HL)
#define MODE_REG_16BIT_BCDEHL (MODE_REG_BC | MODE_REG_DE | MODE_REG_HL)
#define MODE_IND_16BIT (MODE_IND_FT | MODE_IND_BC | MODE_IND_DE | MODE_IND_HL)
#define MODE_IND_16BIT_BCDEHL (MODE_IND_BC | MODE_IND_DE | MODE_IND_HL)
#define MODE_IND_16BIT (MODE_IND_FT | MODE_IND_BC | MODE_IND_DE | MODE_IND_HL)
#define MODE_IND_OFFSET (MODE_IND_BC_L | MODE_IND_BC_H | MODE_IND_BC_E | MODE_IND_BC_D | MODE_IND_BC_T | MODE_IND_BC_F)
#define MODE_IND_OFFSET_FDEHL (MODE_IND_BC_L | MODE_IND_BC_H | MODE_IND_BC_E | MODE_IND_BC_D | MODE_IND_BC_F)

typedef enum {
	// These must be in the same order as the tokens and end with CC_ALWAYS
	// as this is the order of the opcodes
	CC_LE,
	CC_GT,
	CC_LT,
	CC_GE,
	CC_LEU,
	CC_GTU,
	CC_LTU,
	CC_GEU,
	CC_EQ,
	CC_NE,
	CC_ALWAYS,
} EConditionCode;

typedef enum {
	CONDITION_AUTO,
	CONDITION_PASS
} EConditionType;

typedef struct {
	uint32_t mode;
	uint32_t registerIndex;
	SExpression* expression;
} SAddressingMode;

typedef struct Parser {
	uint8_t	baseOpcode;
	EConditionType condition;
	uint32_t firstModes;
	uint32_t secondModes;
	bool (*parser)(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* mode1, SAddressingMode* mode2);
} SParser;


static string*
createUniqueLabel() {
	static uint32_t id = 0;
    char sym[32];

    snprintf(sym, sizeof(sym), "$target%u", id++);
    return str_Create(sym);
}


static EConditionCode
invertCondition(EConditionCode cc) {
	if (cc == CC_ALWAYS)
		 return CC_ALWAYS;

	return cc ^ 1;
}


static bool
handle_OpcodeRegister(uint8_t baseOpcode, SAddressingMode* sourceMode) {
	sect_OutputConst8(baseOpcode | sourceMode->registerIndex);
	return true;
}


static void 
ensureSource(SAddressingMode** destination, SAddressingMode** source) {
	// If only destination has been specified, swap the two modes.
	// This is usually used with instruction that have an implicit destination, such as T

	if ((*source)->mode == MODE_NONE) {
		SAddressingMode* t = *destination;
		*destination = *source;
		*source = t;
	}
}


static bool 
handle_ADD(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	ensureSource(&destination, &source);

	if ((source->mode & MODE_REG_8BIT) && (destination->mode == MODE_NONE || destination->mode == MODE_REG_T))
		return handle_OpcodeRegister(0x40, source);
	else if ((source->mode & MODE_REG_16BIT) && (destination->mode == MODE_NONE || destination->mode == MODE_REG_FT))
		return handle_OpcodeRegister(0xF4, source);

	return false;
}


static bool 
handle_Common_FBCDEHL(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	ensureSource(&destination, &source);

	if ((source->mode & MODE_REG_8BIT_FBCDEHL) && (destination->mode == MODE_NONE || destination->mode == MODE_REG_T))
		return handle_OpcodeRegister(baseOpcode, source);

	return false;
}


static void 
registerPair(SAddressingMode* high, SAddressingMode* low, SAddressingMode* src) {
	uint32_t registerBase = src->registerIndex * 2;
	high->mode = MODE_REG_F << registerBase;
	high->registerIndex = registerBase;
	low->mode = MODE_REG_T << registerBase;
	low->registerIndex = registerBase + 1;
}


static bool
handle_INC_DEC(uint8_t baseOpcode, uint8_t nBase16Opcode, SAddressingMode* destination, SAddressingMode* source) {
	if (destination->mode & MODE_REG_8BIT) {
		return handle_OpcodeRegister(baseOpcode, destination);
	} else if (destination->mode & MODE_REG_16BIT) {
		return handle_OpcodeRegister(nBase16Opcode, destination);
	}

	return false;
}


static bool
handle_DEC(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	return handle_INC_DEC(0x58, 0xC4, destination, source);
}


static bool
handle_INC(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	return handle_INC_DEC(0x48, 0xC0, destination, source);
}


static bool
handle_Implicit(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	sect_OutputConst8(baseOpcode);
	return true;
}


static bool
handle_JumpRelative(uint8_t opcode, SExpression* destination) {
	sect_OutputConst8(opcode);

	SExpression* pcrelExpr = expr_CheckRange(expr_PcRelative(destination, 0), -128, 127);
	if (pcrelExpr == NULL)
		return false;

	sect_OutputExpr8(pcrelExpr);
	return true;
}


static bool
handle_JumpRelativeCC(EConditionCode cc, SExpression* destination) {
	return handle_JumpRelative(0x80 + cc, destination);
}


static bool
handle_DJ(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	return handle_JumpRelative(baseOpcode + destination->registerIndex, source->expression);
}


static bool
handle_J(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	if (destination->mode == MODE_ADDR && source->mode == MODE_NONE)
		return handle_JumpRelativeCC(cc, destination->expression);

	if ((destination->mode & MODE_IND_16BIT) && source->mode == MODE_NONE) {
 		if (cc != CC_ALWAYS) {
			if (!opt_Current->machineOptions->enableSynthInstructions)
				return err_Error(MERROR_REQUIRES_SYNTHESIZED);

			cc = invertCondition(cc);
			sect_OutputConst8(0x80 + cc);
			sect_OutputConst8(0x02);
		}

 		return handle_OpcodeRegister(baseOpcode, destination);
	}

	return false;
}

static bool
handle_LCO(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	return handle_OpcodeRegister(baseOpcode, source);
}


static bool
handle_LCR(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	if (destination->mode == MODE_IND_C && source->mode == MODE_REG_T)
		return handle_Implicit(0xCC, cc, destination, source);
	else if (destination->mode == MODE_REG_T && source->mode == MODE_IND_C)
		return handle_Implicit(0xDC, cc, destination, source);

	return false;
}


static void
handle_LD_R8_const(SAddressingMode* registerDest, uint8_t value) {
	sect_OutputConst8(0x70 | registerDest->registerIndex);
	sect_OutputConst8(value);
}


static bool
handle_LD_R8_imm(SAddressingMode* registerDest, SExpression* expression) {
	expression = expr_CheckRange(expression, -128, 255);
	if (!expression)
		return false;

	if (expr_IsConstant(expression)) {
		handle_LD_R8_const(registerDest, expression->value.integer);
		return true;
	} else {
		SExpression* masked = expr_And(expression, expr_Const(0xFF));

		handle_OpcodeRegister(0x70, registerDest);
		sect_OutputExpr8(masked);

		return true;
	}
}


static bool 
handle_LD_R16_3bytes(SAddressingMode* registerHigh, SAddressingMode* registerLow, uint8_t high, uint8_t low) {
	if (registerHigh->registerIndex == 0 && registerLow->registerIndex == 1) {
		if (high == 0xFF && (low == 0 || (low & 0x80))) {
			handle_LD_R8_const(registerLow, low);	/* LD T,n8 */
			sect_OutputConst8(0x99);				/* SMZ T */
			return true;
		} else if (high == low) {
			handle_LD_R8_const(registerLow, low);	/* LD r8,n8 */
			sect_OutputConst8(0x60);				/* LD F,T */
			return true;
		}
	}

	return false;
}


static bool
handle_LD_R16_4bytes(SAddressingMode* registerHigh, SAddressingMode* registerLow, uint8_t high, uint8_t low) {
	handle_LD_R8_const(registerHigh, high);
	handle_LD_R8_const(registerLow, low);
	return true;
}


static bool
handle_LD_R16_imm(SAddressingMode* dest, SExpression* expression) {
	SAddressingMode registerLow;
	SAddressingMode registerHigh;
	registerPair(&registerHigh, &registerLow, dest);

	SExpression* masked = expr_CheckRange(expression, -32768, 65535);
	if (!masked)
		return false;

	if (expr_IsConstant(masked)) {
		uint16_t value = masked->value.integer;
		uint8_t high = value >> 8;
		uint8_t low = value & 0xFF;

		if (handle_LD_R16_3bytes(&registerHigh, &registerLow, high, low))
			return true;
		if (handle_LD_R16_4bytes(&registerHigh, &registerLow, high, low))
			return true;
	}

	SExpression* high = expr_Asr(expr_Clone(masked), expr_Const(8));
	SExpression* low = expr_And(masked, expr_Const(0xFF));

	if (high == NULL || low == NULL)
		return false;

	if (!handle_LD_R8_imm(&registerHigh, high))
		return false;
	return handle_LD_R8_imm(&registerLow, low);
}


static bool 
handle_LD(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	if ((destination->mode & MODE_REG_8BIT) && source->mode == MODE_IMM) {
		return handle_LD_R8_imm(destination, source->expression);
	} else if ((destination->mode & MODE_REG_16BIT) && source->mode == MODE_IMM && opt_Current->machineOptions->enableSynthInstructions) {
		return handle_LD_R16_imm(destination, source->expression);
	} else if ((destination->mode & MODE_IND_16BIT_BCDEHL) && source->mode == MODE_REG_T) {
		return handle_OpcodeRegister(0x00, destination);
	} else if (destination->mode == MODE_REG_T && (source->mode & MODE_IND_16BIT)) {
		return handle_OpcodeRegister(0x04, source);
	} else if ((destination->mode & MODE_REG_8BIT_FBCDEHL) && source->mode == MODE_REG_T) {
		return handle_OpcodeRegister(0x60, destination);
	} else if (destination->mode == MODE_REG_T && (source->mode & MODE_REG_8BIT_FBCDEHL)) {
		return handle_OpcodeRegister(0x68, source);
	} else if ((destination->mode & MODE_REG_16BIT_BCDEHL) && source->mode == MODE_REG_FT) {
		return handle_OpcodeRegister(0xD0, destination);
	} else if (destination->mode == MODE_REG_FT && (source->mode & MODE_REG_16BIT_BCDEHL)) {
		return handle_OpcodeRegister(0xD8, source);
	} else if ((destination->mode & MODE_IND_OFFSET_FDEHL) && source->mode == MODE_REG_T) {
		return handle_OpcodeRegister(0x18, destination);
	} else if (destination->mode == MODE_REG_T && (source->mode & MODE_IND_OFFSET)) {
		return handle_OpcodeRegister(0x10, source);
	}

	return false;
}


static bool 
handle_JAL(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	if (destination->mode == MODE_ADDR && opt_Current->machineOptions->enableSynthInstructions) {
		SAddressingMode mode = { MODE_REG_HL, 3, NULL };
		if (!handle_LD_R16_imm(&mode, destination->expression))
			return false;
		sect_OutputConst8(baseOpcode + 3);
		return true;
	} else if (destination->mode & MODE_IND_16BIT) {
		return handle_OpcodeRegister(baseOpcode, destination);
	}

	return false;
}


static bool
handle_LoadIndirect(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	if (destination->mode == MODE_IND_BC && source->mode == MODE_REG_T)
		return handle_Implicit(baseOpcode, cc, destination, source);
	else if (destination->mode == MODE_REG_T && source->mode == MODE_IND_BC)
		return handle_Implicit(baseOpcode + 0x08, cc, destination, source);

	return false;
}


static uint8_t bitsSet2(uint8_t value) {
	if (value == 0)
		return 0;

	return ((value & 0x2) >> 1) + (value & 1);
}


static uint8_t bitsSet4(uint8_t value) {
	if (value == 0)
		return 0;

	return bitsSet2(value & 0x03) + bitsSet2(value >> 2);
}


static void
outputStackOps(uint8_t baseOpcode, uint8_t registerMask) {
	while (registerMask != 0) {
		if (registerMask & 1) {
			sect_OutputConst8(baseOpcode);
		}
		registerMask >>= 1;
		baseOpcode += 1;
	}
}

static bool
handle_RegisterStack(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	if (destination->mode == MODE_REGISTER_MASK) {
		if (!opt_Current->machineOptions->enableSynthInstructions)
			return err_Error(MERROR_REQUIRES_SYNTHESIZED);
			
		bool isPop = (baseOpcode & 0x04) != 0;
		uint8_t all = isPop ? 0xF9 : 0xF8;
		uint8_t oppositeOne = baseOpcode ^ 0x04;

		uint8_t registerCount = bitsSet4(destination->registerIndex);
		assert (registerCount >= 2 && registerCount <= 4);
		switch (registerCount) {
			case 4:
				sect_OutputConst8(all);
				break;
			case 3:
				if (!isPop)
					sect_OutputConst8(all);

				outputStackOps(oppositeOne, destination->registerIndex ^ 0xF);

				if (isPop)
					sect_OutputConst8(all);
				break;
			case 2:
				outputStackOps(baseOpcode, destination->registerIndex);
				break;
		}
		return true;
	}

	return handle_OpcodeRegister(baseOpcode, destination);
}


static bool
handle_Shift(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	ensureSource(&destination, &source);

	if ((destination->mode & (MODE_REG_FT | MODE_NONE)) && (source->mode & MODE_REG_8BIT_BCDEHL))
		return handle_OpcodeRegister(baseOpcode, source);

	return false;
}


static bool
handle_CMP(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	if (destination->mode & MODE_IMM) {
		if (!opt_Current->machineOptions->enableSynthInstructions)
			return err_Error(MERROR_REQUIRES_SYNTHESIZED);
			
		sect_OutputConst8(0x70);	// LD F,n8
		sect_OutputExpr8(destination->expression);
		sect_OutputConst8(baseOpcode);
		return true;
	} else if (destination->mode & MODE_REG_8BIT_FBCDEHL) {
		return handle_OpcodeRegister(baseOpcode, destination);
	} else if (destination->mode & MODE_REG_16BIT_BCDEHL) {
		return handle_OpcodeRegister(baseOpcode + 0x54, destination);
	}

	return false;
}


static bool
handle_NOT(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	sect_OutputConst8(baseOpcode | (destination->registerIndex));
	return true;
}


static bool
handle_TST(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	if (destination->mode & MODE_REG_8BIT) {
		return handle_OpcodeRegister(baseOpcode, destination);
	} else if (destination->mode & MODE_REG_16BIT) {
		return handle_OpcodeRegister(baseOpcode + 0x34, destination);
	}

	return false;
}


static bool
handle_NEG(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	if (destination->mode & MODE_REG_16BIT)
		baseOpcode = 0xF0;

	sect_OutputConst8(baseOpcode);

	return true;
}


static bool
handle_SWAP(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	return handle_OpcodeRegister(baseOpcode, destination);
}


static bool
handle_SYS(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	sect_OutputConst8(baseOpcode);
	sect_OutputExpr8(destination->expression);
	return true;
}


static bool
handle_SUB(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	ensureSource(&destination, &source);

	if ((source->mode & MODE_REG_8BIT) && (destination->mode == MODE_NONE || destination->mode == MODE_REG_T))
		return handle_OpcodeRegister(0x50, source);
	else if ((source->mode & MODE_REG_16BIT_BCDEHL) && (destination->mode == MODE_NONE || destination->mode == MODE_REG_FT))
		return handle_OpcodeRegister(0xF0, source);

	return false;
}


static bool handle_XOR(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	ensureSource(&destination, &source);

	if ((source->mode & MODE_REG_8BIT) && (destination->mode == MODE_NONE || destination->mode == MODE_REG_T))
		return handle_OpcodeRegister(0xB8, source);

	return false;
}


static SParser
g_Parsers[T_RC8_XOR - T_RC8_ADD + 1] = {
	{ 0x00, CONDITION_AUTO, MODE_REG_8BIT | MODE_REG_16BIT, MODE_NONE | MODE_REG_8BIT | MODE_REG_16BIT, handle_ADD },	/* ADD */
	{ 0xB0, CONDITION_AUTO, MODE_REG_8BIT, MODE_NONE | MODE_REG_8BIT, handle_Common_FBCDEHL },		/* AND */
	{ 0x78, CONDITION_AUTO, MODE_IMM | MODE_REG_8BIT | MODE_REG_16BIT, MODE_NONE, handle_CMP },		/* CMP */
	{ 0x00, CONDITION_AUTO, MODE_REG_8BIT | MODE_REG_16BIT, MODE_NONE, handle_DEC },				/* DEC */
	{ 0x13, CONDITION_AUTO, MODE_NONE, MODE_NONE, handle_Implicit },								/* DI */
	{ 0x90, CONDITION_AUTO, MODE_REG_8BIT, MODE_ADDR, handle_DJ },									/* DJ */
	{ 0x12, CONDITION_AUTO, MODE_NONE, MODE_NONE, handle_Implicit },								/* EI */
	{ 0x79, CONDITION_AUTO, MODE_NONE, MODE_NONE, handle_Implicit },								/* EXT */
	{ 0x00, CONDITION_AUTO, MODE_REG_8BIT | MODE_REG_16BIT, MODE_NONE, handle_INC },				/* INC */
	{ 0x9C, CONDITION_PASS, MODE_IND_16BIT | MODE_ADDR, MODE_NONE, handle_J },						/* J */
	{ 0x98, CONDITION_AUTO, MODE_IND_16BIT | MODE_ADDR, MODE_NONE, handle_JAL },					/* JAL */
	{ 0x0C, CONDITION_AUTO, MODE_REG_T, MODE_IND_16BIT, handle_LCO },								/* LCO */
	{ 0x00, CONDITION_AUTO, MODE_IND_C | MODE_REG_T, MODE_IND_C | MODE_REG_T, handle_LCR },			/* LCR */
	{ 0x00, CONDITION_AUTO, MODE_REG_8BIT | MODE_REG_16BIT | MODE_IND_16BIT_BCDEHL | MODE_IND_16BIT | MODE_IND_OFFSET_FDEHL, MODE_IMM | MODE_REG_8BIT | MODE_REG_16BIT | MODE_IND_16BIT | MODE_IND_OFFSET, handle_LD },		/* LD */
	{ 0xD0, CONDITION_AUTO, MODE_IND_BC | MODE_REG_T, MODE_IND_BC | MODE_REG_T, handle_LoadIndirect },	/* LIO */
	{ 0xE0, CONDITION_AUTO, MODE_REG_FT | MODE_REG_8BIT_BCDEHL, MODE_REG_8BIT_BCDEHL | MODE_NONE, handle_Shift },	/* LS */
	{ 0x51, CONDITION_AUTO, MODE_REG_T | MODE_REG_FT, MODE_NONE, handle_NEG },						/* NEG */
	{ 0x00, CONDITION_AUTO, MODE_NONE, MODE_NONE, handle_Implicit },								/* NOP */
	{ 0x08, CONDITION_AUTO, MODE_REG_F | MODE_REG_T, MODE_NONE, handle_NOT },						/* NOT */
	{ 0xA8, CONDITION_AUTO, MODE_REG_8BIT, MODE_NONE | MODE_REG_8BIT, handle_Common_FBCDEHL },		/* OR */
	{ 0x3C, CONDITION_AUTO, MODE_REG_16BIT | MODE_REGISTER_MASK, MODE_NONE, handle_RegisterStack },	/* POP */
	{ 0xF9, CONDITION_AUTO, MODE_NONE, MODE_NONE, handle_Implicit },								/* POPA */
	{ 0x38, CONDITION_AUTO, MODE_REG_16BIT | MODE_REGISTER_MASK, MODE_NONE, handle_RegisterStack },	/* PUSH */
	{ 0xF8, CONDITION_AUTO, MODE_NONE, MODE_NONE, handle_Implicit },								/* PUSHA */
	{ 0x69, CONDITION_AUTO, MODE_NONE, MODE_NONE, handle_Implicit },								/* RETI */
	{ 0xE8, CONDITION_AUTO, MODE_REG_FT | MODE_REG_8BIT_BCDEHL, MODE_REG_8BIT_BCDEHL | MODE_NONE, handle_Shift },	/* RS */
	{ 0xF8, CONDITION_AUTO, MODE_REG_FT | MODE_REG_8BIT_BCDEHL, MODE_REG_8BIT_BCDEHL | MODE_NONE, handle_Shift },	/* RSA */
	{ 0x00, CONDITION_AUTO, MODE_REG_8BIT | MODE_REG_16BIT, MODE_NONE | MODE_REG_8BIT | MODE_REG_16BIT | MODE_IMM, handle_SUB },	/* SUB */
	{ 0xC8, CONDITION_AUTO, MODE_REG_16BIT, MODE_NONE, handle_SWAP },								/* SWAP */
	{ 0x19, CONDITION_AUTO, MODE_IMM, MODE_NONE, handle_SYS },										/* SYS */
	{ 0xA0, CONDITION_AUTO, MODE_REG_8BIT | MODE_REG_16BIT, MODE_NONE, handle_TST },				/* TST */
	{ 0xB8, CONDITION_AUTO, MODE_REG_8BIT, MODE_NONE | MODE_REG_8BIT, handle_XOR },					/* XOR */
};


typedef struct RegisterMode {
	uint32_t modeFlag;
	uint32_t token;
	uint32_t opcodeRegister;
} SRegisterMode;


static SRegisterMode 
g_RegisterModes[T_RC8_REG_BC_L_IND - T_RC8_REG_F + 1] = {
	{ MODE_REG_F,  T_RC8_REG_F, 0 },
	{ MODE_REG_T,  T_RC8_REG_T, 1 },
	{ MODE_REG_B,  T_RC8_REG_B, 2 },
	{ MODE_REG_C,  T_RC8_REG_C, 3 },
	{ MODE_REG_D,  T_RC8_REG_D, 4 },
	{ MODE_REG_E,  T_RC8_REG_E, 5 },
	{ MODE_REG_H,  T_RC8_REG_H, 6 },
	{ MODE_REG_L,  T_RC8_REG_L, 7 },
	{ MODE_REG_FT, T_RC8_REG_FT, 0 },
	{ MODE_REG_BC, T_RC8_REG_BC, 1 },
	{ MODE_REG_DE, T_RC8_REG_DE, 2 },
	{ MODE_REG_HL, T_RC8_REG_HL, 3 },
	{ MODE_IND_C, T_RC8_REG_C_IND, 0 },
	{ MODE_IND_FT, T_RC8_REG_FT_IND, 0 },
	{ MODE_IND_BC, T_RC8_REG_BC_IND, 1 },
	{ MODE_IND_DE, T_RC8_REG_DE_IND, 2 },
	{ MODE_IND_HL, T_RC8_REG_HL_IND, 3 },
	{ MODE_IND_BC_F, T_RC8_REG_BC_F_IND, 0 },
	{ MODE_IND_BC_T, T_RC8_REG_BC_T_IND, 1 },
	{ MODE_IND_BC_D, T_RC8_REG_BC_D_IND, 4 },
	{ MODE_IND_BC_E, T_RC8_REG_BC_E_IND, 5 },
	{ MODE_IND_BC_H, T_RC8_REG_BC_H_IND, 6 },
	{ MODE_IND_BC_L, T_RC8_REG_BC_L_IND, 7 },
};


static bool
parseAddressingMode(SAddressingMode* addrMode, int allowedModes) {
	SLexerBookmark bm;
	lex_Bookmark(&bm);

	if (allowedModes & MODE_REGISTER_MASK) {
		uint8_t mask = 0;
		uint8_t registers = 0;

		while (lex_Current.token >= T_RC8_REG_FT && lex_Current.token <= T_RC8_REG_HL) {
			uint32_t firstToken = lex_Current.token;

			mask |= 1 << (firstToken - T_RC8_REG_FT);
			registers += 1;

			parse_GetToken();

			if (lex_Current.token == T_OP_SUBTRACT) {
				parse_GetToken();
				if (lex_Current.token > firstToken && lex_Current.token <= T_RC8_REG_HL) {
					uint32_t lastToken = lex_Current.token;

					firstToken += 1;
					while (firstToken <= lastToken) {
						mask |= 1 << (firstToken - T_RC8_REG_FT);
						firstToken += 1;
						registers += 1;
					}

					parse_GetToken();
				} else {
					break;
				}
			}

			if (lex_Current.token == T_OP_DIVIDE) {
				parse_GetToken();
				continue;
			} else {
				break;
			}
		}

		if (registers >= 2) {
			addrMode->mode = MODE_REGISTER_MASK;
			addrMode->expression = NULL;
			addrMode->registerIndex = mask;

			return true;
		}

		lex_Goto(&bm);
	}

	if (lex_Current.token >= T_RC8_REG_F && lex_Current.token <= T_RC8_REG_BC_L_IND) {
		SRegisterMode* mode = &g_RegisterModes[lex_Current.token - T_RC8_REG_F];
		if (allowedModes & mode->modeFlag) {
			parse_GetToken();
			addrMode->mode = mode->modeFlag;
			addrMode->registerIndex = mode->opcodeRegister;
			return true;
		}
	}
	
	if (allowedModes & MODE_ADDR) {
		addrMode->mode = MODE_ADDR;
		addrMode->expression = parse_Expression(1);

		if (addrMode->expression != NULL)
			return true;

		lex_Goto(&bm);
	}

	if (allowedModes & MODE_IMM) {
		addrMode->mode = MODE_IMM;
		addrMode->expression = rc8_ExpressionSU16();

		if (addrMode->expression != NULL)
			return true;

		lex_Goto(&bm);
	}

	if ((allowedModes == 0) || (allowedModes & MODE_NONE)) {
		addrMode->mode = MODE_NONE;
		addrMode->expression = NULL;
		return true;
	}

	return false;
}


bool
rc8_ParseIntegerInstruction(void) {
	if (T_RC8_ADD <= lex_Current.token && lex_Current.token <= T_RC8_XOR) {
		SAddressingMode addrMode1;
		SAddressingMode addrMode2;
		ETargetToken token = lex_Current.token;
		SParser* parser = &g_Parsers[token - T_RC8_ADD];
		EConditionCode cc = CC_ALWAYS;

		parse_GetToken();

		if (lex_Current.token == T_OP_DIVIDE) {
			parse_GetToken();

			if (lex_Current.token >= T_RC8_CC_LE && lex_Current.token <= T_RC8_CC_NE) {
				cc = lex_Current.token - T_RC8_CC_LE;
			} else {
				err_Error(MERROR_EXPECTED_CONDITION_CODE);
				return false;
			}
			parse_GetToken();
		}

		if (parseAddressingMode(&addrMode1, parser->firstModes)) {
			string* jumpTarget = NULL;

			if (lex_Current.token == ',') {
				parse_GetToken();
				if (!parseAddressingMode(&addrMode2, parser->secondModes))
					return err_Error(MERROR_ILLEGAL_ADDRMODE);
			} else if ((parser->secondModes & MODE_NONE) || (parser->secondModes == 0)) {
				addrMode2.mode = MODE_NONE;
				addrMode2.expression = NULL;
			} else {
				return err_Error(MERROR_ILLEGAL_ADDRMODE);
			}

			if (parser->condition == CONDITION_AUTO && cc != CC_ALWAYS) {
				jumpTarget = createUniqueLabel();
				handle_JumpRelativeCC(invertCondition(cc), expr_Symbol(jumpTarget));
			}

			if (!parser->parser(parser->baseOpcode, cc, &addrMode1, &addrMode2))
				return err_Error(ERROR_OPERAND);

			if (jumpTarget != NULL) {
				sym_CreateLabel(jumpTarget);
			}
		} else {
			return err_Error(ERROR_OPERAND);
		}

	}

	return false;
}
