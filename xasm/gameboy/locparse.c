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
#include "parse.h"
#include "section.h"
#include "project.h"
#include "lexer.h"
#include "localasm.h"
#include "locopt.h"
#include "options.h"

typedef enum
{
	REG_D_NONE = -1,
	REG_D_B = 0,
	REG_D_C,
	REG_D_D,
	REG_D_E,
	REG_D_H,
	REG_D_L,
	REG_D_HL_IND,
	REG_D_A
} ERegD;

typedef enum
{
	REG_SS_NONE = -1,
	REG_SS_BC = 0,
	REG_SS_DE = 1,
	REG_SS_HL = 2,
	REG_SS_SP = 3,
	REG_SS_AF = 3
} ERegSS;

typedef enum
{
	REG_RR_NONE = -1,
	REG_RR_BC_IND = 0,
	REG_RR_DE_IND,
	REG_RR_HL_IND_INC,
	REG_RR_HL_IND_DEC
} ERegRR;

typedef enum
{
	REG_HL_NONE = -1,
	REG_HL_HL = 0,
	REG_HL_IX = 0xDD,
	REG_HL_IY = 0xFD
} ERegHL;

typedef enum
{
	CC_NONE = -1,
	CC_NZ = 0,
	CC_Z,
	CC_NC,
	CC_C,
	CC_PO,	// Z80 only
	CC_PE,	// Z80 only
	CC_P,	// Z80 only
	CC_M,	// Z80 only
} EModeF;

typedef enum
{
	CTRL_NONE = -1,
	CTRL_I,
	CTRL_R
} EModeCtrl;


typedef struct _AddrMode
{
	uint32_t	nMode;

	SExpression* pExpr;
	uint8_t		nCpu;
	ERegD		eRegD;
	ERegSS		eRegSS;
	ERegRR		eRegRR;
	ERegHL		eRegHL;
	EModeF		eModeF;
	EModeCtrl	eRegCtrl;

} SAddrMode;

typedef struct _Opcode
{
	uint8_t		nCpu;
	uint8_t		nPrefix;
	uint8_t		nOpcode;
	uint32_t	nAddrMode1;
	uint32_t	nAddrMode2;
	bool (*pParser)(struct _Opcode* pCode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2);
} SOpcode;

extern SOpcode g_aOpcodes[T_Z80_XOR - T_Z80_ADC + 1];

#define MODE_NONE				0x00000001u
#define MODE_REG_A				0x00000002u
#define MODE_REG_C_IND			0x00000004u
#define MODE_REG_DE				0x00000008u
#define MODE_REG_HL				0x00000010u
#define MODE_REG_SP				0x00000020u
#define MODE_REG_AF				0x00000040u
#define MODE_REG_AF_SEC			0x00000080u
#define MODE_REG_BC_IND			0x00000100u
#define MODE_REG_DE_IND			0x00000200u
#define MODE_REG_HL_IND			0x00000400u
#define MODE_REG_HL_IND_DEC		0x00000800u
#define MODE_REG_HL_IND_INC		0x00001000u
#define MODE_REG_SP_IND			0x00002000u
#define MODE_REG_IX_IND			0x00004000u
#define MODE_REG_IY_IND			0x00008000u
#define MODE_REG_SP_DISP		0x00010000u
#define MODE_REG_IX_IND_DISP	0x00020000u
#define MODE_REG_IY_IND_DISP	0x00040000u
#define MODE_GROUP_D			0x00080000u
#define MODE_GROUP_SS			0x00100000u
#define MODE_GROUP_SS_AF		0x00200000u
#define MODE_GROUP_RR			0x00400000u
#define MODE_GROUP_HL			0x00800000u
#define MODE_IMM				0x01000000u
#define MODE_IMM_IND			0x02000000u
#define MODE_CC_GB				0x04000000u
#define MODE_CC_Z80				0x08000000u
#define MODE_REG_CONTROL		0x10000000u

