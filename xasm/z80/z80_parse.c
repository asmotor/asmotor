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
#include <stdlib.h>

#include "xasm.h"
#include "expression.h"
#include "lexer.h"
#include "options.h"
#include "parse.h"
#include "parse_expression.h"
#include "errors.h"
#include "section.h"

#include "z80_errors.h"
#include "z80_parse.h"
#include "z80_tokens.h"
#include "z80_options.h"

typedef enum {
    REG_D_NONE = -1,
    REG_D_B = 0,
    REG_D_C,
    REG_D_D,
    REG_D_E,
    REG_D_H,
    REG_D_IXH = REG_D_H,
    REG_D_IYH = REG_D_H,
    REG_D_L,
    REG_D_IXL = REG_D_L,
    REG_D_IYL = REG_D_L,
    REG_D_HL_IND,
    REG_D_A
} ERegisterD;

typedef enum {
    REG_SS_NONE = -1,
    REG_SS_BC = 0,
    REG_SS_DE = 1,
    REG_SS_HL = 2,
    REG_SS_SP = 3,
    REG_SS_AF = 3
} ERegisterSS;

typedef enum {
    REG_RR_NONE = -1,
    REG_RR_BC_IND = 0,
    REG_RR_DE_IND,
    REG_RR_HL_IND_INC,
    REG_RR_HL_IND_DEC
} ERegisterRR;

typedef enum {
    REG_HL_NONE = -1,
    REG_HL_HL = 0,
    REG_HL_IX = 0xDD,
    REG_HL_IY = 0xFD
} ERegisterHL;

typedef enum {
    CC_NONE = -1,
    CC_NZ = 0,
    CC_Z,
    CC_NC,
    CC_C,
    CC_PO,  // Z80 only
    CC_PE,  // Z80 only
    CC_P,   // Z80 only
    CC_M,   // Z80 only
} EModeF;

typedef enum {
    CTRL_NONE = -1,
    CTRL_I,
    CTRL_R
} EModeCtrl;

typedef struct _AddrMode {
    uint32_t mode;
    SExpression* expression;
    uint8_t cpu;
    ERegisterD registerD;
    ERegisterSS registerSS;
    ERegisterRR registerRR;
    ERegisterHL registerHL;
    EModeF modeF;
    EModeCtrl registerCtrl;
} SAddressingMode;

typedef struct _Opcode {
    uint8_t cpu;
    uint8_t prefix;
    uint8_t opcode;
    uint32_t allowedModes1;
    uint32_t allowedModes2;
    bool (* handler)(struct _Opcode* code, SAddressingMode* mode1, SAddressingMode* mode2);
} SInstruction;

static SInstruction g_instructions[T_Z80_XOR - T_Z80_ADC + 1];

#define MODE_NONE            0x00000001u
#define MODE_REG_A           0x00000002u
#define MODE_REG_C_IND       0x00000004u
#define MODE_REG_DE          0x00000008u
#define MODE_REG_HL          0x00000010u
#define MODE_REG_SP          0x00000020u
#define MODE_REG_AF          0x00000040u
#define MODE_REG_AF_SEC      0x00000080u
#define MODE_REG_BC_IND      0x00000100u
#define MODE_REG_DE_IND      0x00000200u
#define MODE_REG_HL_IND      0x00000400u
#define MODE_REG_HL_IND_DEC  0x00000800u
#define MODE_REG_HL_IND_INC  0x00001000u
#define MODE_REG_SP_IND      0x00002000u
#define MODE_REG_IX_IND      0x00004000u
#define MODE_REG_IY_IND      0x00008000u
#define MODE_REG_SP_DISP     0x00010000u
#define MODE_REG_IX_IND_DISP 0x00020000u
#define MODE_REG_IY_IND_DISP 0x00040000u
#define MODE_GROUP_D         0x00080000u
#define MODE_GROUP_SS        0x00100000u
#define MODE_GROUP_SS_AF     0x00200000u
#define MODE_GROUP_RR        0x00400000u
#define MODE_GROUP_HL        0x00800000u
#define MODE_IMM             0x01000000u
#define MODE_IMM_IND         0x02000000u
#define MODE_CC_GB           0x04000000u
#define MODE_CC_Z80          0x08000000u
#define MODE_REG_CONTROL     0x10000000u
#define MODE_GROUP_IXYLH       0x20000000u

static ETargetToken g_registerPairsSS[3][2] = {
	{ T_MODE_B, T_MODE_C },
	{ T_MODE_D, T_MODE_E },
	{ T_MODE_H, T_MODE_L }
};

