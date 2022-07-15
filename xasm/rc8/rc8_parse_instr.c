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

#define MODE_REG_F               0x00000001
#define MODE_REG_T               0x00000002
#define MODE_REG_B               0x00000004
#define MODE_REG_C               0x00000008
#define MODE_REG_D               0x00000010
#define MODE_REG_E               0x00000020
#define MODE_REG_H	             0x00000040
#define MODE_REG_L               0x00000080
#define MODE_REG_FT	             0x00000100
#define MODE_REG_BC	             0x00000200
#define MODE_REG_DE	             0x00000400
#define MODE_REG_HL	             0x00000800
#define MODE_IND_FT	             0x00001000
#define MODE_IND_BC	             0x00002000
#define MODE_IND_DE	             0x00004000
#define MODE_IND_HL	             0x00008000
#define MODE_IND_FT_POST_INC     0x00010000
#define MODE_IND_BCDEHL_POST_INC 0x00020000
#define MODE_IND_FT_PRE_DEC      0x00040000
#define MODE_IND_BCDEHL_PRE_DEC  0x00080000
#define MODE_IND_C	             0x00100000
#define MODE_IMM                 0x00200000
#define MODE_ADDR                0x00400000
#define MODE_REGISTER_MASK       0x00800000
#define MODE_NONE                0x01000000

#define MODE_REG_8BIT (MODE_REG_F | MODE_REG_T | MODE_REG_B | MODE_REG_C | MODE_REG_D | MODE_REG_E | MODE_REG_H | MODE_REG_L)
#define MODE_REG_8BIT_FBCDEHL (MODE_REG_F | MODE_REG_B | MODE_REG_C | MODE_REG_D | MODE_REG_E | MODE_REG_H | MODE_REG_L)
#define MODE_REG_8BIT_BCDEHL (MODE_REG_B | MODE_REG_C | MODE_REG_D | MODE_REG_E | MODE_REG_H | MODE_REG_L)
#define MODE_REG_16BIT (MODE_REG_FT | MODE_REG_BC | MODE_REG_DE | MODE_REG_HL)
#define MODE_REG_16BIT_BCDEHL (MODE_REG_BC | MODE_REG_DE | MODE_REG_HL)
#define MODE_IND_16BIT (MODE_IND_FT | MODE_IND_BC | MODE_IND_DE | MODE_IND_HL)
#define MODE_IND_16BIT_BCDEHL (MODE_IND_BC | MODE_IND_DE | MODE_IND_HL)
#define MODE_IND_16BIT_INC_DEC (MODE_IND_FT_POST_INC | MODE_IND_BCDEHL_POST_INC | MODE_IND_FT_PRE_DEC | MODE_IND_BCDEHL_PRE_DEC)
#define MODE_IND_16BIT_FT_INC_DEC (MODE_IND_FT_POST_INC | MODE_IND_FT_PRE_DEC)
#define MODE_IND_16BIT_BCDEHL_INC_DEC (MODE_IND_BCDEHL_POST_INC | MODE_IND_BCDEHL_PRE_DEC)

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