static SAddrMode s_AddressModes[T_CC_M - T_MODE_B + 1] =
{
	{ MODE_GROUP_D, NULL, CPUF_Z80 | CPUF_GB, REG_D_B, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_NONE, CTRL_NONE },	// B
	{ MODE_GROUP_D | MODE_CC_GB | MODE_CC_Z80, NULL, CPUF_Z80 | CPUF_GB, REG_D_C, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_C, CTRL_NONE },	// C
	{ MODE_GROUP_D, NULL, CPUF_Z80 | CPUF_GB, REG_D_D, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_NONE, CTRL_NONE },	// D
	{ MODE_GROUP_D, NULL, CPUF_Z80 | CPUF_GB, REG_D_E, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_NONE, CTRL_NONE },	// E
	{ MODE_GROUP_D, NULL, CPUF_Z80 | CPUF_GB, REG_D_H, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_NONE, CTRL_NONE },	// H
	{ MODE_GROUP_D, NULL, CPUF_Z80 | CPUF_GB, REG_D_L, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_NONE, CTRL_NONE },	// L
	{ MODE_REG_HL_IND | MODE_GROUP_D, NULL, CPUF_Z80 | CPUF_GB, REG_D_HL_IND, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_NONE, CTRL_NONE }, // (HL)
	{ MODE_REG_A | MODE_GROUP_D, NULL, CPUF_Z80 | CPUF_GB, REG_D_A, REG_SS_NONE, REG_RR_NONE, REG_HL_NONE, CC_NONE, CTRL_NONE }, // A
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

#define IS_Z80 (opt_Current->machineOptions->nCpu & CPUF_Z80)
#define IS_GB  (opt_Current->machineOptions->nCpu & CPUF_GB)

static SExpression* parse_CreateExpressionNBit(SExpression* pExpr, int nLowLimit, int nHighLimit, int nBits)
{
	pExpr = expr_CheckRange(pExpr, nLowLimit, nHighLimit);
	if(pExpr == NULL)
		prj_Error(ERROR_EXPRESSION_N_BIT, nBits);

	return pExpr;
}

static SExpression* parse_CreateExpression16U(SExpression* pExpr)
{
	return parse_CreateExpressionNBit(pExpr, 0, 65535, 16);
}

static SExpression* parse_CreateExpression8SU(SExpression* pExpr)
{
	return parse_CreateExpressionNBit(pExpr, -128, 255, 8);
}

static SExpression* parse_CreateExpression8U(SExpression* pExpr)
{
	return parse_CreateExpressionNBit(pExpr, 0, 255, 8);
}

static SExpression* parse_CreateExpression8S(SExpression* pExpr)
{
	return parse_CreateExpressionNBit(pExpr, -128, 127, 8);
}

static SExpression* parse_CreateExpression3U(SExpression* pExpr)
{
	return parse_CreateExpressionNBit(pExpr, 0, 7, 3);
}

static SExpression* parse_CreateExpressionPCRel(SExpression* pExpr)
{
	pExpr = expr_PcRelative(pExpr, -1);
	return parse_CreateExpression8S(pExpr);
}

static SExpression* parse_CreateExpressionImmHi(SExpression* pExpr)
{
	pExpr = expr_CheckRange(pExpr, 0xFF00, 0xFFFF);
	if(pExpr == NULL)
		prj_Error(MERROR_EXPRESSION_FF00);

	return expr_And(pExpr, expr_Const(0xFF));
}

static void parse_OutputIXIY(SAddrMode* pAddrMode, uint8_t nOpcode)
{
	sect_OutputConst8((uint8_t)(pAddrMode->nMode & MODE_GROUP_IX_IND_DISP ? 0xDDu : 0xFDu));
	sect_OutputConst8(nOpcode);
	if(pAddrMode->pExpr != NULL)
		sect_OutputExpr8(pAddrMode->pExpr);
	else
		sect_OutputConst8(0);
}

static void parse_OutputGroupHL(SAddrMode* pAddrMode)
{
	if((pAddrMode->nMode & MODE_GROUP_HL) && pAddrMode->eRegHL)
		sect_OutputConst8(pAddrMode->eRegHL);
}

static bool parse_Alu(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->nMode & MODE_REG_A)
	{
		if(IS_Z80 && (pAddrMode2->nMode & MODE_GROUP_I_IND_DISP))
		{
			parse_OutputIXIY(pAddrMode2, (uint8_t)(0x86u | pOpcode->nOpcode));
			return true;
		}

		if(pAddrMode2->nMode & MODE_GROUP_D)
		{
			uint8_t regD = (uint8_t) pAddrMode2->eRegD;
			sect_OutputConst8((uint8_t) (0x80u | pOpcode->nOpcode | regD));
			return true;
		}

		if(pAddrMode2->nMode & MODE_IMM)
		{
			sect_OutputConst8((uint8_t) 0xC6u | pOpcode->nOpcode);
			sect_OutputExpr8(parse_CreateExpression8SU(pAddrMode2->pExpr));
			return true;
		}
	}

	prj_Error(ERROR_OPERAND);
	return true;
}

static bool parse_Alu_16bit(SOpcode* pOpcode, uint8_t nPrefix, uint8_t nOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	assert(pOpcode != NULL);

	if((pAddrMode1->nMode & MODE_GROUP_HL)
	&& (pAddrMode2->nMode & (MODE_GROUP_SS | MODE_GROUP_HL)))
	{
		if((pAddrMode2->nMode & MODE_GROUP_HL)
		&& (pAddrMode1->eRegHL != pAddrMode2->eRegHL))
		{
			prj_Error(ERROR_SECOND_OPERAND);
			return true;
		}

		if(nPrefix != 0)
			sect_OutputConst8(nPrefix);
		else if(pAddrMode1->eRegHL != 0)
			sect_OutputConst8((uint8_t)pAddrMode1->eRegHL);

		uint8_t regSS = (uint8_t) pAddrMode2->eRegSS << 4u;
		sect_OutputConst8(nOpcode | regSS);
		return true;
	}

	return false;
}

static bool parse_Adc(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(IS_Z80 && parse_Alu_16bit(pOpcode, 0xED, 0x4A, pAddrMode1, pAddrMode2))
		return true;

	return parse_Alu(pOpcode, pAddrMode1, pAddrMode2);
}

static bool parse_Sbc(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(IS_Z80 && parse_Alu_16bit(pOpcode, 0xED, 0x42, pAddrMode1, pAddrMode2))
		return true;

	return parse_Alu(pOpcode, pAddrMode1, pAddrMode2);
}


static bool parse_Add(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(parse_Alu_16bit(pOpcode, 0, 0x09, pAddrMode1, pAddrMode2))
		return true;

	if(IS_GB && (pAddrMode1->nMode & MODE_REG_SP) && (pAddrMode2->nMode & MODE_IMM))
	{
		sect_OutputConst8(0xE8);
		sect_OutputExpr8(parse_CreateExpression8SU(pAddrMode2->pExpr));
		return true;
	}

	return parse_Alu(pOpcode, pAddrMode1, pAddrMode2);
}

static bool parse_Bit(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	uint8_t nOpcode = (uint8_t) pAddrMode2->eRegD | pOpcode->nOpcode;

	if(pAddrMode2->nMode & MODE_GROUP_I_IND_DISP)
	{
		if(IS_Z80)
		{
			parse_OutputIXIY(pAddrMode2, 0xCB);
			nOpcode = pOpcode->nOpcode | (uint8_t) 6u;
		}
		else
		{
			prj_Error(MERROR_INSTRUCTION_NOT_SUPPORTED_BY_CPU);
			return true;
		}
	}
	else
	{
		sect_OutputConst8(0xCB);
	}	

	sect_OutputExpr8(
		expr_Or(
			expr_Const(nOpcode), expr_Asl(parse_CreateExpression3U(pAddrMode1->pExpr), expr_Const(3))
			)
		);

	return true;
}