static SAddressingMode g_addressModes[T_CC_M - T_MODE_B + 1] = {
	{ MODE_GROUP_D, NULL, CPUF_Z80 | CPUF_GB, REG_D_B, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_NONE, CTRL_NONE },	// B
	{ MODE_GROUP_D | MODE_CC_GB | MODE_CC_Z80, NULL, CPUF_Z80 | CPUF_GB, REG_D_C, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_C, CTRL_NONE },	// C
	{ MODE_GROUP_D, NULL, CPUF_Z80 | CPUF_GB, REG_D_D, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_NONE, CTRL_NONE },	// D
	{ MODE_GROUP_D, NULL, CPUF_Z80 | CPUF_GB, REG_D_E, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_NONE, CTRL_NONE },	// E
	{ MODE_GROUP_D, NULL, CPUF_Z80 | CPUF_GB, REG_D_H, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_NONE, CTRL_NONE },	// H
	{ MODE_GROUP_D, NULL, CPUF_Z80 | CPUF_GB, REG_D_L, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_NONE, CTRL_NONE },	// L
	{ MODE_REG_HL_IND | MODE_GROUP_D, NULL, CPUF_Z80 | CPUF_GB, REG_D_HL_IND, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_NONE, CTRL_NONE }, // (HL)
	{ MODE_REG_A | MODE_GROUP_D, NULL, CPUF_Z80 | CPUF_GB, REG_D_A, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_NONE, CTRL_NONE }, // A
	{ MODE_GROUP_IXYLH, NULL, CPUF_Z80, REG_D_H, REG_SS_NONE, REG_RR_NONE, REG_HL_IX, CC_NONE, CTRL_NONE },	// IXH
	{ MODE_GROUP_IXYLH, NULL, CPUF_Z80, REG_D_L, REG_SS_NONE, REG_RR_NONE, REG_HL_IX, CC_NONE, CTRL_NONE },	// IXL
	{ MODE_GROUP_IXYLH, NULL, CPUF_Z80, REG_D_H, REG_SS_NONE, REG_RR_NONE, REG_HL_IY, CC_NONE, CTRL_NONE },	// IYH
	{ MODE_GROUP_IXYLH, NULL, CPUF_Z80, REG_D_L, REG_SS_NONE, REG_RR_NONE, REG_HL_IY, CC_NONE, CTRL_NONE },	// IYL
	{ MODE_GROUP_SS | MODE_GROUP_SS_AF, NULL, CPUF_Z80 | CPUF_GB, REG_D_NONE, REG_SS_BC, REG_RR_NONE, REG_HL_NONE, CC_NONE, CTRL_NONE },	// BC
	{ MODE_REG_DE | MODE_GROUP_SS | MODE_GROUP_SS_AF, NULL, CPUF_Z80 | CPUF_GB, REG_D_NONE, REG_SS_DE, REG_RR_NONE, REG_HL_NONE, CC_NONE, CTRL_NONE },	// DE
	{ MODE_REG_HL | MODE_GROUP_SS | MODE_GROUP_SS_AF | MODE_GROUP_HL, NULL, CPUF_Z80 | CPUF_GB, REG_D_NONE, REG_SS_HL, REG_RR_NONE, REG_HL_HL, CC_NONE, CTRL_NONE },	// HL
	{ MODE_REG_SP | MODE_GROUP_SS, NULL, CPUF_Z80 | CPUF_GB, REG_D_NONE, REG_SS_SP, REG_RR_NONE, REG_HL_NONE, CC_NONE, CTRL_NONE },	// SP
	{ MODE_GROUP_HL, NULL, CPUF_Z80, REG_D_NONE, REG_SS_HL, REG_RR_NONE, REG_HL_IX, CC_NONE, CTRL_NONE },	// IX
	{ MODE_GROUP_HL, NULL, CPUF_Z80, REG_D_NONE, REG_SS_HL, REG_RR_NONE, REG_HL_IY, CC_NONE, CTRL_NONE },	// IY
	{ MODE_REG_C_IND, NULL, CPUF_Z80 | CPUF_GB, REG_D_NONE, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_NONE, CTRL_NONE },	// (C)
	{ MODE_REG_C_IND, NULL, CPUF_GB, REG_D_NONE, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_NONE, CTRL_NONE },	// ($FF00+C)
	{ MODE_REG_SP_IND, NULL, CPUF_Z80 | CPUF_GB, REG_D_NONE, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_NONE, CTRL_NONE },	// (SP)
	{ MODE_REG_BC_IND | MODE_REG_C_IND | MODE_GROUP_RR, NULL, CPUF_Z80 | CPUF_GB, REG_D_NONE, REG_SS_NONE, REG_RR_BC_IND, REG_HL_NONE, CC_NONE, CTRL_NONE },	// (BC)
	{ MODE_REG_DE_IND | MODE_GROUP_RR, NULL, CPUF_Z80 | CPUF_GB, REG_D_NONE, REG_SS_NONE, REG_RR_DE_IND, REG_HL_NONE, CC_NONE, CTRL_NONE },	// (DE)
	{ MODE_REG_HL_IND_INC | MODE_GROUP_RR, NULL, CPUF_GB, REG_D_NONE, REG_SS_NONE, REG_RR_HL_IND_INC, REG_HL_NONE, CC_NONE, CTRL_NONE },	// (HL+)
	{ MODE_REG_HL_IND_DEC | MODE_GROUP_RR, NULL, CPUF_GB, REG_D_NONE, REG_SS_NONE, REG_RR_HL_IND_DEC, REG_HL_NONE, CC_NONE, CTRL_NONE },	// (HL-)
	{ MODE_REG_AF | MODE_GROUP_SS_AF, NULL, CPUF_Z80 | CPUF_GB, REG_D_NONE, REG_SS_AF, REG_RR_NONE, REG_HL_NONE, CC_NONE, CTRL_NONE },	// AF
	{ MODE_REG_AF_SEC, NULL, CPUF_Z80, REG_D_NONE, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_NONE, CTRL_NONE },	// AF'
	{ MODE_REG_CONTROL, NULL, CPUF_Z80, REG_D_NONE, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_NONE, CTRL_I },	// I
	{ MODE_REG_CONTROL, NULL, CPUF_Z80, REG_D_NONE, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_NONE, CTRL_R },	// R
	{ MODE_CC_GB | MODE_CC_Z80, NULL, CPUF_Z80 | CPUF_GB, REG_D_NONE, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_NZ, CTRL_NONE },	// NZ
	{ MODE_CC_GB | MODE_CC_Z80, NULL, CPUF_Z80 | CPUF_GB, REG_D_NONE, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_Z, CTRL_NONE },	// Z
	{ MODE_CC_GB | MODE_CC_Z80, NULL, CPUF_Z80 | CPUF_GB, REG_D_NONE, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_NC, CTRL_NONE },	// NC
	{ MODE_CC_Z80, NULL, CPUF_Z80, REG_D_NONE, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_PO, CTRL_NONE },	// PO
	{ MODE_CC_Z80, NULL, CPUF_Z80, REG_D_NONE, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_PE, CTRL_NONE },	// PO
	{ MODE_CC_Z80, NULL, CPUF_Z80, REG_D_NONE, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_P, CTRL_NONE },	// P
	{ MODE_CC_Z80, NULL, CPUF_Z80, REG_D_NONE, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_M, CTRL_NONE }	// M
};

#define MODE_GROUP_EX (MODE_REG_SP_IND | MODE_REG_AF | MODE_REG_AF_SEC | MODE_REG_DE | MODE_GROUP_HL)
#define MODE_GROUP_IX_IND_DISP (MODE_REG_IX_IND | MODE_REG_IX_IND_DISP)
#define MODE_GROUP_IY_IND_DISP (MODE_REG_IY_IND | MODE_REG_IY_IND_DISP)
#define MODE_GROUP_I_IND_DISP (MODE_GROUP_IX_IND_DISP | MODE_GROUP_IY_IND_DISP)

#define IS_Z80 (opt_Current->machineOptions->cpu & CPUF_Z80)
#define IS_GB  (opt_Current->machineOptions->cpu & CPUF_GB)

static bool
ensureSynthesizedEnabled(void) {
	if (opt_Current->machineOptions->synthesizedInstructions)
		return true;
	
	return err_Error(MERROR_SYNTHESIZED_INSTRUCTIONS);
}

static bool
ensureUndocumentedEnabled(void) {
	if (opt_Current->machineOptions->undocumentedInstructions)
		return true;
	
	return err_Error(MERROR_UNDOCUMENTED_INSTRUCTIONS);
}

static SExpression*
createExpressionNBit(SExpression* expression, int lowLimit, int highLimit, int bits) {
    expression = expr_CheckRange(expression, lowLimit, highLimit);
    if (expression == NULL)
        err_Error(ERROR_EXPRESSION_N_BIT, bits);

    return expression;
}

static SExpression*
createExpression16U(SExpression* expression) {
    return createExpressionNBit(expression, 0, 65535, 16);
}

static SExpression*
createExpression16SU(SExpression* expression) {
    return createExpressionNBit(expression, -32768, 65535, 16);
}

static SExpression*
createExpression8SU(SExpression* expression) {
    return createExpressionNBit(expression, -128, 255, 8);
}

static SExpression*
createExpression8U(SExpression* expression) {
    return createExpressionNBit(expression, 0, 255, 8);
}

static SExpression*
createExpression8S(SExpression* expression) {
    return createExpressionNBit(expression, -128, 127, 8);
}

static SExpression*
createExpression3U(SExpression* expression) {
    return createExpressionNBit(expression, 0, 7, 3);
}

static SExpression*
createExpressionPCRel(SExpression* expression) {
    expression = expr_PcRelative(expression, -1);
    return createExpression8S(expression);
}

static SExpression*
createExpressionImmHi(SExpression* expression) {
    expression = expr_CheckRange(expression, 0xFF00, 0xFFFF);
    if (expression == NULL)
        err_Error(MERROR_EXPRESSION_FF00);

    return expr_And(expression, expr_Const(0xFF));
}