static bool
handle_Op_Reg_Imm(uint8_t baseOpcode, int lowerBound, int upperBound, SAddressingMode* registerDest, SExpression* expression) {
	SExpression* ranged = expr_CheckRange(expression, lowerBound, upperBound);
	if (ranged == NULL) {
		return err_Error(ERROR_OPERAND_RANGE);
	}

	SExpression* masked = 
		expr_And(
			ranged,
			expr_Const(0xFF));

	if (!expression) {
		return false;
	}

	if (registerDest == NULL)
		sect_OutputConst8(baseOpcode);
	else
		handle_OpcodeRegister(baseOpcode, registerDest);

	sect_OutputExpr8(masked);

	return true;
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
handle_Op_Reg_SignedImm(uint8_t baseOpcode, SAddressingMode* registerDest, SExpression* expression) {
	return handle_Op_Reg_Imm(baseOpcode, -128, 127, registerDest, expression);
}


static bool
handle_Op_Reg_SignedOrUnsignedImm(uint8_t baseOpcode, SAddressingMode* registerDest, SExpression* expression) {
	return handle_Op_Reg_Imm(baseOpcode, -128, 255, registerDest, expression);
}


static bool
handle_Op_Reg_UnsignedImm(uint8_t baseOpcode, SAddressingMode* registerDest, SExpression* expression) {
	return handle_Op_Reg_Imm(baseOpcode, 0, 255, registerDest, expression);
}


static bool
handle_Op_Reg_UnsignedImm4(uint8_t baseOpcode, SAddressingMode* registerDest, SExpression* expression) {
	return handle_Op_Reg_Imm(baseOpcode, 0, 15, registerDest, expression);
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

	if ((destination->mode & (MODE_REG_T | MODE_NONE)) && (source->mode & MODE_REG_8BIT))
		return handle_OpcodeRegister(0x40, source);
	else if ((destination->mode & (MODE_REG_FT | MODE_NONE)) && (source->mode & MODE_REG_16BIT))
		return handle_OpcodeRegister(0xF4, source);
	else if ((destination->mode & MODE_REG_8BIT) && (source->mode & MODE_IMM))
		return handle_Op_Reg_SignedOrUnsignedImm(0xA0, destination, source->expression);
	else if ((destination->mode & MODE_REG_16BIT) && (source->mode & MODE_IMM)) {
		SExpression* expr = expr_Asr(expr_Asl(expr_CheckRange(source->expression, -32768, 65535), expr_Const(16)), expr_Const(16));
		if (expr_IsConstant(source->expression) && expr->value.integer >= -128 && expr->value.integer <= 127) {
			return handle_Op_Reg_SignedImm(0xBC, destination, expr);
		}

		if (!opt_Current->machineOptions->enableSynthInstructions)
			return err_Error(MERROR_REQUIRES_SYNTHESIZED);

		SAddressingMode highAddr, lowAddr;
		registerPair(&highAddr, &lowAddr, destination);

		SExpression* low = expr_Asr(expr_Asl(expr_Clone(expr), expr_Const(24)), expr_Const(24));
		SExpression* highAdjust = expr_Asr(expr_Clone(low), expr_Const(8));
		SExpression* high = expr_Sub(expr_Asr(expr_Clone(expr), expr_Const(8)), highAdjust);

		if (!expr_IsConstant(low) || low->value.integer != 0) {
			handle_Op_Reg_SignedImm(0xBC, destination, low);
		}
		if (!expr_IsConstant(high) || high->value.integer != 0) {
			handle_Op_Reg_SignedImm(0xA0, &highAddr, high);
		}

		return true;
	}

	return false;
}


static bool 
handle_Bitwise(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	ensureSource(&destination, &source);

	if ((source->mode & MODE_REG_8BIT_FBCDEHL) && (destination->mode & (MODE_NONE | MODE_REG_T)))
		return handle_OpcodeRegister(baseOpcode, source);
	else if ((source->mode & MODE_IMM) && (destination->mode & (MODE_NONE | MODE_REG_T))) {
		baseOpcode = 0xB0 + ((baseOpcode >> 3) & 0x03);
		return handle_Op_Reg_SignedOrUnsignedImm(baseOpcode, NULL, source->expression);
	}

	return false;
}


static bool 
handle_EXG(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	ensureSource(&destination, &source);

	if ((destination->mode & MODE_REG_8BIT_FBCDEHL) && (source->mode & MODE_REG_T)) {
		return handle_OpcodeRegister(baseOpcode, destination);
	} else if ((source->mode & MODE_REG_8BIT_FBCDEHL) && (destination->mode & (MODE_NONE | MODE_REG_T))) {
		return handle_OpcodeRegister(baseOpcode, source);
	} else if ((source->mode & MODE_REG_8BIT_FBCDEHL) && (destination->mode & MODE_REG_8BIT_FBCDEHL)) {
		if (!opt_Current->machineOptions->enableSynthInstructions)
			return err_Error(MERROR_REQUIRES_SYNTHESIZED);

		return handle_OpcodeRegister(baseOpcode, source)
			&& handle_OpcodeRegister(baseOpcode, destination)
			&& handle_OpcodeRegister(baseOpcode, source);
	} else if ((destination->mode & MODE_REG_16BIT_BCDEHL) && (source->mode & MODE_REG_FT)) {
		return handle_OpcodeRegister(0xC8, destination);
	} else if ((source->mode & MODE_REG_16BIT_BCDEHL) && (destination->mode & (MODE_NONE | MODE_REG_FT))) {
		return handle_OpcodeRegister(0xC8, source);
	} else if ((source->mode & MODE_REG_16BIT_BCDEHL) && (destination->mode & MODE_REG_16BIT_BCDEHL)) {
		if (!opt_Current->machineOptions->enableSynthInstructions)
			return err_Error(MERROR_REQUIRES_SYNTHESIZED);

		return handle_OpcodeRegister(0xC8, source)
			&& handle_OpcodeRegister(0xC8, destination)
			&& handle_OpcodeRegister(0xC8, source);
	}

	return false;
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
	return handle_JumpRelative(0x90 + cc, destination);
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
			sect_OutputConst8(0x90 + cc);
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
		return handle_Implicit(baseOpcode, cc, destination, source);
	else if (destination->mode == MODE_REG_T && source->mode == MODE_IND_C)
		return handle_Implicit(baseOpcode + 0x01, cc, destination, source);

	return false;
}

static bool 
handle_LD_R8_const(SAddressingMode* reg, uint8_t n8) {
	if (!handle_OpcodeRegister(0x80, reg))
		return false;

	sect_OutputConst8(n8);
	return true;
}


static bool 
handle_LD_R16_3bytes(SAddressingMode* registerHigh, SAddressingMode* registerLow, uint8_t high, uint8_t low) {
	if (registerHigh->registerIndex == 0 && registerLow->registerIndex == 1) {
		if (((high == 0xFF) && (low & 0x80))
		||  ((high == 0x00) && (low & 0x80) == 0)) {
			handle_LD_R8_const(registerLow, low);	/* LD T,n8 */
			sect_OutputConst8(0x49);				/* EXT */
			return true;
		} else if (high == low) {
			handle_LD_R8_const(registerLow, low);	/* LD r8,n8 */
			sect_OutputConst8(0x78);				/* LD F,T */
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

		if (handle_LD_R16_3bytes(&registerHigh, &registerLow, high, low)
		|| handle_LD_R16_4bytes(&registerHigh, &registerLow, high, low)) {
			expr_Free(masked);
			return true;
		}
	}

	SExpression* high = expr_Asr(expr_Clone(masked), expr_Const(8));
	SExpression* low = expr_And(masked, expr_Const(0xFF));

	if (high == NULL || low == NULL)
		return false;

	if (!handle_Op_Reg_UnsignedImm(0x80, &registerHigh, high))
		return false;
	return handle_Op_Reg_UnsignedImm(0x80, &registerLow, low);
}


static bool 
handle_LD(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	if ((destination->mode & MODE_REG_8BIT) && source->mode == MODE_IMM) {
		return handle_Op_Reg_SignedOrUnsignedImm(0x80, destination, source->expression);
	} else if ((destination->mode & MODE_REG_16BIT) && source->mode == MODE_IMM && opt_Current->machineOptions->enableSynthInstructions) {
		return handle_LD_R16_imm(destination, source->expression);
	} else if ((destination->mode & MODE_IND_16BIT_BCDEHL) && source->mode == MODE_REG_T) {
		return handle_OpcodeRegister(0x00, destination);
	} else if ((destination->mode == MODE_IND_FT) && (source->mode & MODE_REG_8BIT_BCDEHL)) {
		return handle_OpcodeRegister(0x20, source);
	} else if (destination->mode == MODE_REG_T && (source->mode & MODE_IND_16BIT)) {
		return handle_OpcodeRegister(0x04, source);
	} else if ((destination->mode & MODE_REG_8BIT_FBCDEHL) && (source->mode == MODE_IND_FT)) {
		return handle_OpcodeRegister(0x28, destination);
	} else if ((destination->mode & MODE_REG_8BIT_FBCDEHL) && source->mode == MODE_REG_T) {
		return handle_OpcodeRegister(0x78, destination);
	} else if (destination->mode == MODE_REG_T && (source->mode & MODE_REG_8BIT_FBCDEHL)) {
		return handle_OpcodeRegister(0x58, source);
	} else if ((destination->mode & MODE_REG_16BIT_BCDEHL) && source->mode == MODE_REG_FT) {
		return handle_OpcodeRegister(0xD0, destination);
	} else if (destination->mode == MODE_REG_FT && (source->mode & MODE_REG_16BIT_BCDEHL)) {
		return handle_OpcodeRegister(0xD8, source);
	} else if (destination->mode == MODE_REG_FT && (source->mode & MODE_IND_16BIT_BCDEHL) && opt_Current->machineOptions->enableSynthInstructions) {
		// LD FT,(hl)
		handle_OpcodeRegister(0x04, source);	// LD T,(hl)
		handle_OpcodeRegister(0xBC, source);	// ADD hl,
		sect_OutputConst8(1);					//        1
		sect_OutputConst8(0x10);				// EXG F,T
		handle_OpcodeRegister(0x04, source);	// LD T,(hl)
		handle_OpcodeRegister(0xBC, source);	// ADD hl,
		sect_OutputConst8(-1);					//        -1
		sect_OutputConst8(0x10);				// EXG F,T
		return true;
	} else if ((destination->mode & MODE_IND_16BIT_BCDEHL) && source->mode == MODE_REG_FT && opt_Current->machineOptions->enableSynthInstructions) {
		// LD (hl),FT
		handle_OpcodeRegister(0x00, destination);	// LD (hl),T
		handle_OpcodeRegister(0xBC, destination);	// ADD hl,
		sect_OutputConst8(1);						//        1
		sect_OutputConst8(0x10);					// EXG F,T
		handle_OpcodeRegister(0x00, destination);	// LD T,(hl)
		handle_OpcodeRegister(0xBC, destination);	// ADD hl,
		sect_OutputConst8(-1);						//        -1
		sect_OutputConst8(0x10);					// EXG F,T
		return true;
	} else if (destination->mode == MODE_REG_FT && (source->mode & MODE_IND_BCDEHL_POST_INC) && opt_Current->machineOptions->enableSynthInstructions) {
		// LD FT,(hl+)
		handle_OpcodeRegister(0x04, source);	// LD T,(hl)
		handle_OpcodeRegister(0xBC, source);	// ADD hl,
		sect_OutputConst8(1);					//        1
		sect_OutputConst8(0x10);				// EXG F,T
		handle_OpcodeRegister(0x04, source);	// LD T,(hl)
		sect_OutputConst8(0x10);				// EXG F,T
		return true;
	} else if (destination->mode == MODE_REG_T && (source->mode & MODE_IND_BCDEHL_POST_INC) && opt_Current->machineOptions->enableSynthInstructions) {
		// LD T,(hl+)
		handle_OpcodeRegister(0x04, source);	// LD T,(hl)
		handle_OpcodeRegister(0xBC, source);	// ADD hl,
		sect_OutputConst8(1);					//        1
		return true;
	} else if ((destination->mode & MODE_IND_BCDEHL_POST_INC) && source->mode == MODE_REG_FT && opt_Current->machineOptions->enableSynthInstructions) {
		// LD (hl+),FT
		handle_OpcodeRegister(0x00, destination);	// LD (hl),T
		handle_OpcodeRegister(0xBC, destination);	// ADD hl,
		sect_OutputConst8(1);						//        1
		sect_OutputConst8(0x10);					// EXG F,T
		handle_OpcodeRegister(0x00, destination);	// LD (hl),T
		sect_OutputConst8(0x10);					// EXG F,T
		return true;
	} else if ((destination->mode & MODE_IND_BCDEHL_POST_INC) && source->mode == MODE_REG_T && opt_Current->machineOptions->enableSynthInstructions) {
		// LD (hl+),T
		handle_OpcodeRegister(0x00, destination);	// LD (hl),T
		handle_OpcodeRegister(0xBC, destination);	// ADD hl,
		sect_OutputConst8(1);						//        1
		return true;
	} else if (destination->mode == MODE_REG_FT && (source->mode & MODE_IND_BCDEHL_PRE_DEC) && opt_Current->machineOptions->enableSynthInstructions) {
		// LD FT,(-hl)
		handle_OpcodeRegister(0x04, source);	// LD T,(hl)
		handle_OpcodeRegister(0xBC, source);	// ADD hl,
		sect_OutputConst8(-1);					//        -1
		sect_OutputConst8(0x10);				// EXG F,T
		handle_OpcodeRegister(0x04, source);	// LD T,(hl)
		return true;
	} else if (destination->mode == MODE_REG_T && (source->mode & MODE_IND_BCDEHL_PRE_DEC) && opt_Current->machineOptions->enableSynthInstructions) {
		// LD T,(-hl)
		handle_OpcodeRegister(0xBC, source);	// ADD hl,
		sect_OutputConst8(-1);					//        -1
		handle_OpcodeRegister(0x04, source);	// LD T,(hl)
		return true;
	} else if ((destination->mode & MODE_IND_BCDEHL_PRE_DEC) && source->mode == MODE_REG_FT && opt_Current->machineOptions->enableSynthInstructions) {
		// LD (-hl),FT
		sect_OutputConst8(0x10);					// EXG F,T
		handle_OpcodeRegister(0x00, destination);	// LD (hl),T
		handle_OpcodeRegister(0xBC, destination);	// ADD hl,
		sect_OutputConst8(-1);						//        -1
		sect_OutputConst8(0x10);					// EXG F,T
		handle_OpcodeRegister(0x00, destination);	// LD (hl),T
		return true;
	} else if ((destination->mode & MODE_IND_BCDEHL_PRE_DEC) && source->mode == MODE_REG_T && opt_Current->machineOptions->enableSynthInstructions) {
		// LD (-hl),T
		handle_OpcodeRegister(0xBC, destination);	// ADD hl,
		sect_OutputConst8(-1);						//        -1
		handle_OpcodeRegister(0x00, destination);	// LD (hl),T
		return true;
	} else if ((destination->mode & MODE_REG_16BIT_BCDEHL) && source->mode == MODE_IND_FT && opt_Current->machineOptions->enableSynthInstructions) {
		// LD hl,(FT)
		SAddressingMode high, low;
		registerPair(&high, &low, destination);
		handle_OpcodeRegister(0x28, &low);		// LD l,(FT)
		handle_OpcodeRegister(0xBC, source);	// ADD FT,
		sect_OutputConst8(1);					//        1
		handle_OpcodeRegister(0x28, &high);		// LD h,(FT)
		handle_OpcodeRegister(0xBC, source);	// ADD FT,
		sect_OutputConst8(-1);					//        -1
		return true;
	} else if (destination->mode == MODE_IND_FT && (source->mode & MODE_REG_16BIT_BCDEHL) && opt_Current->machineOptions->enableSynthInstructions) {
		// LD (FT),hl
		SAddressingMode high, low;
		registerPair(&high, &low, source);
		handle_OpcodeRegister(0x20, &low);			// LD (FT),l
		handle_OpcodeRegister(0xBC, destination);	// ADD FT,
		sect_OutputConst8(1);						//        1
		handle_OpcodeRegister(0x20, &high);			// LD (FT),h
		handle_OpcodeRegister(0xBC, destination);	// ADD FT,
		sect_OutputConst8(-1);						//        -1
		return true;
	} else if ((destination->mode & MODE_REG_16BIT_BCDEHL) && source->mode == MODE_IND_FT_POST_INC && opt_Current->machineOptions->enableSynthInstructions) {
		// LD hl,(FT+)
		SAddressingMode high, low;
		registerPair(&high, &low, destination);
		handle_OpcodeRegister(0x28, &low);		// LD l,(FT)
		handle_OpcodeRegister(0xBC, source);	// ADD FT,
		sect_OutputConst8(1);					//        1
		handle_OpcodeRegister(0x28, &high);		// LD h,(FT)
		return true;
	} else if ((destination->mode & MODE_REG_8BIT_FBCDEHL) && source->mode == MODE_IND_FT_POST_INC && opt_Current->machineOptions->enableSynthInstructions) {
		// LD r,(FT+)
		handle_OpcodeRegister(0x28, destination);	// LD r,(FT)
		handle_OpcodeRegister(0xBC, source);		// ADD FT,
		sect_OutputConst8(1);						//        1
		return true;
	} else if (destination->mode == MODE_IND_FT_POST_INC && (source->mode & MODE_REG_16BIT_BCDEHL) && opt_Current->machineOptions->enableSynthInstructions) {
		// LD (FT+),hl
		SAddressingMode high, low;
		registerPair(&high, &low, source);
		handle_OpcodeRegister(0x20, &low);			// LD (FT),l
		handle_OpcodeRegister(0xBC, destination);	// ADD FT,
		sect_OutputConst8(1);						//        1
		handle_OpcodeRegister(0x20, &high);			// LD (FT),l
		return true;
	} else if (destination->mode == MODE_IND_FT_POST_INC && (source->mode & MODE_REG_8BIT_FBCDEHL) && opt_Current->machineOptions->enableSynthInstructions) {
		// LD (FT+),r
		handle_OpcodeRegister(0x20, source);		// LD (FT),l
		handle_OpcodeRegister(0xBC, destination);	// ADD FT,
		sect_OutputConst8(1);						//        1
		return true;
	} else if ((destination->mode & MODE_REG_16BIT_BCDEHL) && source->mode == MODE_IND_FT_PRE_DEC && opt_Current->machineOptions->enableSynthInstructions) {
		// LD hl,(-FT)
		SAddressingMode high, low;
		registerPair(&high, &low, destination);
		handle_OpcodeRegister(0x28, &high);		// LD h,(FT)
		handle_OpcodeRegister(0xBC, source);	// ADD FT,
		sect_OutputConst8(-1);					//        -1
		handle_OpcodeRegister(0x28, &low);		// LD l,(FT)
		return true;
	} else if ((destination->mode & MODE_REG_8BIT_FBCDEHL) && source->mode == MODE_IND_FT_PRE_DEC && opt_Current->machineOptions->enableSynthInstructions) {
		// LD r,(-FT)
		handle_OpcodeRegister(0xBC, source);		// ADD FT,
		sect_OutputConst8(-1);						//        -1
		handle_OpcodeRegister(0x28, destination);	// LD r,(FT)
		return true;
	} else if (destination->mode == MODE_IND_FT_PRE_DEC && (source->mode & MODE_REG_16BIT_BCDEHL) && opt_Current->machineOptions->enableSynthInstructions) {
		// LD (-FT),hl
		SAddressingMode high, low;
		registerPair(&high, &low, source);
		handle_OpcodeRegister(0x20, &high);			// LD h,(FT)
		handle_OpcodeRegister(0xBC, destination);	// ADD FT,
		sect_OutputConst8(-1);						//        -1
		handle_OpcodeRegister(0x20, &low);			// LD l,(FT)
		return true;
	} else if (destination->mode == MODE_IND_FT_PRE_DEC && (source->mode & MODE_REG_8BIT_FBCDEHL) && opt_Current->machineOptions->enableSynthInstructions) {
		// LD (-FT),r
		handle_OpcodeRegister(0xBC, destination);	// ADD FT,
		sect_OutputConst8(-1);						//        -1
		handle_OpcodeRegister(0x20, source);		// LD r,(FT)
		return true;
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
	if ((destination->mode & MODE_IND_16BIT_BCDEHL) && source->mode == MODE_REG_T)
		return handle_OpcodeRegister(baseOpcode, destination);
	else if (destination->mode == MODE_REG_T && (source->mode & MODE_IND_16BIT))
		return handle_OpcodeRegister(baseOpcode + 0x04, source);

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
			
		bool isPop = baseOpcode == 0xC4;
		bool isSwap = baseOpcode == 0xDC;
		uint8_t all = isSwap ? 0xCC : isPop ? 0xF9 : 0xF8;
		uint8_t oppositeOne = isSwap ? baseOpcode : baseOpcode ^ 0x04;

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
handle_PICK(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	if (source->mode == MODE_IMM) {
		SExpression* masked = expr_CheckRange(source->expression, 0, 255);

		SAddressingMode high, low;
		registerPair(&high, &low, destination);
		handle_OpcodeRegister(0x80, &low);	// LD l,
		sect_OutputExpr8(masked);			//      x
	}

	handle_OpcodeRegister(baseOpcode, destination);
	return true;
}


static bool
handle_Shift(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	ensureSource(&destination, &source);

	if ((destination->mode & (MODE_REG_FT | MODE_NONE)) && (source->mode & MODE_REG_8BIT_BCDEHL))
		return handle_OpcodeRegister(baseOpcode, source);
	else if ((destination->mode & (MODE_REG_FT | MODE_NONE)) && (source->mode & MODE_IMM)) {
		baseOpcode = 0xB8 + ((baseOpcode >> 3) & 0x03);
		return handle_Op_Reg_UnsignedImm4(baseOpcode, NULL, source->expression);
	}

	return false;
}


static bool
handle_CMP(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	ensureSource(&destination, &source);

	if ((destination->mode & MODE_REG_8BIT) && (source->mode & MODE_IMM)) {
		handle_OpcodeRegister(0xA8, destination);
		sect_OutputExpr8(source->expression);
		return true;
	} else if ((destination->mode & (MODE_REG_T | MODE_NONE)) && (source->mode & MODE_REG_8BIT_FBCDEHL)) {
		return handle_OpcodeRegister(0x48, source);
	} else if ((destination->mode & (MODE_REG_FT | MODE_NONE)) && (source->mode & MODE_REG_16BIT_BCDEHL)) {
		return handle_OpcodeRegister(0xCC, source);
	}

	return false;
}


static bool
handle_NOT(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	if (destination->mode & MODE_REG_T) {
		if (!opt_Current->machineOptions->enableSynthInstructions)
			return err_Error(MERROR_REQUIRES_SYNTHESIZED);
		sect_OutputConst8(0xB2);	// XOR T,n8
		sect_OutputConst8(0xFF);
		return true;
	} else if (destination->mode & MODE_REG_FT) {
		if (!opt_Current->machineOptions->enableSynthInstructions)
			return err_Error(MERROR_REQUIRES_SYNTHESIZED);
		sect_OutputConst8(0xB2);	// XOR T,n8
		sect_OutputConst8(0xFF);
		sect_OutputConst8(baseOpcode);
		return true;
	} else if (destination->mode & MODE_REG_F) {
		sect_OutputConst8(baseOpcode);
		return true;
	} 
	return true;
}


static bool
handle_TST(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	return handle_OpcodeRegister(baseOpcode, destination);
}


static bool
handle_NEG(uint8_t baseOpcode, EConditionCode cc, SAddressingMode* destination, SAddressingMode* source) {
	if (destination->mode & MODE_REG_16BIT)
		baseOpcode = 0xF0;

	sect_OutputConst8(baseOpcode);

	return true;
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

	if ((source->mode & MODE_REG_8BIT) && (destination->mode & (MODE_NONE | MODE_REG_T)))
		return handle_OpcodeRegister(0x50, source);
	else if ((source->mode & MODE_REG_16BIT_BCDEHL) && (destination->mode & (MODE_NONE | MODE_REG_FT)))
		return handle_OpcodeRegister(0xF0, source);
	else if ((destination->mode & (MODE_REG_8BIT | MODE_REG_16BIT)) && (source->mode & MODE_IMM)) {
		if (!opt_Current->machineOptions->enableSynthInstructions)
			return err_Error(MERROR_REQUIRES_SYNTHESIZED);
		source->expression = expr_Sub(expr_Const(0), source->expression);
		return handle_ADD(0x00, cc, destination, source);
	}

	return false;
}


static SParser
g_Parsers[T_RC8_XOR - T_RC8_ADD + 1] = {
	{ 0x00, CONDITION_AUTO, MODE_REG_8BIT | MODE_REG_16BIT, MODE_IMM | MODE_REG_8BIT | MODE_REG_16BIT | MODE_NONE, handle_ADD },	/* ADD */
	{ 0x68, CONDITION_AUTO, MODE_REG_8BIT | MODE_IMM, MODE_NONE | MODE_REG_8BIT | MODE_IMM, handle_Bitwise },	/* AND */
	{ 0x00, CONDITION_AUTO, MODE_REG_8BIT | MODE_REG_16BIT, MODE_IMM | MODE_REG_8BIT_FBCDEHL | MODE_REG_16BIT_BCDEHL | MODE_NONE, handle_CMP },		/* CMP */
	{ 0x1B, CONDITION_AUTO, MODE_NONE, MODE_NONE, handle_Implicit },								/* DI */
	{ 0x88, CONDITION_AUTO, MODE_REG_8BIT, MODE_ADDR, handle_DJ },									/* DJ */
	{ 0x1A, CONDITION_AUTO, MODE_NONE, MODE_NONE, handle_Implicit },								/* EI */
	{ 0x10, CONDITION_AUTO, MODE_REG_8BIT | MODE_REG_16BIT, MODE_NONE | MODE_REG_8BIT | MODE_REG_16BIT, handle_EXG },	/* EXG */
	{ 0x49, CONDITION_AUTO, MODE_NONE, MODE_NONE, handle_Implicit },								/* EXT */
	{ 0x3C, CONDITION_PASS, MODE_IND_16BIT | MODE_ADDR, MODE_NONE, handle_J },						/* J */
	{ 0x38, CONDITION_AUTO, MODE_IND_16BIT | MODE_ADDR, MODE_NONE, handle_JAL },					/* JAL */
	{ 0x0C, CONDITION_AUTO, MODE_REG_T, MODE_IND_16BIT, handle_LCO },								/* LCO */
	{ 0x0A, CONDITION_AUTO, MODE_IND_C | MODE_REG_T, MODE_IND_C | MODE_REG_T, handle_LCR },			/* LCR */
	{ 0x00, CONDITION_AUTO, MODE_REG_8BIT | MODE_REG_16BIT | MODE_IND_16BIT | MODE_IND_16BIT_INC_DEC, MODE_IMM | MODE_REG_8BIT | MODE_REG_16BIT | MODE_IND_16BIT | MODE_IND_16BIT_INC_DEC, handle_LD },		/* LD */
	{ 0x30, CONDITION_AUTO, MODE_IND_16BIT_BCDEHL | MODE_REG_T, MODE_IND_16BIT | MODE_REG_T, handle_LoadIndirect },	/* LIO */
	{ 0xE0, CONDITION_AUTO, MODE_REG_FT | MODE_REG_8BIT_BCDEHL | MODE_IMM, MODE_REG_8BIT_BCDEHL | MODE_IMM | MODE_NONE, handle_Shift },	/* LS */
	{ 0x51, CONDITION_AUTO, MODE_REG_T | MODE_REG_FT, MODE_NONE, handle_NEG },						/* NEG */
	{ 0x00, CONDITION_AUTO, MODE_NONE, MODE_NONE, handle_Implicit },								/* NOP */
	{ 0x18, CONDITION_AUTO, MODE_REG_F | MODE_REG_T | MODE_REG_FT, MODE_NONE, handle_NOT },						/* NOT */
	{ 0x60, CONDITION_AUTO, MODE_REG_8BIT | MODE_IMM, MODE_NONE | MODE_REG_8BIT | MODE_IMM, handle_Bitwise },	/* OR */
	{ 0x1C, CONDITION_AUTO, MODE_REG_16BIT, MODE_NONE | MODE_IMM, handle_PICK },					/* PICK */
	{ 0xC4, CONDITION_AUTO, MODE_REG_16BIT | MODE_REGISTER_MASK, MODE_NONE, handle_RegisterStack },	/* POP */
	{ 0xF9, CONDITION_AUTO, MODE_NONE, MODE_NONE, handle_Implicit },								/* POPA */
	{ 0xC0, CONDITION_AUTO, MODE_REG_16BIT | MODE_REGISTER_MASK, MODE_NONE, handle_RegisterStack },	/* PUSH */
	{ 0xF8, CONDITION_AUTO, MODE_NONE, MODE_NONE, handle_Implicit },								/* PUSHA */
	{ 0x59, CONDITION_AUTO, MODE_NONE, MODE_NONE, handle_Implicit },								/* RETI */
	{ 0xE8, CONDITION_AUTO, MODE_REG_FT | MODE_REG_8BIT_BCDEHL | MODE_IMM, MODE_REG_8BIT_BCDEHL | MODE_IMM | MODE_NONE, handle_Shift },	/* RS */
	{ 0xF8, CONDITION_AUTO, MODE_REG_FT | MODE_REG_8BIT_BCDEHL | MODE_IMM, MODE_REG_8BIT_BCDEHL | MODE_IMM | MODE_NONE, handle_Shift },	/* RSA */
	{ 0x00, CONDITION_AUTO, MODE_REG_8BIT | MODE_REG_16BIT, MODE_NONE | MODE_REG_8BIT | MODE_REG_16BIT | MODE_IMM, handle_SUB },	/* SUB */
	{ 0xDC, CONDITION_AUTO, MODE_REG_16BIT | MODE_REGISTER_MASK, MODE_NONE, handle_RegisterStack },	/* SWAP */
	{ 0xCC, CONDITION_AUTO, MODE_NONE, MODE_NONE, handle_Implicit },								/* SWAPA */
	{ 0x9B, CONDITION_AUTO, MODE_IMM, MODE_NONE, handle_SYS },										/* SYS */
	{ 0xD4, CONDITION_AUTO, MODE_REG_16BIT, MODE_NONE, handle_TST },								/* TST */
	{ 0x70, CONDITION_AUTO, MODE_REG_8BIT | MODE_IMM, MODE_NONE | MODE_REG_8BIT | MODE_IMM, handle_Bitwise },	/* XOR */
};


typedef struct RegisterMode {
	uint32_t modeFlag;
	uint16_t token;
	uint8_t opcodeRegister;
} SRegisterMode;


static SRegisterMode 
g_RegisterModes[T_RC8_REG_HL_IND_PRE_DEC - T_RC8_REG_F + 1] = {
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
	{ MODE_IND_FT_POST_INC, T_RC8_REG_FT_IND_POST_INC, 0 },
	{ MODE_IND_BCDEHL_POST_INC, T_RC8_REG_BC_IND_POST_INC, 1 },
	{ MODE_IND_BCDEHL_POST_INC, T_RC8_REG_DE_IND_POST_INC, 2 },
	{ MODE_IND_BCDEHL_POST_INC, T_RC8_REG_HL_IND_POST_INC, 3 },
	{ MODE_IND_FT_PRE_DEC, T_RC8_REG_FT_IND_PRE_DEC, 0 },
	{ MODE_IND_BCDEHL_PRE_DEC, T_RC8_REG_BC_IND_PRE_DEC, 1 },
	{ MODE_IND_BCDEHL_PRE_DEC, T_RC8_REG_DE_IND_PRE_DEC, 2 },
	{ MODE_IND_BCDEHL_PRE_DEC, T_RC8_REG_HL_IND_PRE_DEC, 3 },
};


static bool
parseAddressingMode(SAddressingMode* addrMode, int allowedModes) {
	SLexerContext bm;
	lex_Bookmark(&bm);

	if (allowedModes & MODE_REGISTER_MASK) {
		uint8_t mask = 0;
		uint8_t registers = 0;

		while (lex_Context->token.id >= T_RC8_REG_FT && lex_Context->token.id <= T_RC8_REG_HL) {
			uint32_t firstToken = lex_Context->token.id;

			mask |= 1 << (firstToken - T_RC8_REG_FT);
			registers += 1;

			parse_GetToken();

			if (lex_Context->token.id == T_OP_SUBTRACT) {
				parse_GetToken();
				if (lex_Context->token.id > firstToken && lex_Context->token.id <= T_RC8_REG_HL) {
					uint32_t lastToken = lex_Context->token.id;

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

			if (lex_Context->token.id == T_OP_DIVIDE) {
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

	if (lex_Context->token.id >= T_RC8_REG_F && lex_Context->token.id <= T_RC8_REG_HL_IND_PRE_DEC) {
		SRegisterMode* mode = &g_RegisterModes[lex_Context->token.id - T_RC8_REG_F];
		if (allowedModes & mode->modeFlag) {
			parse_GetToken();
			addrMode->mode = mode->modeFlag;
			addrMode->registerIndex = mode->opcodeRegister;
			return true;
		}
	}
	
	if (lex_Context->token.id == '(' && (allowedModes & MODE_IND_16BIT)) {
		parse_GetToken();
		if (lex_Context->token.id >= T_RC8_REG_FT && lex_Context->token.id <= T_RC8_REG_HL) {
			int ind_token = lex_Context->token.id - T_RC8_REG_FT + T_RC8_REG_FT_IND;
			parse_GetToken();
			if (lex_Context->token.id == ')') {
				parse_GetToken();
				SRegisterMode* mode = &g_RegisterModes[ind_token - T_RC8_REG_F];
				if (allowedModes & mode->modeFlag) {
					addrMode->mode = mode->modeFlag;
					addrMode->registerIndex = mode->opcodeRegister;
					return true;
				}
			}
		}
		lex_Goto(&bm);
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
	if (T_RC8_ADD <= lex_Context->token.id && lex_Context->token.id <= T_RC8_XOR) {
		SAddressingMode addrMode1;
		SAddressingMode addrMode2;
		ETargetToken token = lex_Context->token.id;
		SParser* parser = &g_Parsers[token - T_RC8_ADD];
		EConditionCode cc = CC_ALWAYS;

		parse_GetToken();

		if (lex_Context->token.id == T_OP_DIVIDE) {
			parse_GetToken();

			if (lex_Context->token.id == T_RC8_REG_C) {
				cc = CC_LTU;
			} else if (lex_Context->token.id >= T_RC8_CC_LE && lex_Context->token.id <= T_RC8_CC_NE) {
				cc = lex_Context->token.id - T_RC8_CC_LE;
			} else {
				err_Error(MERROR_EXPECTED_CONDITION_CODE);
				return false;
			}
			parse_GetToken();
		}

		if (parseAddressingMode(&addrMode1, parser->firstModes)) {
			string* jumpTarget = NULL;

			if (lex_Context->token.id == ',') {
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
				str_Free(jumpTarget);
			}
		} else {
			return err_Error(ERROR_OPERAND);
		}

	}

	return false;
}