static bool parse_Call(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if((pAddrMode1->nMode & MODE_IMM) && pAddrMode2->nMode == 0)
	{
		sect_OutputConst8(pOpcode->nOpcode);
		sect_OutputExpr16(parse_CreateExpression16U(pAddrMode1->pExpr));
	}
	else if((pAddrMode1->nMode & MODE_CC_Z80) && (pAddrMode2->nMode & MODE_IMM))
	{
		if(IS_GB && !(pAddrMode1->nMode & MODE_CC_GB))
		{
			prj_Error(MERROR_INSTRUCTION_NOT_SUPPORTED_BY_CPU);
			return true;
		}
		uint8_t modeF = (uint8_t) pAddrMode1->eModeF << 3u;
		sect_OutputConst8((uint8_t) (pOpcode->nOpcode & ~0x19u) | modeF);
		sect_OutputExpr16(parse_CreateExpression16U(pAddrMode2->pExpr));
	}
	else
	{
		prj_Error(ERROR_OPERAND);
	}

	return true;
}

static bool parse_Jp(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if((pAddrMode1->nMode & MODE_REG_HL_IND) && pAddrMode2->nMode == 0)
	{
		sect_OutputConst8(0xE9);
		return true;
	}
	else if(IS_Z80 && (pAddrMode1->nMode & MODE_REG_IX_IND) && pAddrMode2->nMode == 0)
	{
		sect_OutputConst8(0xDD);
		sect_OutputConst8(0xE9);
		return true;
	}
	else if(IS_Z80 && (pAddrMode1->nMode & MODE_REG_IY_IND) && pAddrMode2->nMode == 0)
	{
		sect_OutputConst8(0xFD);
		sect_OutputConst8(0xE9);
		return true;
	}

	return parse_Call(pOpcode, pAddrMode1, pAddrMode2);
}

static bool parse_Implied(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	assert(pAddrMode1 == NULL);
	assert(pAddrMode2 == NULL);

	if(pOpcode->nPrefix != 0)
		sect_OutputConst8(pOpcode->nPrefix);
	sect_OutputConst8(pOpcode->nOpcode);
	return true;
}

static bool parse_Dec(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	assert(pAddrMode2 == NULL);

	if(pAddrMode1->nMode & (MODE_GROUP_SS | MODE_GROUP_HL))
	{
		uint8_t opcode = (uint8_t) pOpcode->nOpcode << 3u;
		uint8_t regSS = (uint8_t) pAddrMode1->eRegSS << 4u;

		parse_OutputGroupHL(pAddrMode1);
		sect_OutputConst8((uint8_t)(0x03u | opcode | regSS));
	}
	else if(pAddrMode1->nMode & MODE_GROUP_I_IND_DISP)
	{
		parse_OutputIXIY(pAddrMode1, (uint8_t)(0x04u | pOpcode->nOpcode | (6u << 3u)));
	}
	else
	{
		uint8_t regD = (uint8_t) pAddrMode1->eRegD << 3u;
		sect_OutputConst8((uint8_t) (0x04u | pOpcode->nOpcode | regD));
	}
	return true;
}

static bool parse_Jr(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	assert(pOpcode != NULL);

	if((pAddrMode1->nMode & MODE_IMM) && pAddrMode2->nMode == 0)
	{
		sect_OutputConst8(0x18);
		sect_OutputExpr8(parse_CreateExpressionPCRel(pAddrMode1->pExpr));
	}
	else if((pAddrMode1->nMode & MODE_CC_GB) && (pAddrMode2->nMode & MODE_IMM))
	{
		uint8_t modeF = (uint8_t) pAddrMode1->eModeF << 3u;
		sect_OutputConst8((uint8_t) 0x20u | modeF);
		sect_OutputExpr8(parse_CreateExpressionPCRel(pAddrMode2->pExpr));
	}
	else
		prj_Error(ERROR_OPERAND);

	return true;
}