static void
outputIXIY(SAddressingMode* addrMode, uint8_t opcode) {
	if (addrMode->mode & MODE_GROUP_IXYLH) {
	    sect_OutputConst8((uint8_t) (addrMode->registerHL == REG_HL_IX ? 0xDDu : 0xFDu));
	} else {
	    sect_OutputConst8((uint8_t) (addrMode->mode & MODE_GROUP_IX_IND_DISP ? 0xDDu : 0xFDu));
	}

    sect_OutputConst8(opcode);

	if (addrMode->mode & MODE_GROUP_I_IND_DISP) {
		if (addrMode->expression != NULL)
			sect_OutputExpr8(addrMode->expression);
		else
			sect_OutputConst8(0);
	}
}

static void
outputGroupHL(SAddressingMode* addrMode) {
    if ((addrMode->mode & MODE_GROUP_HL) && addrMode->registerHL)
        sect_OutputConst8(addrMode->registerHL);
}

static bool
handleAlu(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
    if (addrMode1->mode & MODE_REG_A) {
        if (IS_Z80 && (addrMode2->mode & MODE_GROUP_I_IND_DISP)) {
            outputIXIY(addrMode2, (uint8_t) (0x86u | instruction->opcode));
            return true;
        }

        if (addrMode2->mode & MODE_GROUP_D) {
            uint8_t regD = (uint8_t) addrMode2->registerD;
            sect_OutputConst8((uint8_t) (0x80u | instruction->opcode | regD));
            return true;
        }

        if (addrMode2->mode & MODE_IMM) {
            sect_OutputConst8((uint8_t) 0xC6u | instruction->opcode);
            sect_OutputExpr8(createExpression8SU(addrMode2->expression));
            return true;
        }
    }

    err_Error(ERROR_OPERAND);
    return true;
}

static bool
handleAlu16bit(SInstruction* instruction, uint8_t prefix, uint8_t opcode, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
    if ((addrMode1->mode & MODE_GROUP_HL) && (addrMode2->mode & (MODE_GROUP_SS | MODE_GROUP_HL))) {
        if ((addrMode2->mode & MODE_GROUP_HL) && (addrMode1->registerHL != addrMode2->registerHL)) {
            err_Error(ERROR_SECOND_OPERAND);
            return true;
        }

        if (prefix != 0) {
			sect_OutputConst8(prefix);
		} else if (addrMode1->registerHL != 0) {
			sect_OutputConst8((uint8_t) addrMode1->registerHL);
		}

        uint8_t regSS = (uint8_t) addrMode2->registerSS << 4u;
        sect_OutputConst8(opcode | regSS);
        return true;
    }

    return false;
}

static bool
handleAdc(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
    if (IS_Z80 && handleAlu16bit(instruction, 0xED, 0x4A, addrMode1, addrMode2))
        return true;

    return handleAlu(instruction, addrMode1, addrMode2);
}

static bool
handleSbc(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
    if (IS_Z80 && handleAlu16bit(instruction, 0xED, 0x42, addrMode1, addrMode2))
        return true;

    return handleAlu(instruction, addrMode1, addrMode2);
}

static bool
handleAdd(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
    if (handleAlu16bit(instruction, 0, 0x09, addrMode1, addrMode2))
        return true;

    if (IS_GB && (addrMode1->mode & MODE_REG_SP) && (addrMode2->mode & MODE_IMM)) {
        sect_OutputConst8(0xE8);
        sect_OutputExpr8(createExpression8SU(addrMode2->expression));
        return true;
    }

    return handleAlu(instruction, addrMode1, addrMode2);
}

static bool
handleBit(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
    uint8_t opcode = (uint8_t) addrMode2->registerD | instruction->opcode;

    if (addrMode2->mode & MODE_GROUP_I_IND_DISP) {
        if (IS_Z80) {
            outputIXIY(addrMode2, 0xCB);
            opcode = instruction->opcode | (uint8_t) 6u;
        } else {
            err_Error(MERROR_INSTRUCTION_NOT_SUPPORTED_BY_CPU);
            return true;
        }
    } else {
        sect_OutputConst8(0xCB);
    }

    sect_OutputExpr8(
        expr_Or(
            expr_Const(opcode),
            expr_Asl(
                createExpression3U(addrMode1->expression),
                expr_Const(3))));

    return true;
}

static bool
handleCall(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
    if ((addrMode1->mode & MODE_IMM) && addrMode2->mode == 0) {
        sect_OutputConst8(instruction->opcode);
        sect_OutputExpr16(createExpression16U(addrMode1->expression));
    } else if ((addrMode1->mode & MODE_CC_Z80) && (addrMode2->mode & MODE_IMM)) {
        if (IS_GB && !(addrMode1->mode & MODE_CC_GB)) {
            err_Error(MERROR_INSTRUCTION_NOT_SUPPORTED_BY_CPU);
            return true;
        }
        uint8_t modeF = (uint8_t) addrMode1->modeF << 3u;
        sect_OutputConst8((uint8_t) (instruction->opcode & ~0x19u) | modeF);
        sect_OutputExpr16(createExpression16U(addrMode2->expression));
    } else {
        err_Error(ERROR_OPERAND);
    }

    return true;
}

static bool
handleJp(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	if ((addrMode1->mode & MODE_REG_HL_IND) && addrMode2->mode == 0) {
		sect_OutputConst8(0xE9);
		return true;
	} else if (IS_Z80 && (addrMode1->mode & MODE_REG_IX_IND) && addrMode2->mode == 0) {
		sect_OutputConst8(0xDD);
		sect_OutputConst8(0xE9);
		return true;
	} else if (IS_Z80 && (addrMode1->mode & MODE_REG_IY_IND) && addrMode2->mode == 0) {
		sect_OutputConst8(0xFD);
		sect_OutputConst8(0xE9);
		return true;
	}

	return handleCall(instruction, addrMode1, addrMode2);
}

static bool
handleImplied(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	if (instruction->prefix != 0) {
		sect_OutputConst8(instruction->prefix);
	}
	sect_OutputConst8(instruction->opcode);
	return true;
}

static bool
handleDec(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	if (addrMode1->mode & (MODE_GROUP_SS | MODE_GROUP_HL)) {
		uint8_t opcode = (uint8_t) instruction->opcode << 3u;
		uint8_t regSS = (uint8_t) addrMode1->registerSS << 4u;

		outputGroupHL(addrMode1);
		sect_OutputConst8((uint8_t) (0x03u | opcode | regSS));
	} else if (addrMode1->mode & MODE_GROUP_I_IND_DISP) {
		outputIXIY(addrMode1, (uint8_t) (0x04u | instruction->opcode | (6u << 3u)));
	} else if (addrMode1->mode & MODE_GROUP_IXYLH) {
		outputIXIY(addrMode1, (uint8_t) (0x04u | instruction->opcode | (addrMode1->registerD << 3u)));
	} else {
		uint8_t regD = (uint8_t) addrMode1->registerD << 3u;
		sect_OutputConst8((uint8_t) (0x04u | instruction->opcode | regD));
	}
	return true;
}