static bool parse_Ld(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	assert(pOpcode != NULL);

	if((pAddrMode1->nMode & MODE_GROUP_D) && (pAddrMode2->nMode & MODE_GROUP_D)
	&& (pAddrMode1->eRegD != REG_D_HL_IND || pAddrMode2->eRegD != REG_D_HL_IND))
	{
		uint8_t regD1 = (uint8_t) pAddrMode1->eRegD << 3u;
		uint8_t regD2 = (uint8_t) pAddrMode2->eRegD;
		sect_OutputConst8((uint8_t)(0x40u | regD1 | regD2));
	}
	else if((pAddrMode1->nMode & MODE_REG_A) && (pAddrMode2->nMode & MODE_GROUP_RR))
	{
		uint8_t regRR = (uint8_t) pAddrMode2->eRegRR << 4u;
		sect_OutputConst8((uint8_t) 0x0Au | regRR);
	}
	else if((pAddrMode1->nMode & MODE_REG_A) && (pAddrMode2->nMode & MODE_IMM_IND))
	{
		if(IS_GB && expr_IsConstant(pAddrMode2->pExpr)
		&& pAddrMode2->pExpr->value.integer >= 0xFF00
		&& pAddrMode2->pExpr->value.integer <= 0xFFFF)
		{
			sect_OutputConst8(0xF0);
			sect_OutputExpr8(parse_CreateExpressionImmHi(pAddrMode2->pExpr));
		}
		else
		{
			sect_OutputConst8((uint8_t)(IS_GB ? 0xFAu : 0x3Au));
			sect_OutputExpr16(parse_CreateExpression16U(pAddrMode2->pExpr));
		}
	}
	else if((pAddrMode1->nMode & MODE_GROUP_D) && (pAddrMode2->nMode & MODE_IMM))
	{
		uint8_t regD = (uint8_t) pAddrMode1->eRegD << 3u;
		sect_OutputConst8((uint8_t) 0x06u | regD);
		sect_OutputExpr8(parse_CreateExpression8SU(pAddrMode2->pExpr));
	}
	else if((pAddrMode1->nMode & MODE_IMM_IND) && (pAddrMode2->nMode & MODE_REG_A))
	{
		if(IS_GB && expr_IsConstant(pAddrMode1->pExpr)
		&& pAddrMode1->pExpr->value.integer >= 0xFF00
		&& pAddrMode1->pExpr->value.integer <= 0xFFFF)
		{
			sect_OutputConst8(0xE0);
			sect_OutputExpr8(parse_CreateExpressionImmHi(pAddrMode1->pExpr));
		}
		else
		{
			sect_OutputConst8((uint8_t) (IS_GB ? 0xEAu : 0x32u));
			sect_OutputExpr16(parse_CreateExpression16U(pAddrMode1->pExpr));
		}
	}
	else if((pAddrMode1->nMode & MODE_REG_SP) && (pAddrMode2->nMode & MODE_GROUP_HL))
	{
		parse_OutputGroupHL(pAddrMode2);
		sect_OutputConst8(0xF9);
	}
	else if((pAddrMode1->nMode & MODE_GROUP_RR) && (pAddrMode2->nMode & MODE_REG_A))
	{
		uint8_t regRR = (uint8_t) pAddrMode1->eRegRR << 4u;
		sect_OutputConst8((uint8_t) 0x02u | regRR);
	}
	else if((pAddrMode1->nMode & (MODE_GROUP_SS | MODE_GROUP_HL)) && (pAddrMode2->nMode & MODE_IMM))
	{
		uint8_t regSS = (uint8_t) pAddrMode1->eRegSS << 4u;
		parse_OutputGroupHL(pAddrMode1);
		sect_OutputConst8((uint8_t) 0x01u | regSS);
		sect_OutputExpr16(parse_CreateExpression16U(pAddrMode2->pExpr));
	}
	else if(IS_GB && (pAddrMode1->nMode & MODE_REG_A) && (pAddrMode2->nMode & MODE_REG_C_IND))
	{
		sect_OutputConst8(0xF2);
	}
	else if(IS_GB && (pAddrMode1->nMode & MODE_REG_HL) && (pAddrMode2->nMode & MODE_REG_SP_DISP))
	{
		sect_OutputConst8(0xF8);
		sect_OutputExpr8(pAddrMode2->pExpr);
	}
	else if(IS_GB && (pAddrMode1->nMode & MODE_REG_C_IND) && (pAddrMode2->nMode & MODE_REG_A))
	{
		sect_OutputConst8(0xE2);
	}
	else if(IS_GB && (pAddrMode1->nMode & MODE_IMM_IND) && (pAddrMode2->nMode & MODE_REG_SP))
	{
		sect_OutputConst8(0x08);
		sect_OutputExpr16(parse_CreateExpression16U(pAddrMode1->pExpr));
	}
	else if(IS_Z80 && (pAddrMode1->nMode & MODE_IMM_IND) && (pAddrMode2->nMode & (MODE_GROUP_SS | MODE_GROUP_HL)))
	{
		if(pAddrMode2->eRegSS == REG_SS_HL)
		{
			parse_OutputGroupHL(pAddrMode2);
			sect_OutputConst8(0x22);
		}
		else
		{
			uint8_t regSS = (uint8_t) pAddrMode2->eRegSS << 4u;
			sect_OutputConst8(0xED);
			sect_OutputConst8((uint8_t) 0x43u | regSS);
		}
		sect_OutputExpr16(parse_CreateExpression16U(pAddrMode1->pExpr));
	}
	else if(IS_Z80 && (pAddrMode1->nMode & MODE_GROUP_I_IND_DISP) && (pAddrMode2->nMode & MODE_GROUP_D))
	{
		parse_OutputIXIY(pAddrMode1, (uint8_t)(0x70u | (uint8_t) pAddrMode2->eRegD));
	}
	else if(IS_Z80 && (pAddrMode1->nMode & MODE_GROUP_I_IND_DISP) && (pAddrMode2->nMode & MODE_IMM))
	{
		parse_OutputIXIY(pAddrMode1, 0x36);
		sect_OutputExpr8(parse_CreateExpression8SU(pAddrMode2->pExpr));
	}
	else if(IS_Z80 && (pAddrMode1->nMode & MODE_GROUP_D) && (pAddrMode2->nMode & MODE_GROUP_I_IND_DISP))
	{
		uint8_t regD = (uint8_t) pAddrMode1->eRegD << 3u;
		parse_OutputIXIY(pAddrMode2, (uint8_t) 0x46u | regD);
	}
	else if(IS_Z80 && (pAddrMode1->nMode & MODE_REG_A) && (pAddrMode2->nMode & MODE_REG_CONTROL))
	{
		uint8_t regCtrl = (uint8_t) pAddrMode2->eRegCtrl << 3u;
		sect_OutputConst8(0xED);
		sect_OutputConst8((uint8_t)  0x57u | regCtrl);
	}
	else if(IS_Z80 && (pAddrMode1->nMode & MODE_REG_CONTROL) && (pAddrMode2->nMode & MODE_REG_A))
	{
		uint8_t regCtrl = (uint8_t) pAddrMode1->eRegCtrl << 3u;
		sect_OutputConst8(0xED);
		sect_OutputConst8((uint8_t) 0x47u | regCtrl);
	}
	else if(IS_Z80 && (pAddrMode1->nMode & (MODE_GROUP_SS | MODE_GROUP_HL)) && (pAddrMode2->nMode & MODE_IMM_IND))
	{
		if(pAddrMode1->eRegSS == REG_SS_HL)
		{
			parse_OutputGroupHL(pAddrMode1);
			sect_OutputConst8(0x2A);
		}
		else
		{
			uint8_t regSS = (uint8_t) pAddrMode1->eRegSS << 4u;
			sect_OutputConst8(0xED);
			sect_OutputConst8((uint8_t) 0x4Bu | regSS);
		}
		sect_OutputExpr16(parse_CreateExpression16U(pAddrMode2->pExpr));
	}
	else
		prj_Error(ERROR_OPERAND);

	return true;
}