static bool
handleJr(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	if ((addrMode1->mode & MODE_IMM) && addrMode2->mode == 0) {
		sect_OutputConst8(0x18);
		sect_OutputExpr8(createExpressionPCRel(addrMode1->expression));
	} else if ((addrMode1->mode & MODE_CC_GB) && (addrMode2->mode & MODE_IMM)) {
		uint8_t modeF = (uint8_t) addrMode1->modeF << 3u;
		sect_OutputConst8((uint8_t) 0x20u | modeF);
		sect_OutputExpr8(createExpressionPCRel(addrMode2->expression));
	} else {
        err_Error(ERROR_OPERAND);
	}

	return true;
}

static bool
handleLd(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	if ((addrMode1->mode & MODE_GROUP_D) && (addrMode2->mode & MODE_GROUP_D)
		&& (addrMode1->registerD != REG_D_HL_IND || addrMode2->registerD != REG_D_HL_IND)) {
		uint8_t regD1 = (uint8_t) addrMode1->registerD << 3u;
		uint8_t regD2 = (uint8_t) addrMode2->registerD;
		sect_OutputConst8((uint8_t) (0x40u | regD1 | regD2));
	} else if ((addrMode1->mode & MODE_REG_A) && (addrMode2->mode & MODE_GROUP_RR)) {
		uint8_t regRR = (uint8_t) addrMode2->registerRR << 4u;
		sect_OutputConst8((uint8_t) 0x0Au | regRR);
	} else if ((addrMode1->mode & MODE_REG_A) && (addrMode2->mode & MODE_IMM_IND)) {
		if (IS_GB && expr_IsConstant(addrMode2->expression) && addrMode2->expression->value.integer >= 0xFF00
			&& addrMode2->expression->value.integer <= 0xFFFF) {
			sect_OutputConst8(0xF0);
			sect_OutputExpr8(createExpressionImmHi(addrMode2->expression));
		} else {
			sect_OutputConst8((uint8_t) (IS_GB ? 0xFAu : 0x3Au));
			sect_OutputExpr16(createExpression16U(addrMode2->expression));
		}
	} else if ((addrMode1->mode & (MODE_GROUP_D | MODE_GROUP_IXYLH)) && (addrMode2->mode & MODE_IMM)) {
		uint8_t regD = (uint8_t) addrMode1->registerD << 3u;
		if (addrMode1->mode & MODE_GROUP_IXYLH) {
		    sect_OutputConst8((uint8_t) (addrMode1->registerHL == REG_HL_IX ? 0xDDu : 0xFDu));
		}
		sect_OutputConst8((uint8_t) 0x06u | regD);
		sect_OutputExpr8(createExpression8SU(addrMode2->expression));
	} else if ((addrMode1->mode & MODE_IMM_IND) && (addrMode2->mode & MODE_REG_A)) {
		if (IS_GB && expr_IsConstant(addrMode1->expression) && addrMode1->expression->value.integer >= 0xFF00
			&& addrMode1->expression->value.integer <= 0xFFFF) {
			sect_OutputConst8(0xE0);
			sect_OutputExpr8(createExpressionImmHi(addrMode1->expression));
		} else {
			sect_OutputConst8((uint8_t) (IS_GB ? 0xEAu : 0x32u));
			sect_OutputExpr16(createExpression16U(addrMode1->expression));
		}
	} else if ((addrMode1->mode & MODE_REG_SP) && (addrMode2->mode & MODE_GROUP_HL)) {
		outputGroupHL(addrMode2);
		sect_OutputConst8(0xF9);
	} else if ((addrMode1->mode & MODE_GROUP_RR) && (addrMode2->mode & MODE_REG_A)) {
		uint8_t regRR = (uint8_t) addrMode1->registerRR << 4u;
		sect_OutputConst8((uint8_t) 0x02u | regRR);
	} else if ((addrMode1->mode & (MODE_GROUP_SS | MODE_GROUP_HL)) && (addrMode2->mode & MODE_IMM)) {
		uint8_t regSS = (uint8_t) addrMode1->registerSS << 4u;
		outputGroupHL(addrMode1);
		sect_OutputConst8((uint8_t) 0x01u | regSS);
		sect_OutputExpr16(createExpression16SU(addrMode2->expression));
	} else if (IS_GB && (addrMode1->mode & MODE_REG_A) && (addrMode2->mode & MODE_REG_C_IND)) {
		sect_OutputConst8(0xF2);
	} else if (IS_GB && (addrMode1->mode & MODE_REG_HL) && (addrMode2->mode & MODE_REG_SP_DISP)) {
		sect_OutputConst8(0xF8);
		sect_OutputExpr8(addrMode2->expression);
	} else if (IS_GB && (addrMode1->mode & MODE_REG_C_IND) && (addrMode2->mode & MODE_REG_A)) {
		sect_OutputConst8(0xE2);
	} else if (IS_GB && (addrMode1->mode & MODE_IMM_IND) && (addrMode2->mode & MODE_REG_SP)) {
		sect_OutputConst8(0x08);
		sect_OutputExpr16(createExpression16U(addrMode1->expression));
	} else if (IS_Z80 && (addrMode1->mode & MODE_IMM_IND) && (addrMode2->mode & (MODE_GROUP_SS | MODE_GROUP_HL))) {
		if (addrMode2->registerSS == REG_SS_HL) {
			outputGroupHL(addrMode2);
			sect_OutputConst8(0x22);
		} else {
			uint8_t regSS = (uint8_t) addrMode2->registerSS << 4u;
			sect_OutputConst8(0xED);
			sect_OutputConst8((uint8_t) 0x43u | regSS);
		}
		sect_OutputExpr16(createExpression16U(addrMode1->expression));
	} else if (IS_Z80 && (addrMode1->mode & MODE_GROUP_I_IND_DISP) && (addrMode2->mode & MODE_GROUP_D)) {
		outputIXIY(addrMode1, (uint8_t) (0x70u | (uint8_t) addrMode2->registerD));
	} else if (IS_Z80 && (addrMode1->mode & MODE_GROUP_I_IND_DISP) && (addrMode2->mode & MODE_IMM)) {
		outputIXIY(addrMode1, 0x36);
		sect_OutputExpr8(createExpression8SU(addrMode2->expression));
	} else if (IS_Z80 && (addrMode1->mode & MODE_GROUP_D) && (addrMode2->mode & MODE_GROUP_I_IND_DISP)) {
		uint8_t regD = (uint8_t) addrMode1->registerD << 3u;
		outputIXIY(addrMode2, (uint8_t) 0x46u | regD);
	} else if (IS_Z80 && (addrMode1->mode & MODE_REG_A) && (addrMode2->mode & MODE_REG_CONTROL)) {
		uint8_t regCtrl = (uint8_t) addrMode2->registerCtrl << 3u;
		sect_OutputConst8(0xED);
		sect_OutputConst8((uint8_t) 0x57u | regCtrl);
	} else if (IS_Z80 && (addrMode1->mode & MODE_REG_CONTROL) && (addrMode2->mode & MODE_REG_A)) {
		uint8_t regCtrl = (uint8_t) addrMode1->registerCtrl << 3u;
		sect_OutputConst8(0xED);
		sect_OutputConst8((uint8_t) 0x47u | regCtrl);
	} else if (IS_Z80 && (addrMode1->mode & (MODE_GROUP_SS | MODE_GROUP_HL)) && (addrMode2->mode & MODE_IMM_IND)) {
		if (addrMode1->registerSS == REG_SS_HL) {
			outputGroupHL(addrMode1);
			sect_OutputConst8(0x2A);
		} else {
			uint8_t regSS = (uint8_t) addrMode1->registerSS << 4u;
			sect_OutputConst8(0xED);
			sect_OutputConst8((uint8_t) 0x4Bu | regSS);
		}
		sect_OutputExpr16(createExpression16U(addrMode2->expression));
	} else if ((addrMode1->mode & MODE_GROUP_SS) && (addrMode1->registerSS <= REG_SS_HL) && (addrMode2->mode & MODE_GROUP_SS) && (addrMode2->registerSS <= REG_SS_HL)) {
		if (!ensureSynthesizedEnabled())
			return false;

		ETargetToken* destTokens = g_registerPairsSS[addrMode1->registerSS];
		ETargetToken* srcTokens = g_registerPairsSS[addrMode2->registerSS];
		for (int i = 0; i <= 1; ++i) {
			if (!handleLd(instruction, &g_addressModes[destTokens[i] - T_MODE_B], &g_addressModes[srcTokens[i] - T_MODE_B]))
				return false;
		}
		return true;
	} else if ((addrMode1->mode & MODE_GROUP_SS) && (addrMode1->registerSS <= REG_SS_HL) && (addrMode2->mode & MODE_GROUP_I_IND_DISP)) {
		if (!ensureSynthesizedEnabled())
			return false;

		ETargetToken* destTokens = g_registerPairsSS[addrMode1->registerSS];
		SAddressingMode offsetPlusOne = *addrMode2;
		offsetPlusOne.expression =
			addrMode2->expression == NULL ? expr_Const(1) :
			createExpression8S(
				expr_Add(
					expr_Copy(addrMode2->expression),
					expr_Const(1)));
		if (!handleLd(instruction, &g_addressModes[destTokens[1] - T_MODE_B], addrMode2))
			return false;
		if (!handleLd(instruction, &g_addressModes[destTokens[0] - T_MODE_B], &offsetPlusOne))
			return false;
		return true;
	} else if ((addrMode1->mode & MODE_GROUP_I_IND_DISP) && (addrMode2->mode & MODE_GROUP_SS) && (addrMode2->registerSS <= REG_SS_HL)) {
		if (!ensureSynthesizedEnabled())
			return false;

		ETargetToken* destTokens = g_registerPairsSS[addrMode2->registerSS];
		SAddressingMode offsetPlusOne = *addrMode1;
		offsetPlusOne.expression =
			addrMode1->expression == NULL ? expr_Const(1) :
			createExpression8S(
				expr_Add(
					expr_Copy(addrMode1->expression),
					expr_Const(1)));
		if (!handleLd(instruction, addrMode1, &g_addressModes[destTokens[1] - T_MODE_B]))
			return false;
		if (!handleLd(instruction, &offsetPlusOne, &g_addressModes[destTokens[0] - T_MODE_B]))
			return false;
		return true;
	} else {
        err_Error(ERROR_OPERAND);
	}

	return true;
}