static bool parse_Ldd(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(IS_GB && (pAddrMode1->nMode & MODE_REG_A) && (pAddrMode2->nMode & MODE_REG_HL_IND))
		sect_OutputConst8((uint8_t) (pOpcode->nOpcode | 0x08u));
	else if(IS_GB && (pAddrMode1->nMode & MODE_REG_HL_IND) && (pAddrMode2->nMode & MODE_REG_A))
		sect_OutputConst8((uint8_t) pOpcode->nOpcode);
	else if(IS_Z80 && (pAddrMode1->nMode == 0) && (pAddrMode2->nMode == 0))
	{
		sect_OutputConst8(0xED);
		// Translate Gameboy opcode to Z80 equivalent...
		sect_OutputConst8((uint8_t) (0xA0u | ((pOpcode->nOpcode & 0x10u) >> 1u)));
	}
	else
		prj_Error(ERROR_OPERAND);

	return true;
}

static bool parse_Ldh(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if((pAddrMode1->nMode & MODE_REG_A) && (pAddrMode2->nMode & MODE_IMM_IND))
	{
		sect_OutputConst8((uint8_t)(pOpcode->nOpcode | 0x10u));
		sect_OutputExpr8(parse_CreateExpressionImmHi(pAddrMode2->pExpr));
	}
	else if((pAddrMode1->nMode & MODE_IMM_IND) && (pAddrMode2->nMode & MODE_REG_A))
	{
		sect_OutputConst8(pOpcode->nOpcode);
		sect_OutputExpr8(parse_CreateExpressionImmHi(pAddrMode1->pExpr));
	}
	else
		prj_Error(ERROR_OPERAND);

	return true;
}

static bool parse_Ldhl(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	assert(pAddrMode1 != NULL);

	sect_OutputConst8(pOpcode->nOpcode);
	sect_OutputExpr8(parse_CreateExpression8S(pAddrMode2->pExpr));

	return true;
}

static bool parse_Pop(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	assert(pAddrMode2 == NULL);

	uint8_t regSS = (uint8_t) pAddrMode1->eRegSS << 4u;

	parse_OutputGroupHL(pAddrMode1);
	sect_OutputConst8((uint8_t) pOpcode->nOpcode | regSS);
	return true;
}

static bool parse_Rotate(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	assert(pAddrMode2 == NULL);

	if(pAddrMode1->nMode & MODE_GROUP_I_IND_DISP)
		parse_OutputIXIY(pAddrMode1, 0xCB);
	else
		sect_OutputConst8(0xCB);

	sect_OutputConst8((uint8_t) pAddrMode1->eRegD | pOpcode->nOpcode);
	return true;
}

static bool parse_Rr(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->eRegD == REG_D_A)
		prj_Warn(MERROR_SUGGEST_OPCODE, "RRA");
	return parse_Rotate(pOpcode, pAddrMode1, pAddrMode2);
}

static bool parse_Rl(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->eRegD == REG_D_A)
		prj_Warn(MERROR_SUGGEST_OPCODE, "RLA");
	return parse_Rotate(pOpcode, pAddrMode1, pAddrMode2);
}

static bool parse_Rrc(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->eRegD == REG_D_A)
		prj_Warn(MERROR_SUGGEST_OPCODE, "RRCA");
	return parse_Rotate(pOpcode, pAddrMode1, pAddrMode2);
}

static bool parse_Rlc(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->eRegD == REG_D_A)
		prj_Warn(MERROR_SUGGEST_OPCODE, "RLCA");
	return parse_Rotate(pOpcode, pAddrMode1, pAddrMode2);
}

static bool parse_Ret(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2) {
	assert(pOpcode != NULL);
	assert(pAddrMode2 == NULL);

	if (pAddrMode1->nMode & MODE_CC_Z80) {
		uint8_t modeF = (uint8_t) pAddrMode1->eModeF << 3u;
		sect_OutputConst8((uint8_t) 0xC0u | modeF);
	} else {
		sect_OutputConst8(0xC9);
	}

	return true;
}

static bool parse_Rst(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	assert(pAddrMode2 == NULL);

	if(expr_IsConstant(pAddrMode1->pExpr))
	{
		uint32_t val = (uint32_t) pAddrMode1->pExpr->value.integer;
		if (val == (val & 0x38u))
			sect_OutputConst8((uint8_t) (pOpcode->nOpcode | val));
		else
			prj_Error(ERROR_OPERAND_RANGE);
	}
	else
		prj_Error(ERROR_EXPR_CONST);

	return true;
}

static bool parse_Stop(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	assert(pOpcode != NULL);
	assert(pAddrMode1 == NULL);
	assert(pAddrMode2 == NULL);

	sect_OutputConst8(0x10);
	sect_OutputConst8(0x00);
	return true;
}

static bool parse_Djnz(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	assert(pAddrMode2 == NULL);

	sect_OutputConst8(pOpcode->nOpcode);
	sect_OutputExpr8(parse_CreateExpressionPCRel(pAddrMode1->pExpr));
	return true;
}

#define EX_MATCH_MODE1(o1,o2) ((pAddrMode1->nMode & (o1)) && (pAddrMode2->nMode & (o2)))
#define EX_MATCH_MODE(o1,o2) (EX_MATCH_MODE1(o1,o2) || EX_MATCH_MODE1(o2,o1))

static bool parse_Ex(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	assert(pOpcode != NULL);

	if(!IS_Z80)
	{
		prj_Error(MERROR_INSTRUCTION_NOT_SUPPORTED_BY_CPU);
		return true;
	}

	if(EX_MATCH_MODE(MODE_REG_SP_IND, MODE_GROUP_HL))
	{
		parse_OutputGroupHL(pAddrMode1->nMode & MODE_GROUP_HL ? pAddrMode1 : pAddrMode2);
		sect_OutputConst8(0xE3);
	}
	else if(EX_MATCH_MODE(MODE_REG_AF, MODE_REG_AF_SEC))
	{
		sect_OutputConst8(0x08);
	}
	else if(EX_MATCH_MODE(MODE_REG_DE, MODE_REG_HL))
	{
		sect_OutputConst8(0xEB);
	}
	else
	{
		prj_Error(ERROR_SECOND_OPERAND);
	}

	return true;
}