static bool
handleLdd(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	if (IS_GB && (addrMode1->mode & MODE_REG_A) && (addrMode2->mode & MODE_REG_HL_IND)) {
		sect_OutputConst8((uint8_t) (instruction->opcode | 0x08u));
	} else if (IS_GB && (addrMode1->mode & MODE_REG_HL_IND) && (addrMode2->mode & MODE_REG_A)) {
		sect_OutputConst8((uint8_t) instruction->opcode);
	} else if (IS_Z80 && (addrMode1->mode == 0) && (addrMode2->mode == 0)) {
		sect_OutputConst8(0xED);
		// Translate Gameboy opcode to Z80 equivalent...
		sect_OutputConst8((uint8_t) (0xA0u | ((instruction->opcode & 0x10u) >> 1u)));
	} else {
        err_Error(ERROR_OPERAND);
	}

	return true;
}

static bool
handleLdh(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	if ((addrMode1->mode & MODE_REG_A) && (addrMode2->mode & MODE_IMM_IND)) {
		sect_OutputConst8((uint8_t) (instruction->opcode | 0x10u));
		sect_OutputExpr8(createExpressionImmHi(addrMode2->expression));
	} else if ((addrMode1->mode & MODE_IMM_IND) && (addrMode2->mode & MODE_REG_A)) {
		sect_OutputConst8(instruction->opcode);
		sect_OutputExpr8(createExpressionImmHi(addrMode1->expression));
	} else {
        err_Error(ERROR_OPERAND);
	}

	return true;
}

static bool
handleLdhl(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	sect_OutputConst8(instruction->opcode);
	sect_OutputExpr8(createExpression8S(addrMode2->expression));

	return true;
}

static bool
handlePop(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	uint8_t regSS = (uint8_t) addrMode1->registerSS << 4u;

	outputGroupHL(addrMode1);
	sect_OutputConst8((uint8_t) instruction->opcode | regSS);
	return true;
}

static bool
handleRotate(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	if (addrMode1->mode & MODE_GROUP_I_IND_DISP) {
		outputIXIY(addrMode1, 0xCB);
	} else {
		sect_OutputConst8(0xCB);
	}

	sect_OutputConst8((uint8_t) addrMode1->registerD | instruction->opcode);
	return true;
}

static bool
handle16BitShifts(SInstruction* instruction, ETargetToken synthesizedInstruction1, ETargetToken register1, ETargetToken synthesizedInstruction2, ETargetToken register2) {
	if (!ensureSynthesizedEnabled())
		return false;

	if (!handleRotate(&g_instructions[synthesizedInstruction1 - T_Z80_ADC], &g_addressModes[register1 - T_MODE_B], NULL))
		return false;
	if (!handleRotate(&g_instructions[synthesizedInstruction2 - T_Z80_ADC], &g_addressModes[register2 - T_MODE_B], NULL))
		return false;
	return true;
}

static bool
handleSRx(SInstruction* instruction, ETargetToken upperSynthesizedInstruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	if ((addrMode1->mode & MODE_GROUP_SS) && (addrMode1->registerSS <= REG_SS_HL)) {
		ETargetToken* registers = g_registerPairsSS[addrMode1->registerSS];
		return handle16BitShifts(instruction, upperSynthesizedInstruction, registers[0], T_Z80_RR, registers[1]);
	}

	return handleRotate(instruction, addrMode1, addrMode2);
}

static bool
handleSLx(SInstruction* instruction, ETargetToken lowerSynthesizedInstruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	if ((addrMode1->mode & MODE_GROUP_SS) && (addrMode1->registerSS <= REG_SS_HL)) {
		ETargetToken* registers = g_registerPairsSS[addrMode1->registerSS];
		return handle16BitShifts(instruction, lowerSynthesizedInstruction, registers[1], T_Z80_RL, registers[0]);
	}

	return handleRotate(instruction, addrMode1, addrMode2);
}

static bool
handleSRA(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	return handleSRx(instruction, T_Z80_SRA, addrMode1, addrMode2);
}

static bool
handleSRL(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	return handleSRx(instruction, T_Z80_SRL, addrMode1, addrMode2);
}

static bool
handleSLA(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	return handleSLx(instruction, T_Z80_SLA, addrMode1, addrMode2);
}

static bool
handleSLL(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	if (!ensureUndocumentedEnabled())
		return false;

	return handleSLx(instruction, T_Z80_SLL, addrMode1, addrMode2);
}