static bool parse_Im(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	assert(pOpcode != NULL);
	assert(pAddrMode2 == NULL);

	if(!expr_IsConstant(pAddrMode1->pExpr))
	{
		prj_Error(ERROR_EXPR_CONST);
		return true;
	}

	sect_OutputConst8(0xED);
	switch(pAddrMode1->pExpr->value.integer)
	{
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
			prj_Error(ERROR_OPERAND_RANGE);
			return true;
	}
}

static bool parse_InOut(SOpcode* pOpcode, SAddrMode* pAddrMode, ERegD eReg)
{
	if(pAddrMode->nMode & MODE_REG_C_IND)
	{
		uint8_t regD = (uint8_t) eReg << 3u;
		sect_OutputConst8(pOpcode->nPrefix);
		sect_OutputConst8(pOpcode->nOpcode | regD);
		return true;
	}

	sect_OutputConst8((uint8_t) (0xDBu ^ ((pOpcode->nOpcode & 1u) << 3u)));
	sect_OutputExpr8(parse_CreateExpression8U(pAddrMode->pExpr));

	return true;
}

static bool parse_In(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	return parse_InOut(pOpcode, pAddrMode2, pAddrMode1->eRegD);
}

static bool parse_Out(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	return parse_InOut(pOpcode, pAddrMode1, pAddrMode2->eRegD);
}

static bool parse_Reti(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(IS_GB)
		sect_OutputConst8(0xD9);
	else if(IS_Z80)
		return parse_Implied(pOpcode, pAddrMode1, pAddrMode2);

	return true;
}