static bool
handleRr(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	if (addrMode1->registerD == REG_D_A)
        err_Warn(MERROR_SUGGEST_OPCODE, "RRA");
	return handleRotate(instruction, addrMode1, addrMode2);
}

static bool
handleRl(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	if (addrMode1->registerD == REG_D_A)
        err_Warn(MERROR_SUGGEST_OPCODE, "RLA");
	return handleRotate(instruction, addrMode1, addrMode2);
}

static bool
handleRrc(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	if (addrMode1->registerD == REG_D_A)
        err_Warn(MERROR_SUGGEST_OPCODE, "RRCA");
	return handleRotate(instruction, addrMode1, addrMode2);
}

static bool
handleRlc(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	if (addrMode1->registerD == REG_D_A)
        err_Warn(MERROR_SUGGEST_OPCODE, "RLCA");
	return handleRotate(instruction, addrMode1, addrMode2);
}

static bool
handleRet(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	if (addrMode1->mode & MODE_CC_Z80) {
		uint8_t modeF = (uint8_t) addrMode1->modeF << 3u;
		sect_OutputConst8((uint8_t) 0xC0u | modeF);
	} else {
		sect_OutputConst8(0xC9);
	}

	return true;
}

static bool
handleRst(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	if (expr_IsConstant(addrMode1->expression)) {
		uint32_t val = (uint32_t) addrMode1->expression->value.integer;
		if (val == (val & 0x38u)) {
			sect_OutputConst8((uint8_t) (instruction->opcode | val));
		} else {
            err_Error(ERROR_OPERAND_RANGE);
		}
	} else {
        err_Error(ERROR_EXPR_CONST);
	}

	return true;
}

static bool
handleStop(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	sect_OutputConst8(0x10);
	sect_OutputConst8(0x00);
	return true;
}

static bool
handleDjnz(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	sect_OutputConst8(instruction->opcode);
	sect_OutputExpr8(createExpressionPCRel(addrMode1->expression));
	return true;
}

#define EX_MATCH_MODE1(o1,o2) ((addrMode1->mode & (o1)) && (addrMode2->mode & (o2)))
#define EX_MATCH_MODE(o1,o2) (EX_MATCH_MODE1(o1,o2) || EX_MATCH_MODE1(o2,o1))

static bool
handleEx(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	if (!IS_Z80) {
        err_Error(MERROR_INSTRUCTION_NOT_SUPPORTED_BY_CPU);
		return true;
	}

	if (EX_MATCH_MODE(MODE_REG_SP_IND, MODE_GROUP_HL)) {
		outputGroupHL(addrMode1->mode & MODE_GROUP_HL ? addrMode1 : addrMode2);
		sect_OutputConst8(0xE3);
	} else if (EX_MATCH_MODE(MODE_REG_AF, MODE_REG_AF_SEC)) {
		sect_OutputConst8(0x08);
	} else if (EX_MATCH_MODE(MODE_REG_DE, MODE_REG_HL)) {
		sect_OutputConst8(0xEB);
	} else {
        err_Error(ERROR_SECOND_OPERAND);
	}

	return true;
}

static bool
handleIm(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	if (!expr_IsConstant(addrMode1->expression)) {
        err_Error(ERROR_EXPR_CONST);
		return true;
	}

	sect_OutputConst8(0xED);
	switch (addrMode1->expression->value.integer) {
		case 0:
			sect_OutputConst8(0x46);
			return true;
		case 1:
			sect_OutputConst8(0x56);
			return true;
		case 2:
			sect_OutputConst8(0x5E);
			return true;
		default:
            err_Error(ERROR_OPERAND_RANGE);
			return true;
	}
}

static bool
handleInOut(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	if (addrMode1->mode & MODE_REG_C_IND) {
		sect_OutputConst8(instruction->prefix);
		sect_OutputConst8(instruction->opcode | addrMode2->registerD << 3u);
		return true;
	}

	if (addrMode2->mode & (MODE_REG_A | MODE_NONE)) {
		sect_OutputConst8((uint8_t) (0xDBu ^ ((instruction->opcode & 1u) << 3u)));
		sect_OutputExpr8(createExpression8U(addrMode1->expression));
		return true;
	}

	err_Error(ERROR_OPERAND);
	return true;
}

static bool
handleIn(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	if ((addrMode1->mode & MODE_GROUP_D) && (addrMode1->registerD == REG_D_HL_IND))
		return err_Error(ERROR_DEST_OPERAND);

	if (addrMode1->mode & MODE_REG_C_IND) {
		if (addrMode2 != NULL && addrMode2->mode != 0)
			return err_Error(ERROR_SOURCE_OPERAND);
		if (!ensureUndocumentedEnabled())
			return false;

		sect_OutputConst8(0xED);
		sect_OutputConst8(0x70);
		return false;
	}

	return handleInOut(instruction, addrMode2, addrMode1);
}

static bool
handleOut(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	if ((addrMode2->mode & MODE_GROUP_D) && (addrMode2->registerD == REG_D_HL_IND))
		return err_Error(ERROR_SOURCE_OPERAND);

	if (addrMode2->mode & MODE_IMM) {
		if (!ensureUndocumentedEnabled())
			return false;
		
		if (expr_IsConstant(addrMode2->expression) && (addrMode2->expression->value.integer == 0)) {
			sect_OutputConst8(0xED);
			sect_OutputConst8(0x71);
			return true;
		}

		return err_Error(ERROR_SOURCE_OPERAND);
	}		

	return handleInOut(instruction, addrMode1, addrMode2);
}

static bool
handleReti(SInstruction* instruction, SAddressingMode* addrMode1, SAddressingMode* addrMode2) {
	if (IS_GB) {
		sect_OutputConst8(0xD9);
	} else if (IS_Z80) {
		return handleImplied(instruction, addrMode1, addrMode2);
	}

	return true;
}