SOpcode g_aOpcodes[T_Z80_XOR - T_Z80_ADC + 1] =
{
	{ CPUF_GB | CPUF_Z80, 0xED, 0x08, MODE_REG_A | MODE_REG_HL, MODE_GROUP_D | MODE_IMM | MODE_GROUP_I_IND_DISP | MODE_GROUP_SS, parse_Adc },	/* ADC */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x00, MODE_REG_A | MODE_GROUP_HL | MODE_REG_SP, MODE_GROUP_D | MODE_IMM | MODE_GROUP_SS | MODE_GROUP_HL | MODE_GROUP_I_IND_DISP, parse_Add },	/* ADD */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x20, MODE_REG_A, MODE_GROUP_D | MODE_IMM | MODE_GROUP_I_IND_DISP, parse_Alu },	/* AND */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x40, MODE_IMM, MODE_GROUP_D | MODE_GROUP_I_IND_DISP, parse_Bit },				/* BIT */
	{ CPUF_GB | CPUF_Z80, 0x00, 0xCD, MODE_CC_Z80 | MODE_IMM, MODE_IMM | MODE_NONE, parse_Call },	/* CALL */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x3F, 0, 0, parse_Implied },							/* CCF */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x38, MODE_REG_A, MODE_GROUP_D | MODE_IMM | MODE_GROUP_I_IND_DISP, parse_Alu },	/* CP */
	{ CPUF_Z80, 0xED, 0xA9, 0, 0, parse_Implied },	/* CPD */
	{ CPUF_Z80, 0xED, 0xB9, 0, 0, parse_Implied },	/* CPDR */
	{ CPUF_Z80, 0xED, 0xA1, 0, 0, parse_Implied },	/* CPI */
	{ CPUF_Z80, 0xED, 0xB1, 0, 0, parse_Implied },	/* CPIR */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x2F, 0, 0, parse_Implied },							/* CPL */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x27, 0, 0, parse_Implied },							/* DAA */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x01, MODE_GROUP_SS | MODE_GROUP_D | MODE_GROUP_I_IND_DISP | MODE_GROUP_HL, 0, parse_Dec },			/* DEC */
	{ CPUF_GB | CPUF_Z80, 0x00, 0xF3, 0, 0, parse_Implied },							/* DI */
	{ CPUF_Z80, 0x00, 0x10, MODE_IMM, 0, parse_Djnz },	/* DJNZ */
	{ CPUF_GB | CPUF_Z80, 0x00, 0xFB, 0, 0, parse_Implied },							/* EI */
	{ CPUF_Z80, 0x00, 0x00, MODE_GROUP_EX, MODE_GROUP_EX, parse_Ex },	/* EX */
	{ CPUF_Z80, 0x00, 0xD9, 0, 0, parse_Implied },	/* EXX */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x76, 0, 0, parse_Implied },							/* HALT */
	{ CPUF_Z80, 0xED, 0x46, MODE_IMM, 0, parse_Im },	/* IM */
	{ CPUF_Z80, 0xED, 0x40, MODE_GROUP_D, MODE_IMM_IND | MODE_REG_C_IND, parse_In },	/* IN */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x00, MODE_GROUP_SS | MODE_GROUP_D | MODE_GROUP_I_IND_DISP | MODE_GROUP_HL, 0, parse_Dec },			/* INC */
	{ CPUF_Z80, 0xED, 0xAA, 0, 0, parse_Implied },							/* IND */
	{ CPUF_Z80, 0xED, 0xBA, 0, 0, parse_Implied },							/* INDR */
	{ CPUF_Z80, 0xED, 0xA2, 0, 0, parse_Implied },							/* INI */
	{ CPUF_Z80, 0xED, 0xB2, 0, 0, parse_Implied },							/* INIR */
	{ CPUF_GB | CPUF_Z80, 0x00, 0xC3, MODE_CC_Z80 | MODE_IMM | MODE_REG_HL_IND | MODE_REG_IX_IND | MODE_REG_IY_IND, MODE_IMM | MODE_NONE, parse_Jp },	/* JP */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x00, MODE_CC_GB | MODE_IMM, MODE_IMM | MODE_NONE, parse_Jr },	/* JR */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x00, 
		MODE_REG_A | MODE_REG_C_IND | MODE_REG_HL | MODE_REG_SP | MODE_GROUP_HL | MODE_REG_CONTROL | MODE_GROUP_D | MODE_GROUP_RR | MODE_GROUP_SS | MODE_IMM_IND | MODE_GROUP_I_IND_DISP,
		MODE_REG_A | MODE_REG_C_IND | MODE_REG_HL | MODE_REG_SP | MODE_GROUP_HL | MODE_REG_CONTROL | MODE_REG_SP_DISP | MODE_GROUP_D | MODE_GROUP_RR | MODE_IMM_IND | MODE_IMM | MODE_GROUP_SS | MODE_GROUP_I_IND_DISP, parse_Ld },	/* LD */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x32, MODE_NONE | MODE_REG_A | MODE_REG_HL_IND, MODE_NONE | MODE_REG_A | MODE_REG_HL_IND, parse_Ldd },	/* LDD */
	{ CPUF_Z80, 0xED, 0xB8, 0, 0, parse_Implied },	/* LDDR */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x22, MODE_NONE | MODE_REG_A | MODE_REG_HL_IND, MODE_NONE | MODE_REG_A | MODE_REG_HL_IND, parse_Ldd },	/* LDI */
	{ CPUF_Z80, 0xED, 0xB0, 0, 0, parse_Implied },	/* LDIR */
	{ CPUF_GB, 0x00, 0xE0, MODE_IMM_IND | MODE_REG_A, MODE_IMM_IND | MODE_REG_A, parse_Ldh },	/* LDH */
	{ CPUF_GB, 0x00, 0xF8, MODE_REG_SP, MODE_IMM, parse_Ldhl },	/* LDHL */
	{ CPUF_Z80, 0xED, 0x44, 0, 0, parse_Implied },	/* NEG */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x00, 0, 0, parse_Implied },	/* NOP */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x30, MODE_REG_A, MODE_GROUP_D | MODE_IMM | MODE_GROUP_I_IND_DISP, parse_Alu },	/* OR */
	{ CPUF_Z80, 0xED, 0xBB, 0, 0, parse_Implied },	/* OTDR */
	{ CPUF_Z80, 0xED, 0xB3, 0, 0, parse_Implied },	/* OTIR */
	{ CPUF_Z80, 0xED, 0x41, MODE_IMM_IND | MODE_REG_C_IND, MODE_GROUP_D, parse_Out },	/* OUT */
	{ CPUF_Z80, 0xED, 0xAB, 0, 0, parse_Implied },	/* OUTD */
	{ CPUF_Z80, 0xED, 0xA3, 0, 0, parse_Implied },	/* OUTI */
	{ CPUF_GB | CPUF_Z80, 0x00, 0xC1, MODE_GROUP_SS_AF | MODE_GROUP_HL, 0, parse_Pop },	/* POP */
	{ CPUF_GB | CPUF_Z80, 0x00, 0xC5, MODE_GROUP_SS_AF | MODE_GROUP_HL, 0, parse_Pop },	/* PUSH */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x80, MODE_IMM, MODE_GROUP_D | MODE_GROUP_I_IND_DISP, parse_Bit },				/* RES */
	{ CPUF_GB | CPUF_Z80, 0x00, 0xC0, MODE_NONE | MODE_CC_Z80, 0, parse_Ret },	/* RET */
	{ CPUF_GB | CPUF_Z80, 0xED, 0x4D, 0, 0, parse_Reti },	/* RETI */
	{ CPUF_Z80, 0xED, 0x45, 0, 0, parse_Implied },	/* RETN */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x10, MODE_GROUP_D | MODE_GROUP_I_IND_DISP, 0, parse_Rl },	/* RL */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x17, 0, 0, parse_Implied },	/* RLA */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x00, MODE_GROUP_D | MODE_GROUP_I_IND_DISP, 0, parse_Rlc },	/* RLC */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x07, 0, 0, parse_Implied },	/* RLCA */
	{ CPUF_Z80, 0xED, 0x6F, 0, 0, parse_Implied },	/* RLD */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x18, MODE_GROUP_D | MODE_GROUP_I_IND_DISP, 0, parse_Rr },	/* RR */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x1F, 0, 0, parse_Implied },	/* RRA */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x08, MODE_GROUP_D | MODE_GROUP_I_IND_DISP, 0, parse_Rrc },	/* RRC */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x0F, 0, 0, parse_Implied },	/* RRCA */
	{ CPUF_Z80, 0xED, 0x67, 0, 0, parse_Implied },	/* RRD */
	{ CPUF_GB | CPUF_Z80, 0x00, 0xC7, MODE_IMM, 0, parse_Rst },	/* RST */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x18, MODE_REG_A | MODE_REG_HL, MODE_GROUP_D | MODE_IMM | MODE_GROUP_I_IND_DISP | MODE_GROUP_SS, parse_Sbc },	/* SBC */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x37, 0, 0, parse_Implied },	/* SCF */
	{ CPUF_GB | CPUF_Z80, 0x00, 0xC0, MODE_IMM, MODE_GROUP_D | MODE_GROUP_I_IND_DISP, parse_Bit },				/* SET */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x20, MODE_GROUP_D | MODE_GROUP_I_IND_DISP, 0, parse_Rotate },	/* SLA */
	{ CPUF_Z80, 0x00, 0x30, MODE_GROUP_D | MODE_GROUP_I_IND_DISP, 0, parse_Rotate },	/* SLL */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x28, MODE_GROUP_D | MODE_GROUP_I_IND_DISP, 0, parse_Rotate },	/* SRA */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x38, MODE_GROUP_D | MODE_GROUP_I_IND_DISP, 0, parse_Rotate },	/* SRL */
	{ CPUF_GB, 0x00, 0x10, 0, 0, parse_Stop },	/* STOP */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x10, MODE_REG_A, MODE_GROUP_D | MODE_IMM | MODE_GROUP_I_IND_DISP, parse_Alu },	/* SUB */
	{ CPUF_GB, 0x00, 0x30, MODE_GROUP_D, 0, parse_Rotate },	/* SWAP */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x28, MODE_REG_A, MODE_GROUP_D | MODE_IMM | MODE_GROUP_I_IND_DISP, parse_Alu },	/* XOR */
};


static bool parse_AddrMode(SAddrMode* pAddrMode)
{
	if(lex_Current.token >= T_MODE_B
	&& lex_Current.token <= T_CC_M)
	{
		int mode = lex_Current.token;

		parse_GetToken();

		if(mode == T_MODE_SP && (lex_Current.token == T_OP_ADD || lex_Current.token == T_OP_SUBTRACT))
		{
			SExpression* pExpr = parse_CreateExpression8S(parse_Expression(1));
			if(pExpr != NULL)
			{
				pAddrMode->nMode = MODE_REG_SP_DISP;
				pAddrMode->pExpr = pExpr;
				return true;
			}
		}

		*pAddrMode = s_AddressModes[mode - T_MODE_B];
		return true;
	}

	if(lex_Current.token == '[' || lex_Current.token == '(')
	{
		char endToken = (char) (lex_Current.token == '[' ? ']' : ')');
		SLexerBookmark bm;

		lex_Bookmark(&bm);
		parse_GetToken();

		if(lex_Current.token == T_MODE_IX
		|| lex_Current.token == T_MODE_IY)
		{
			int regToken = lex_Current.token;

			parse_GetToken();

			if(lex_Current.token == T_OP_ADD
			|| lex_Current.token == T_OP_SUBTRACT)
			{
				SExpression* pExpr = parse_CreateExpression8S(parse_Expression(1));

				if(pExpr != NULL && parse_ExpectChar(endToken))
				{
					pAddrMode->nMode =
						regToken == T_MODE_IX ? MODE_REG_IX_IND_DISP :
						/*regToken == T_MODE_IY ? */ MODE_REG_IY_IND_DISP;
					pAddrMode->pExpr = pExpr;
					return true;
				}
				expr_Free(pExpr);
			}
			else if(parse_ExpectChar(endToken))
			{
				pAddrMode->nMode =
					regToken == T_MODE_IX ? MODE_REG_IX_IND :
					/*regToken == T_MODE_IY ? */ MODE_REG_IY_IND;
				pAddrMode->pExpr = NULL;
				return true;
			}
		}
		lex_Goto(&bm);
	}

	if(lex_Current.token == '[')
	{
		SExpression* pExpr;

		parse_GetToken();
		
		pExpr = parse_Expression(2);

		if(pExpr != NULL && parse_ExpectChar(']'))
		{
			pAddrMode->nMode = MODE_IMM_IND;
			pAddrMode->pExpr = pExpr;
			return true;
		}

		return false;
	}

	/* Try expression */
	{
		SLexerBookmark bm;
		SExpression* pExpr;

		lex_Bookmark(&bm);
		pExpr = parse_Expression(2);

		if(pExpr != NULL)
		{
			if(expr_Type(pExpr) == EXPR_PARENS)
			{
				pAddrMode->nMode = MODE_IMM_IND;
				pAddrMode->pExpr = pExpr;
				return true;
			}

			pAddrMode->nMode = MODE_IMM;
			pAddrMode->pExpr = pExpr;
			return true;
		}

		lex_Goto(&bm);
	}

	return false;
}

bool parse_TargetSpecific(void)
{
	if((lex_Current.token >= T_Z80_ADC && lex_Current.token <= T_Z80_XOR)
	|| lex_Current.token == T_SYM_SET)
	{
		int nToken = (lex_Current.token == T_SYM_SET ? T_Z80_SET : lex_Current.token) - T_Z80_ADC;
		SOpcode* pOpcode = &g_aOpcodes[nToken];
		SAddrMode addrMode1;
		SAddrMode addrMode2;

		parse_GetToken();

		addrMode1.nMode = 0;
		addrMode1.nCpu = CPUF_Z80 | CPUF_GB;
		addrMode2.nMode = 0;
		addrMode2.nCpu = CPUF_Z80 | CPUF_GB;

		if(pOpcode->nAddrMode1 != 0)
		{
			if(parse_AddrMode(&addrMode1))
			{
				if(lex_Current.token == ',')
				{
					parse_GetToken();
					if(!parse_AddrMode(&addrMode2))
					{
						prj_Error(ERROR_SECOND_OPERAND);
						return true;
					}
				}
				else if(pOpcode->nAddrMode2 != 0
				&& (pOpcode->nAddrMode1 & MODE_REG_A)
				&& (pOpcode->nAddrMode2 & addrMode1.nMode))
				{
					addrMode2 = addrMode1;
					addrMode1.nMode = MODE_REG_A | MODE_GROUP_D;
					addrMode1.eRegD = REG_D_A;
				}
			}
			else if(addrMode1.nMode != 0 && (addrMode1.nMode & MODE_NONE) == 0)
			{
				prj_Error(ERROR_FIRST_OPERAND);
				return true;
			}
		}

		if((addrMode1.nMode & pOpcode->nAddrMode1)
		|| (addrMode1.nMode == 0 && ((pOpcode->nAddrMode1 == 0) || (pOpcode->nAddrMode1 & MODE_NONE))))
		{
			if((addrMode2.nMode & pOpcode->nAddrMode2)
			|| (addrMode2.nMode == 0 && ((pOpcode->nAddrMode2 == 0) || (pOpcode->nAddrMode2 & MODE_NONE))))
			{
				if((opt_Current->machineOptions->nCpu & pOpcode->nCpu)
				&& (opt_Current->machineOptions->nCpu & addrMode1.nCpu)
				&& (opt_Current->machineOptions->nCpu & addrMode2.nCpu))
				{
					return pOpcode->pParser(pOpcode, pOpcode->nAddrMode1 != 0 ? &addrMode1 : NULL, pOpcode->nAddrMode2 != 0 ? &addrMode2 : NULL);
				}
				else
					prj_Error(MERROR_INSTRUCTION_NOT_SUPPORTED_BY_CPU);
			}
			else
				prj_Error(ERROR_SECOND_OPERAND);
		}
		else
			prj_Error(ERROR_FIRST_OPERAND);

		return true;
	}

	return false;
}


SExpression* parse_TargetFunction(void)
{
	return NULL;
}