static SInstruction g_instructions[T_Z80_XOR - T_Z80_ADC + 1] = {
	{ CPUF_GB | CPUF_Z80, 0xED, 0x08, MODE_REG_A | MODE_REG_HL, MODE_GROUP_D | MODE_IMM | MODE_GROUP_I_IND_DISP | MODE_GROUP_SS, handleAdc },	/* ADC */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x00, MODE_REG_A | MODE_GROUP_HL | MODE_REG_SP, MODE_GROUP_D | MODE_IMM | MODE_GROUP_SS | MODE_GROUP_HL | MODE_GROUP_I_IND_DISP, handleAdd },	/* ADD */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x20, MODE_REG_A, MODE_GROUP_D | MODE_IMM | MODE_GROUP_I_IND_DISP, handleAlu },	/* AND */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x40, MODE_IMM, MODE_GROUP_D | MODE_GROUP_I_IND_DISP, handleBit },				/* BIT */
	{ CPUF_GB | CPUF_Z80, 0x00, 0xCD, MODE_CC_Z80 | MODE_IMM, MODE_IMM | MODE_NONE, handleCall },	/* CALL */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x3F, 0, 0, handleImplied },							/* CCF */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x38, MODE_REG_A, MODE_GROUP_D | MODE_IMM | MODE_GROUP_I_IND_DISP, handleAlu },	/* CP */
	{ CPUF_Z80, 0xED, 0xA9, 0, 0, handleImplied },	/* CPD */
	{ CPUF_Z80, 0xED, 0xB9, 0, 0, handleImplied },	/* CPDR */
	{ CPUF_Z80, 0xED, 0xA1, 0, 0, handleImplied },	/* CPI */
	{ CPUF_Z80, 0xED, 0xB1, 0, 0, handleImplied },	/* CPIR */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x2F, 0, 0, handleImplied },							/* CPL */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x27, 0, 0, handleImplied },							/* DAA */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x01, MODE_GROUP_SS | MODE_GROUP_D | MODE_GROUP_I_IND_DISP | MODE_GROUP_HL, 0, handleDec },			/* DEC */
	{ CPUF_GB | CPUF_Z80, 0x00, 0xF3, 0, 0, handleImplied },							/* DI */
	{ CPUF_Z80, 0x00, 0x10, MODE_IMM, 0, handleDjnz },	/* DJNZ */
	{ CPUF_GB | CPUF_Z80, 0x00, 0xFB, 0, 0, handleImplied },							/* EI */
	{ CPUF_Z80, 0x00, 0x00, MODE_GROUP_EX, MODE_GROUP_EX, handleEx },	/* EX */
	{ CPUF_Z80, 0x00, 0xD9, 0, 0, handleImplied },	/* EXX */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x76, 0, 0, handleImplied },							/* HALT */
	{ CPUF_Z80, 0xED, 0x46, MODE_IMM, 0, handleIm },	/* IM */
	{ CPUF_Z80, 0xED, 0x40, MODE_GROUP_D | MODE_REG_C_IND, MODE_IMM_IND | MODE_REG_C_IND | MODE_NONE, handleIn },	/* IN */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x00, MODE_GROUP_SS | MODE_GROUP_D | MODE_GROUP_I_IND_DISP | MODE_GROUP_HL | MODE_GROUP_IXYLH, 0, handleDec },			/* INC */
	{ CPUF_Z80, 0xED, 0xAA, 0, 0, handleImplied },							/* IND */
	{ CPUF_Z80, 0xED, 0xBA, 0, 0, handleImplied },							/* INDR */
	{ CPUF_Z80, 0xED, 0xA2, 0, 0, handleImplied },							/* INI */
	{ CPUF_Z80, 0xED, 0xB2, 0, 0, handleImplied },							/* INIR */
	{ CPUF_GB | CPUF_Z80, 0x00, 0xC3, MODE_CC_Z80 | MODE_IMM | MODE_REG_HL_IND | MODE_REG_IX_IND | MODE_REG_IY_IND, MODE_IMM | MODE_NONE, handleJp },	/* JP */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x00, MODE_CC_GB | MODE_IMM, MODE_IMM | MODE_NONE, handleJr },	/* JR */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x00,
		MODE_REG_A | MODE_REG_C_IND | MODE_REG_HL | MODE_REG_SP | MODE_GROUP_HL | MODE_REG_CONTROL | MODE_GROUP_D | MODE_GROUP_RR | MODE_GROUP_SS | MODE_IMM_IND | MODE_GROUP_I_IND_DISP | MODE_GROUP_IXYLH,
		MODE_REG_A | MODE_REG_C_IND | MODE_REG_HL | MODE_REG_SP | MODE_GROUP_HL | MODE_REG_CONTROL | MODE_REG_SP_DISP | MODE_GROUP_D | MODE_GROUP_RR | MODE_IMM_IND | MODE_IMM | MODE_GROUP_SS | MODE_GROUP_I_IND_DISP, handleLd },	/* LD */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x32, MODE_NONE | MODE_REG_A | MODE_REG_HL_IND, MODE_NONE | MODE_REG_A | MODE_REG_HL_IND, handleLdd },	/* LDD */
	{ CPUF_Z80, 0xED, 0xB8, 0, 0, handleImplied },	/* LDDR */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x22, MODE_NONE | MODE_REG_A | MODE_REG_HL_IND, MODE_NONE | MODE_REG_A | MODE_REG_HL_IND, handleLdd },	/* LDI */
	{ CPUF_Z80, 0xED, 0xB0, 0, 0, handleImplied },	/* LDIR */
	{ CPUF_GB, 0x00, 0xE0, MODE_IMM_IND | MODE_REG_A, MODE_IMM_IND | MODE_REG_A, handleLdh },	/* LDH */
	{ CPUF_GB, 0x00, 0xF8, MODE_REG_SP, MODE_IMM, handleLdhl },	/* LDHL */
	{ CPUF_Z80, 0xED, 0x44, 0, 0, handleImplied },	/* NEG */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x00, 0, 0, handleImplied },	/* NOP */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x30, MODE_REG_A, MODE_GROUP_D | MODE_IMM | MODE_GROUP_I_IND_DISP, handleAlu },	/* OR */
	{ CPUF_Z80, 0xED, 0xBB, 0, 0, handleImplied },	/* OTDR */
	{ CPUF_Z80, 0xED, 0xB3, 0, 0, handleImplied },	/* OTIR */
	{ CPUF_Z80, 0xED, 0x41, MODE_IMM_IND | MODE_REG_C_IND, MODE_GROUP_D | MODE_IMM, handleOut },	/* OUT */
	{ CPUF_Z80, 0xED, 0xAB, 0, 0, handleImplied },	/* OUTD */
	{ CPUF_Z80, 0xED, 0xA3, 0, 0, handleImplied },	/* OUTI */
	{ CPUF_GB | CPUF_Z80, 0x00, 0xC1, MODE_GROUP_SS_AF | MODE_GROUP_HL, 0, handlePop },	/* POP */
	{ CPUF_GB | CPUF_Z80, 0x00, 0xC5, MODE_GROUP_SS_AF | MODE_GROUP_HL, 0, handlePop },	/* PUSH */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x80, MODE_IMM, MODE_GROUP_D | MODE_GROUP_I_IND_DISP, handleBit },				/* RES */
	{ CPUF_GB | CPUF_Z80, 0x00, 0xC0, MODE_NONE | MODE_CC_Z80, 0, handleRet },	/* RET */
	{ CPUF_GB | CPUF_Z80, 0xED, 0x4D, 0, 0, handleReti },	/* RETI */
	{ CPUF_Z80, 0xED, 0x45, 0, 0, handleImplied },	/* RETN */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x10, MODE_GROUP_D | MODE_GROUP_I_IND_DISP, 0, handleRl },	/* RL */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x17, 0, 0, handleImplied },	/* RLA */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x00, MODE_GROUP_D | MODE_GROUP_I_IND_DISP, 0, handleRlc },	/* RLC */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x07, 0, 0, handleImplied },	/* RLCA */
	{ CPUF_Z80, 0xED, 0x6F, 0, 0, handleImplied },	/* RLD */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x18, MODE_GROUP_D | MODE_GROUP_I_IND_DISP, 0, handleRr },	/* RR */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x1F, 0, 0, handleImplied },	/* RRA */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x08, MODE_GROUP_D | MODE_GROUP_I_IND_DISP, 0, handleRrc },	/* RRC */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x0F, 0, 0, handleImplied },	/* RRCA */
	{ CPUF_Z80, 0xED, 0x67, 0, 0, handleImplied },	/* RRD */
	{ CPUF_GB | CPUF_Z80, 0x00, 0xC7, MODE_IMM, 0, handleRst },	/* RST */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x18, MODE_REG_A | MODE_REG_HL, MODE_GROUP_D | MODE_IMM | MODE_GROUP_I_IND_DISP | MODE_GROUP_SS, handleSbc },	/* SBC */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x37, 0, 0, handleImplied },	/* SCF */
	{ CPUF_GB | CPUF_Z80, 0x00, 0xC0, MODE_IMM, MODE_GROUP_D | MODE_GROUP_I_IND_DISP, handleBit },				/* SET */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x20, MODE_GROUP_SS | MODE_GROUP_D | MODE_GROUP_I_IND_DISP, 0, handleSLA },	/* SLA */
	{ CPUF_Z80, 0x00, 0x30, MODE_GROUP_SS | MODE_GROUP_D | MODE_GROUP_I_IND_DISP, 0, handleSLL },	/* SLL */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x28, MODE_GROUP_SS | MODE_GROUP_D | MODE_GROUP_I_IND_DISP, 0, handleSRA },	/* SRA */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x38, MODE_GROUP_SS | MODE_GROUP_D | MODE_GROUP_I_IND_DISP, 0, handleSRL },	/* SRL */
	{ CPUF_GB, 0x00, 0x10, 0, 0, handleStop },	/* STOP */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x10, MODE_REG_A, MODE_GROUP_D | MODE_IMM | MODE_GROUP_I_IND_DISP, handleAlu },	/* SUB */
	{ CPUF_GB, 0x00, 0x30, MODE_GROUP_D, 0, handleRotate },	/* SWAP */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x28, MODE_REG_A, MODE_GROUP_D | MODE_IMM | MODE_GROUP_I_IND_DISP, handleAlu },	/* XOR */
};

static bool
parse_AddrMode(SAddressingMode* addrMode) {
	if (lex_Current.token >= T_MODE_B && lex_Current.token <= T_CC_M) {
		int mode = lex_Current.token;

		parse_GetToken();

		if (mode == T_MODE_SP && (lex_Current.token == T_OP_ADD || lex_Current.token == T_OP_SUBTRACT)) {
			SExpression* expression = createExpression8S(parse_Expression(1));
			if (expression != NULL) {
				addrMode->mode = MODE_REG_SP_DISP;
				addrMode->expression = expression;
				return true;
			}
		}

		*addrMode = g_addressModes[mode - T_MODE_B];

		if (addrMode->mode & MODE_GROUP_IXYLH)
			return ensureUndocumentedEnabled();

		return true;
	}
	
	if (lex_Current.token == '[' || lex_Current.token == '(') {
		char endToken = (char) (lex_Current.token == '[' ? ']' : ')');
		SLexerBookmark bm;

		lex_Bookmark(&bm);
		parse_GetToken();

		if (lex_Current.token == T_MODE_IX || lex_Current.token == T_MODE_IY) {
			int regToken = lex_Current.token;

			parse_GetToken();

			if (lex_Current.token == T_OP_ADD || lex_Current.token == T_OP_SUBTRACT) {
				SExpression* pExpr = createExpression8S(parse_Expression(1));

				if (pExpr != NULL && parse_ExpectChar(endToken)) {
					addrMode->mode = regToken == T_MODE_IX ? MODE_REG_IX_IND_DISP :
									  /*regToken == T_MODE_IY ? */ MODE_REG_IY_IND_DISP;
					addrMode->expression = pExpr;
					return true;
				}
				expr_Free(pExpr);
			} else if (parse_ExpectChar(endToken)) {
				addrMode->mode = regToken == T_MODE_IX ? MODE_REG_IX_IND :
								  /*regToken == T_MODE_IY ? */ MODE_REG_IY_IND;
				addrMode->expression = NULL;
				return true;
			}
		}
		lex_Goto(&bm);
	}
	
	if (lex_Current.token == '[') {
		SLexerBookmark bm;
		lex_Bookmark(&bm);

		parse_GetToken();

		SExpression* expression = parse_Expression(2);

		if (expression != NULL && lex_Current.token == ']') {
			parse_GetToken();
			addrMode->mode = MODE_IMM_IND;
			addrMode->expression = expression;
			return true;
		}
		expr_Free(expression);
		lex_Goto(&bm);
	}

	SLexerBookmark bm;
	lex_Bookmark(&bm);

	SExpression* expression = parse_Expression(2);

	if (expression != NULL) {
		if (expr_Type(expression) == EXPR_PARENS) {
			addrMode->mode = MODE_IMM_IND;
			addrMode->expression = expression;
			return true;
		}

		addrMode->mode = MODE_IMM;
		addrMode->expression = expression;
		return true;
	}

	lex_Goto(&bm);
	return false;
}

static uint32_t
translateToken(uint32_t token) {
	return token == T_SYM_SET ? T_Z80_SET : token;
}

bool
z80_ParseInstruction(void) {
	uint32_t token = translateToken(lex_Current.token);
	if (token >= T_Z80_ADC && token <= T_Z80_XOR) {
		token = token - T_Z80_ADC;

		SInstruction* instruction = &g_instructions[token];
		parse_GetToken();

		SAddressingMode addrMode1;
		SAddressingMode addrMode2;

		addrMode1.mode = 0;
		addrMode1.cpu = CPUF_Z80 | CPUF_GB;
		addrMode2.mode = 0;
		addrMode2.cpu = CPUF_Z80 | CPUF_GB;

		if (instruction->allowedModes1 != 0) {
			if (parse_AddrMode(&addrMode1)) {
				if (lex_Current.token == ',') {
					parse_GetToken();
					if (!parse_AddrMode(&addrMode2)) {
                        err_Error(ERROR_SECOND_OPERAND);
						return true;
					}
				} else if (instruction->allowedModes2 != 0
						&& (instruction->allowedModes1 & MODE_REG_A)
						&& (instruction->allowedModes2 & addrMode1.mode)) {
					addrMode2 = addrMode1;
					addrMode1.expression = NULL;
					addrMode1.mode = MODE_REG_A | MODE_GROUP_D;
					addrMode1.registerD = REG_D_A;
				}
			} else if (addrMode1.mode != 0 && (addrMode1.mode & MODE_NONE) == 0) {
                err_Error(ERROR_FIRST_OPERAND);
				return true;
			}
		}

		if ((addrMode1.mode & instruction->allowedModes1)
			|| (addrMode1.mode == 0 && ((instruction->allowedModes1 == 0) || (instruction->allowedModes1 & MODE_NONE)))) {
			if ((addrMode2.mode & instruction->allowedModes2)
				|| (addrMode2.mode == 0 && ((instruction->allowedModes2 == 0) || (instruction->allowedModes2 & MODE_NONE)))) {
				if ((opt_Current->machineOptions->cpu & instruction->cpu)
					&& (opt_Current->machineOptions->cpu & addrMode1.cpu)
					&& (opt_Current->machineOptions->cpu & addrMode2.cpu)) {
					return instruction->handler(instruction, instruction->allowedModes1 != 0 ? &addrMode1 : NULL, instruction->allowedModes2 != 0 ? &addrMode2 : NULL);
				} else {
                    err_Error(MERROR_INSTRUCTION_NOT_SUPPORTED_BY_CPU);
				}
			} else {
                err_Error(ERROR_SECOND_OPERAND);
			}
		} else {
            err_Error(ERROR_FIRST_OPERAND);
		}
		return true;
	}

	return false;
}

SExpression*
z80_ParseFunction(void) {
	return NULL;
}
