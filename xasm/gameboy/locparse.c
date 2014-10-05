/*  Copyright 2008 Carsten Sørensen

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

#include <stdlib.h>
#include "xasm.h"
#include "expr.h"
#include "parse.h"
#include "section.h"
#include "project.h"
#include "lexer.h"
#include "localasm.h"
#include "locopt.h"
#include "options.h"

typedef enum
{
	REGD_B = 0,
	REGD_C,
	REGD_D,
	REGD_E,
	REGD_H,
	REGD_L,
	REGD_HL_IND,
	REGD_A
} ERegD;

typedef enum
{
	REGSS_BC = 0,
	REGSS_DE,
	REGSS_HL,
	REGSS_SP
} ERegSS;

typedef enum
{
	REGSSIX_BC = 0,
	REGSSIX_DE,
	REGSSIX_IX,
	REGSSIX_SP
} ERegSSIX;

typedef enum
{
	REGSSIY_BC = 0,
	REGSSIY_DE,
	REGSSIY_IY,
	REGSSIY_SP
} ERegSSIY;

typedef enum
{
	REGRR_BC_IND = 0,
	REGRR_DE_IND,
	REGRR_HL_INDINC,
	REGRR_HL_INDDEC
} ERegRR;

typedef enum
{
	REGTT_BC = 0,
	REGTT_DE,
	REGTT_HL,
	REGTT_AF
} ERegTT;

typedef enum
{
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
	ERegSSIX	eRegSSIX;
	ERegSSIY	eRegSSIY;
	ERegRR		eRegRR;
	ERegTT		eRegTT;
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
	bool_t (*pParser)(struct _Opcode* pCode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2);
} SOpcode;

extern SOpcode g_aOpcodes[T_Z80_XOR - T_Z80_ADC + 1];

#define MODE_NONE				0x00000001
#define MODE_REG_A				0x00000002
#define MODE_REG_C_IND			0x00000004
#define MODE_REG_DE				0x00000008
#define MODE_REG_HL				0x00000010
#define MODE_REG_SP				0x00000020
#define MODE_REG_AF				0x00000040
#define MODE_REG_AF_SEC			0x00000080
#define MODE_REG_IX				0x00000100
#define MODE_REG_IY				0x00000200
#define MODE_REG_BC_IND			0x00000400
#define MODE_REG_DE_IND			0x00000800
#define MODE_REG_HL_IND			0x00001000
#define MODE_REG_HL_INDDEC		0x00002000
#define MODE_REG_HL_INDINC		0x00004000
#define MODE_REG_SP_IND			0x00008000
#define MODE_REG_IX_IND			0x00010000
#define MODE_REG_IY_IND			0x00020000
#define MODE_REG_SP_IND_DISP	0x00040000
#define MODE_REG_IX_IND_DISP	0x00080000
#define MODE_REG_IY_IND_DISP	0x00100000
#define MODE_GROUP_D			0x00200000
#define MODE_GROUP_SS			0x00400000
#define MODE_GROUP_SSIX			0x00800000
#define MODE_GROUP_SSIY			0x01000000
#define MODE_GROUP_RR			0x02000000
#define MODE_GROUP_TT			0x04000000
#define MODE_IMM				0x08000000
#define MODE_IMM_IND			0x10000000
#define MODE_CC_GB				0x20000000
#define MODE_CC_Z80				0x40000000
#define MODE_REG_CONTROL		0x80000000

static SAddrMode s_AddressModes[T_CC_M - T_MODE_B + 1] =
{
	{ MODE_GROUP_D, NULL, CPUF_Z80 | CPUF_GB, REGD_B, -1, -1, -1, -1, -1, -1, -1 },	// B
	{ MODE_GROUP_D | MODE_CC_GB | MODE_CC_Z80, NULL, CPUF_Z80 | CPUF_GB, REGD_C, -1, -1, -1, -1, -1, CC_C, -1 },	// C
	{ MODE_GROUP_D, NULL, CPUF_Z80 | CPUF_GB, REGD_D, -1, -1, -1, -1, -1, -1, -1 },	// D
	{ MODE_GROUP_D, NULL, CPUF_Z80 | CPUF_GB, REGD_E, -1, -1, -1, -1, -1, -1, -1 },	// E
	{ MODE_GROUP_D, NULL, CPUF_Z80 | CPUF_GB, REGD_H, -1, -1, -1, -1, -1, -1, -1 },	// H
	{ MODE_GROUP_D, NULL, CPUF_Z80 | CPUF_GB, REGD_L, -1, -1, -1, -1, -1, -1, -1 },	// L
	{ MODE_REG_HL_IND | MODE_GROUP_D, NULL, CPUF_Z80 | CPUF_GB, REGD_HL_IND, -1, -1, -1, -1, -1, -1, -1 }, // (HL)
	{ MODE_REG_A | MODE_GROUP_D, NULL, CPUF_Z80 | CPUF_GB, REGD_A, -1, -1, -1, -1, -1, -1, -1 }, // A
	{ MODE_GROUP_SS | MODE_GROUP_SSIX | MODE_GROUP_SSIY | MODE_GROUP_TT, NULL, CPUF_Z80 | CPUF_GB, -1, REGSS_BC, REGSSIX_BC, REGSSIY_BC, -1, REGTT_BC, -1, -1 },	// BC
	{ MODE_REG_DE | MODE_GROUP_SS | MODE_GROUP_SSIX | MODE_GROUP_SSIY | MODE_GROUP_TT, NULL, CPUF_Z80 | CPUF_GB, -1, REGSS_DE, REGSSIX_DE, REGSSIY_DE, -1, REGTT_DE, -1, -1 },	// DE
	{ MODE_REG_HL | MODE_GROUP_SS | MODE_GROUP_TT, NULL, CPUF_Z80 | CPUF_GB, -1, REGSS_HL, -1, -1, -1, REGTT_HL, -1, -1 },	// HL
	{ MODE_REG_SP | MODE_GROUP_SS | MODE_GROUP_SSIX | MODE_GROUP_SSIY, NULL, CPUF_Z80 | CPUF_GB, -1, REGSS_SP, REGSSIX_SP, REGSSIY_SP, -1, -1, -1, -1 },	// SP
	{ MODE_REG_IX | MODE_GROUP_SSIX, NULL, CPUF_Z80, -1, -1, REGSSIX_IX, -1, -1, REGTT_HL, -1, -1 },	// IX
	{ MODE_REG_IY | MODE_GROUP_SSIY, NULL, CPUF_Z80, -1, -1, -1, REGSSIY_IY, -1, REGTT_HL, -1, -1 },	// IY
	{ MODE_REG_C_IND, NULL, CPUF_Z80 | CPUF_GB, -1, -1, -1, -1, -1, -1, -1, -1 },	// (C)
	{ MODE_REG_SP_IND, NULL, CPUF_Z80 | CPUF_GB, -1, -1, -1, -1, -1, -1, -1, -1 },	// (SP)
	{ MODE_REG_BC_IND | MODE_REG_C_IND | MODE_GROUP_RR, NULL, CPUF_Z80 | CPUF_GB, -1, -1, -1, -1, REGRR_BC_IND, -1, -1, -1 },	// (BC)
	{ MODE_REG_DE_IND | MODE_GROUP_RR, NULL, CPUF_Z80 | CPUF_GB, -1, -1, -1, -1, REGRR_DE_IND, -1, -1, -1 },	// (DE)
	{ MODE_REG_HL_INDDEC | MODE_GROUP_RR, NULL, CPUF_GB, -1, -1, -1, -1, REGRR_HL_INDDEC, -1, -1, -1 },	// (HL-)
	{ MODE_REG_HL_INDINC | MODE_GROUP_RR, NULL, CPUF_GB, -1, -1, -1, -1, REGRR_HL_INDINC, -1, -1, -1 },	// (HL+)
	{ MODE_REG_AF | MODE_GROUP_TT, NULL, CPUF_Z80 | CPUF_GB, -1, -1, -1, -1, -1, REGTT_AF, -1, -1 },	// AF
	{ MODE_REG_AF_SEC, NULL, CPUF_Z80, -1, -1, -1, -1, -1, -1, -1, -1 },	// AF'
	{ MODE_REG_CONTROL, NULL, CPUF_Z80, -1, -1, -1, -1, -1, -1, -1, CTRL_I },	// I
	{ MODE_REG_CONTROL, NULL, CPUF_Z80, -1, -1, -1, -1, -1, -1, -1, CTRL_R },	// R
	{ MODE_CC_GB | MODE_CC_Z80, NULL, CPUF_Z80 | CPUF_GB, -1, -1, -1, -1, -1, -1, CC_NZ, -1 },	// NZ
	{ MODE_CC_GB | MODE_CC_Z80, NULL, CPUF_Z80 | CPUF_GB, -1, -1, -1, -1, -1, -1, CC_Z, -1 },	// Z
	{ MODE_CC_GB | MODE_CC_Z80, NULL, CPUF_Z80 | CPUF_GB, -1, -1, -1, -1, -1, -1, CC_NC, -1 },	// NC
	{ MODE_CC_Z80, NULL, CPUF_Z80, -1, -1, -1, -1, -1, -1, CC_PO, -1 },	// PO
	{ MODE_CC_Z80, NULL, CPUF_Z80, -1, -1, -1, -1, -1, -1, CC_PE, -1 },	// PO
	{ MODE_CC_Z80, NULL, CPUF_Z80, -1, -1, -1, -1, -1, -1, CC_P, -1 },	// P
	{ MODE_CC_Z80, NULL, CPUF_Z80, -1, -1, -1, -1, -1, -1, CC_M, -1 }	// M
};

#define MODE_GROUP_EX (MODE_REG_SP_IND | MODE_REG_HL | MODE_REG_AF | MODE_REG_AF_SEC | MODE_REG_DE | MODE_REG_IX | MODE_REG_IY)
#define MODE_GROUP_IX_IND_DISP (MODE_REG_IX_IND | MODE_REG_IX_IND_DISP)
#define MODE_GROUP_IY_IND_DISP (MODE_REG_IY_IND | MODE_REG_IY_IND_DISP)
#define MODE_GROUP_I_IND_DISP (MODE_GROUP_IX_IND_DISP | MODE_GROUP_IY_IND_DISP)

#define IS_Z80 (g_pOptions->pMachine->nCpu & CPUF_Z80)
#define IS_GB  (g_pOptions->pMachine->nCpu & CPUF_GB)

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

static bool_t parse_Alu(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->nMode & MODE_REG_A)
	{
		if(pAddrMode2->nMode & MODE_GROUP_I_IND_DISP)
		{
			if(IS_Z80)
			{
				if(pAddrMode2->nMode & MODE_GROUP_IX_IND_DISP)
					sect_OutputConst8(0xDD);
				else
					sect_OutputConst8(0xFD);
				sect_OutputConst8((uint8_t)(0x86 | pOpcode->nOpcode));

				if(pAddrMode2->pExpr != NULL)
					sect_OutputExpr8(parse_CreateExpression8S(pAddrMode2->pExpr));
				else
					sect_OutputConst8(0);

				return true;
			}

			prj_Error(MERROR_INSTRUCTION_NOT_SUPPORTED_BY_CPU);
			return true;
		}

		if(pAddrMode2->nMode & MODE_GROUP_D)
		{
			sect_OutputConst8((uint8_t)(0x80 | pOpcode->nOpcode | pAddrMode2->eRegD));
			return true;
		}

		sect_OutputConst8(0xC6 | pOpcode->nOpcode);
		sect_OutputExpr8(parse_CreateExpression8SU(pAddrMode2->pExpr));
		return true;
	}

	return false;
}

static bool_t parse_Alu_16bit(SOpcode* pOpcode, int nAddrMode1, int nAddrMode2, int nPrefix, int nOpcode, int nReg, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->nMode & nAddrMode1)
	{
		if(pAddrMode2->nMode & nAddrMode2)
		{
			if(nPrefix != 0)
				sect_OutputConst8(nPrefix);
			sect_OutputConst8(nOpcode | (nReg << 4));
			return true;
		}

		prj_Error(ERROR_SECOND_OPERAND);
		return true;
	}

	return false;
}

static bool_t parse_AluHL(SOpcode* pOpcode, int nPrefix, int nOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	return parse_Alu_16bit(pOpcode, MODE_REG_HL, MODE_GROUP_SS, nPrefix, nOpcode, pAddrMode2->eRegSS, pAddrMode1, pAddrMode2);
}

static bool_t parse_AluIX(SOpcode* pOpcode, int nOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	return parse_Alu_16bit(pOpcode, MODE_REG_IX, MODE_GROUP_SSIX, 0xDD, nOpcode, pAddrMode2->eRegSSIX, pAddrMode1, pAddrMode2);
}

static bool_t parse_AluIY(SOpcode* pOpcode, int nOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	return parse_Alu_16bit(pOpcode, MODE_REG_IY, MODE_GROUP_SSIY, 0xFD, nOpcode, pAddrMode2->eRegSSIY, pAddrMode1, pAddrMode2);
}

static bool_t parse_Adc(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(IS_Z80 && parse_AluHL(pOpcode, 0xED, 0x4A, pAddrMode1, pAddrMode2))
		return true;

	return parse_Alu(pOpcode, pAddrMode1, pAddrMode2);
}

static bool_t parse_Sbc(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(IS_Z80 && parse_AluHL(pOpcode, 0xED, 0x42, pAddrMode1, pAddrMode2))
		return true;

	return parse_Alu(pOpcode, pAddrMode1, pAddrMode2);
}


static bool_t parse_Add(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(parse_AluHL(pOpcode, 0, 0x09, pAddrMode1, pAddrMode2))
		return true;

	if(IS_Z80 && parse_AluIX(pOpcode, 0x09, pAddrMode1, pAddrMode2))
		return true;

	if(IS_Z80 && parse_AluIY(pOpcode, 0x09, pAddrMode1, pAddrMode2))
		return true;

	if(IS_GB && (pAddrMode1->nMode & MODE_REG_SP) && (pAddrMode2->nMode & MODE_IMM))
	{
		sect_OutputConst8(0xE8);
		sect_OutputExpr8(parse_CreateExpression8SU(pAddrMode2->pExpr));
		return true;
	}



	return parse_Alu(pOpcode, pAddrMode1, pAddrMode2);

}

static bool_t parse_Bit(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	int nOpcode = pOpcode->nOpcode | pAddrMode2->eRegD;

	if(pAddrMode2->nMode & MODE_GROUP_I_IND_DISP)
	{
		if(IS_Z80)
		{
			sect_OutputConst8(pAddrMode2->nMode & MODE_GROUP_IX_IND_DISP ? 0xDD : 0xFD);
			sect_OutputConst8(0xCB);
			sect_OutputExpr8(parse_CreateExpression8S(pAddrMode2->pExpr));
			nOpcode = pOpcode->nOpcode | 6;
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
			expr_Const(nOpcode),
			expr_Shl(
			parse_CreateExpression3U(pAddrMode1->pExpr),
				expr_Const(3))
			)
		);

	return true;
}

static bool_t parse_Call(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
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
		sect_OutputConst8((uint8_t)((pOpcode->nOpcode & ~0x19) | (pAddrMode1->eModeF << 3)));
		sect_OutputExpr16(parse_CreateExpression16U(pAddrMode2->pExpr));
	}
	else
	{
		prj_Error(ERROR_OPERAND);
	}

	return true;
}

static bool_t parse_Jp(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
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

static bool_t parse_Implied(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pOpcode->nPrefix != 0)
		sect_OutputConst8(pOpcode->nPrefix);
	sect_OutputConst8(pOpcode->nOpcode);
	return true;
}

static bool_t parse_Dec(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->nMode & MODE_GROUP_SS)
	{
		sect_OutputConst8((uint8_t)(0x03 | (pOpcode->nOpcode << 3) | (pAddrMode1->eRegSS << 4)));
	}
	else if(pAddrMode1->nMode & MODE_GROUP_I_IND_DISP)
	{
		sect_OutputConst8(pAddrMode1->nMode & MODE_GROUP_IX_IND_DISP ? 0xDD : 0xFD);
		sect_OutputConst8((uint8_t)(0x04 | pOpcode->nOpcode | (6 << 3)));
		sect_OutputExpr8(parse_CreateExpression8S(pAddrMode1->pExpr));
	}
	else if(pAddrMode1->nMode & MODE_REG_IX)
	{
		sect_OutputConst8(0xDD);
		sect_OutputConst8((uint8_t)(0x03 | (pOpcode->nOpcode << 3) | (2 << 4)));
	}
	else if(pAddrMode1->nMode & MODE_REG_IY)
	{
		sect_OutputConst8(0xFD);
		sect_OutputConst8((uint8_t)(0x03 | (pOpcode->nOpcode << 3) | (2 << 4)));
	}
	else
	{
		sect_OutputConst8((uint8_t)(0x04 | pOpcode->nOpcode | (pAddrMode1->eRegD << 3)));
	}
	return true;
}

static bool_t parse_Jr(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if((pAddrMode1->nMode & MODE_IMM) && pAddrMode2->nMode == 0)
	{
		sect_OutputConst8(0x18);
		sect_OutputExpr8(parse_CreateExpressionPCRel(pAddrMode1->pExpr));
	}
	else if((pAddrMode1->nMode & MODE_CC_GB) && (pAddrMode2->nMode & MODE_IMM))
	{
		sect_OutputConst8((uint8_t)(0x20 | (pAddrMode1->eModeF << 3)));
		sect_OutputExpr8(parse_CreateExpressionPCRel(pAddrMode2->pExpr));
	}
	else
		prj_Error(ERROR_OPERAND);

	return true;
}

static bool_t parse_Ld(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if((pAddrMode1->nMode & MODE_GROUP_D) && (pAddrMode2->nMode & MODE_GROUP_D)
	&& (pAddrMode1->eRegD != REGD_HL_IND || pAddrMode2->eRegD != REGD_HL_IND))
	{
		sect_OutputConst8((uint8_t)(0x40 | (pAddrMode1->eRegD << 3) | pAddrMode2->eRegD));
	}
	else if((pAddrMode1->nMode & MODE_REG_A) && (pAddrMode2->nMode & MODE_GROUP_RR)
	&& (IS_GB || pAddrMode2->eRegRR <= REGRR_DE_IND))
	{
		sect_OutputConst8((uint8_t)(0x0A | (pAddrMode2->eRegRR << 4)));
	}
	else if((pAddrMode1->nMode & MODE_REG_A) && (pAddrMode2->nMode & MODE_IMM_IND))
	{
		if(IS_GB && expr_IsConstant(pAddrMode2->pExpr)
		&& pAddrMode2->pExpr->Value.Value >= 0xFF00 
		&& pAddrMode2->pExpr->Value.Value <= 0xFFFF)
		{
			sect_OutputConst8(0xF0);
			sect_OutputExpr8(parse_CreateExpressionImmHi(pAddrMode2->pExpr));
		}
		else
		{
			sect_OutputConst8(IS_GB ? 0xFA : 0x3A);
			sect_OutputExpr16(parse_CreateExpression16U(pAddrMode2->pExpr));
		}
	}
	else if((pAddrMode1->nMode & MODE_GROUP_D) && (pAddrMode2->nMode & MODE_IMM))
	{
		sect_OutputConst8((uint8_t)(0x06 | (pAddrMode1->eRegD << 3)));
		sect_OutputExpr8(parse_CreateExpression8SU(pAddrMode2->pExpr));
	}
	else if((pAddrMode1->nMode & MODE_IMM_IND) && (pAddrMode2->nMode & MODE_REG_A))
	{
		if(IS_GB && expr_IsConstant(pAddrMode1->pExpr)
		&& pAddrMode1->pExpr->Value.Value >= 0xFF00 
		&& pAddrMode1->pExpr->Value.Value <= 0xFFFF)
		{
			sect_OutputConst8(0xE0);
			sect_OutputExpr8(parse_CreateExpressionImmHi(pAddrMode1->pExpr));
		}
		else
		{
			sect_OutputConst8(IS_GB ? 0xEA : 0x32);
			sect_OutputExpr16(parse_CreateExpression16U(pAddrMode1->pExpr));
		}
	}
	else if((pAddrMode1->nMode & MODE_REG_SP) && (pAddrMode2->nMode & MODE_REG_HL))
	{
		sect_OutputConst8(0xF9);
	}
	else if((IS_GB || pAddrMode1->eRegRR <= REGRR_DE_IND) && (pAddrMode1->nMode & MODE_GROUP_RR) && (pAddrMode2->nMode & MODE_REG_A))
	{
		sect_OutputConst8((uint8_t)(0x02 | (pAddrMode1->eRegRR << 4)));
	}
	else if(IS_GB && (pAddrMode1->nMode & MODE_REG_A) && (pAddrMode2->nMode & MODE_REG_C_IND))
	{
		sect_OutputConst8(0xF2);
	}
	else if(IS_GB && (pAddrMode1->nMode & MODE_REG_HL) && (pAddrMode2->nMode & MODE_REG_SP_IND_DISP))
	{
		sect_OutputConst8(0xF8);
		sect_OutputExpr8(pAddrMode2->pExpr);
	}
	else if(IS_GB && (pAddrMode1->nMode & MODE_GROUP_SS) && (pAddrMode2->nMode & MODE_IMM))
	{
		sect_OutputConst8((uint8_t)(0x01 | (pAddrMode1->eRegSS << 4)));
		sect_OutputExpr16(parse_CreateExpression16U(pAddrMode2->pExpr));
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
	else if(IS_Z80 && (pAddrMode1->nMode & MODE_IMM_IND) && (pAddrMode2->nMode & MODE_GROUP_SS))
	{
		switch(pAddrMode2->eRegSS)
		{
			case REGSS_BC:
				sect_OutputConst8(0xED);
				sect_OutputConst8(0x43);
				break;
			case REGSS_DE:
				sect_OutputConst8(0xED);
				sect_OutputConst8(0x53);
				break;
			case REGSS_HL:
				sect_OutputConst8(0x22);
				break;
			case REGSS_SP:
				sect_OutputConst8(0xED);
				sect_OutputConst8(0x73);
				break;
		}
		sect_OutputExpr16(parse_CreateExpression16U(pAddrMode1->pExpr));
	}
	else if(IS_Z80 && (pAddrMode1->nMode & MODE_REG_SP) && (pAddrMode2->nMode & (MODE_REG_IX | MODE_REG_IY)))
	{
		sect_OutputConst8(pAddrMode2->nMode & MODE_REG_IX ? 0xDD : 0xFD);
		sect_OutputConst8(0xF9);
	}
	else if(IS_Z80 && (pAddrMode1->nMode & MODE_IMM_IND) && (pAddrMode2->nMode & (MODE_REG_IX | MODE_REG_IY)))
	{
		sect_OutputConst8(pAddrMode2->nMode & MODE_REG_IX ? 0xDD : 0xFD);
		sect_OutputConst8(0x22);
		sect_OutputExpr16(parse_CreateExpression16U(pAddrMode1->pExpr));
	}
	else if(IS_Z80 && (pAddrMode1->nMode & (MODE_REG_IX | MODE_REG_IY)) && (pAddrMode2->nMode & MODE_IMM_IND))
	{
		sect_OutputConst8(pAddrMode1->nMode & MODE_REG_IX ? 0xDD : 0xFD);
		sect_OutputConst8(0x2A);
		sect_OutputExpr16(parse_CreateExpression16U(pAddrMode2->pExpr));
	}
	else if(IS_Z80 && (pAddrMode1->nMode & (MODE_REG_IX | MODE_REG_IY)) && (pAddrMode2->nMode & MODE_IMM))
	{
		sect_OutputConst8(pAddrMode1->nMode & MODE_REG_IX ? 0xDD : 0xFD);
		sect_OutputConst8(0x21);
		sect_OutputExpr16(parse_CreateExpression16U(pAddrMode2->pExpr));
	}
	else if(IS_Z80 && (pAddrMode1->nMode & MODE_GROUP_I_IND_DISP) && (pAddrMode2->nMode & MODE_GROUP_D))
	{
		sect_OutputConst8(pAddrMode1->nMode & MODE_GROUP_IX_IND_DISP ? 0xDD : 0xFD);
		sect_OutputConst8(0x70 | pAddrMode2->eRegD);
		sect_OutputExpr8(parse_CreateExpression8S(pAddrMode1->pExpr));
	}
	else if(IS_Z80 && (pAddrMode1->nMode & MODE_GROUP_I_IND_DISP) && (pAddrMode2->nMode & MODE_IMM))
	{
		sect_OutputConst8(pAddrMode1->nMode & MODE_GROUP_IX_IND_DISP ? 0xDD : 0xFD);
		sect_OutputConst8(0x36);
		sect_OutputExpr8(parse_CreateExpression8S(pAddrMode1->pExpr));
		sect_OutputExpr8(parse_CreateExpression8SU(pAddrMode2->pExpr));
	}
	else if(IS_Z80 && (pAddrMode1->nMode & MODE_GROUP_D) && (pAddrMode2->nMode & MODE_GROUP_I_IND_DISP))
	{
		sect_OutputConst8(pAddrMode2->nMode & MODE_REG_IX_IND_DISP ? 0xDD : 0xFD);
		sect_OutputConst8((uint8_t)(0x46 | (pAddrMode1->eRegD << 3)));
		sect_OutputExpr8(parse_CreateExpression8S(pAddrMode2->pExpr));
	}
	else if(IS_Z80 && (pAddrMode1->nMode & MODE_REG_A) && (pAddrMode2->nMode & MODE_REG_CONTROL))
	{
		sect_OutputConst8(0xED);
		sect_OutputConst8(0x57 | (pAddrMode2->eRegCtrl << 3));
	}
	else if(IS_Z80 && (pAddrMode1->nMode & MODE_REG_CONTROL) && (pAddrMode2->nMode & MODE_REG_A))
	{
		sect_OutputConst8(0xED);
		sect_OutputConst8(0x47 | (pAddrMode1->eRegCtrl << 3));
	}
	else if(IS_Z80 && (pAddrMode1->nMode & MODE_GROUP_SS) && (pAddrMode2->nMode & MODE_IMM_IND))
	{
		if(pAddrMode1->eRegSS == REGSS_HL)
		{
			sect_OutputConst8(0x2A);
		}
		else
		{
			sect_OutputConst8(0xED);
			sect_OutputConst8(0x4B | (pAddrMode1->eRegSS << 4));
		}
		sect_OutputExpr16(parse_CreateExpression16U(pAddrMode2->pExpr));
	}
	else if(IS_Z80 && (pAddrMode1->nMode & MODE_GROUP_SS) && (pAddrMode2->nMode & MODE_IMM))
	{
		sect_OutputConst8(0x01 | (pAddrMode1->eRegSS << 4));
		sect_OutputExpr16(parse_CreateExpression16U(pAddrMode2->pExpr));
	}
	else
		prj_Error(ERROR_OPERAND);

	return true;
}

static bool_t parse_Ldd(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(IS_GB && (pAddrMode1->nMode & MODE_REG_A) && (pAddrMode2->nMode & MODE_REG_HL_IND))
		sect_OutputConst8((uint8_t)(pOpcode->nOpcode | 0x08));
	else if(IS_GB && (pAddrMode1->nMode & MODE_REG_HL_IND) && (pAddrMode2->nMode & MODE_REG_A))
		sect_OutputConst8((uint8_t)pOpcode->nOpcode);
	else if(IS_Z80 && (pAddrMode1->nMode == 0) && (pAddrMode2->nMode == 0))
	{
		sect_OutputConst8(0xED);
		// Translate Gameboy opcode to Z80 equivalent...
		sect_OutputConst8(0xA0 | ((pOpcode->nOpcode & 0x10) >> 1));
	}
	else
		prj_Error(ERROR_OPERAND);

	return true;
}


static bool_t parse_Ldh(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if((pAddrMode1->nMode & MODE_REG_A) && (pAddrMode2->nMode & MODE_IMM_IND))
	{
		sect_OutputConst8((uint8_t)(pOpcode->nOpcode | 0x10));
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


static bool_t parse_Pop(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->nMode & (MODE_REG_IX | MODE_REG_IY))
		sect_OutputConst8(pAddrMode1->nMode & MODE_REG_IX ? 0xDD : 0xFD);

	sect_OutputConst8((uint8_t)(pOpcode->nOpcode | (pAddrMode1->eRegTT << 4)));
	return true;
}

static bool_t parse_Rotate(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->nMode & MODE_GROUP_I_IND_DISP)
	{
		sect_OutputConst8(pAddrMode1->nMode & MODE_REG_IX_IND_DISP ? 0xDD : 0xFD);
		sect_OutputConst8(0xCB);
		sect_OutputExpr8(parse_CreateExpression8S(pAddrMode1->pExpr));
	}
	else
		sect_OutputConst8(0xCB);

	sect_OutputConst8((uint8_t)(pOpcode->nOpcode | pAddrMode1->eRegD));
	return true;
}

static bool_t parse_Rr(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->eRegD == REGD_A)
		prj_Warn(MERROR_SUGGEST_OPCODE, "RRA");
	return parse_Rotate(pOpcode, pAddrMode1, pAddrMode2);
}

static bool_t parse_Rl(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->eRegD == REGD_A)
		prj_Warn(MERROR_SUGGEST_OPCODE, "RLA");
	return parse_Rotate(pOpcode, pAddrMode1, pAddrMode2);
}

static bool_t parse_Rrc(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->eRegD == REGD_A)
		prj_Warn(MERROR_SUGGEST_OPCODE, "RRCA");
	return parse_Rotate(pOpcode, pAddrMode1, pAddrMode2);
}

static bool_t parse_Rlc(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->eRegD == REGD_A)
		prj_Warn(MERROR_SUGGEST_OPCODE, "RLCA");
	return parse_Rotate(pOpcode, pAddrMode1, pAddrMode2);
}

static bool_t parse_Ret(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->nMode & MODE_CC_Z80)
		sect_OutputConst8((uint8_t)(0xC0 | (pAddrMode1->eModeF << 3)));
	else
		sect_OutputConst8(0xC9);

	return true;
}

static bool_t parse_Rst(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(expr_IsConstant(pAddrMode1->pExpr))
	{
		int32_t val = pAddrMode1->pExpr->Value.Value;
		if(val == (val & 0x38))
			sect_OutputConst8((uint8_t)(pOpcode->nOpcode | val));
		else
			prj_Error(ERROR_OPERAND_RANGE);
	}
	else
		prj_Error(ERROR_EXPR_CONST);

	return true;
}

static bool_t parse_Stop(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	sect_OutputConst8(0x10);
	sect_OutputConst8(0x00);
	return true;
}

static bool_t parse_Djnz(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	sect_OutputConst8(pOpcode->nOpcode);
	sect_OutputExpr8(parse_CreateExpressionPCRel(pAddrMode1->pExpr));
	return true;
}

#define EX_MATCH_MODE1(o1,o2) ((pAddrMode1->nMode & (o1)) && (pAddrMode2->nMode & (o2)))
#define EX_MATCH_MODE(o1,o2) (EX_MATCH_MODE1(o1,o2) || EX_MATCH_MODE1(o2,o1))

static bool_t parse_Ex(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(!IS_Z80)
	{
		prj_Error(MERROR_INSTRUCTION_NOT_SUPPORTED_BY_CPU);
		return true;
	}

	if(EX_MATCH_MODE(MODE_REG_SP_IND, MODE_REG_HL))
	{
		sect_OutputConst8(0xE3);
	}
	else if(EX_MATCH_MODE(MODE_REG_SP_IND, MODE_REG_IX))
	{
		sect_OutputConst8(0xDD);
		sect_OutputConst8(0xE3);
	}
	else if(EX_MATCH_MODE(MODE_REG_SP_IND, MODE_REG_IY))
	{
		sect_OutputConst8(0xFD);
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

static bool_t parse_Im(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(!expr_IsConstant(pAddrMode1->pExpr))
	{
		prj_Error(ERROR_EXPR_CONST);
		return true;
	}

	sect_OutputConst8(0xED);
	switch(pAddrMode1->pExpr->Value.Value)
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

	prj_Error(MERROR_INSTRUCTION_NOT_SUPPORTED_BY_CPU);
	return true;
}

static bool_t parse_InOut(SOpcode* pOpcode, SAddrMode* pAddrMode, ERegD eReg)
{
	if(pAddrMode->nMode & MODE_REG_C_IND)
	{
		sect_OutputConst8(pOpcode->nPrefix);
		sect_OutputConst8(pOpcode->nOpcode | (eReg << 3));
		return true;
	}

	sect_OutputConst8(0xDB ^ ((pOpcode->nOpcode & 1) << 3));
	sect_OutputExpr8(parse_CreateExpression8U(pAddrMode->pExpr));

	return true;
}

static bool_t parse_In(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	return parse_InOut(pOpcode, pAddrMode2, pAddrMode1->eRegD);
}

static bool_t parse_Out(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	return parse_InOut(pOpcode, pAddrMode1, pAddrMode2->eRegD);
}

static bool_t parse_Reti(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
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
	{ CPUF_GB | CPUF_Z80, 0x00, 0x00, MODE_REG_A | MODE_REG_HL | MODE_REG_SP | MODE_REG_IX | MODE_REG_IY, MODE_GROUP_D | MODE_IMM | MODE_GROUP_SS | MODE_GROUP_SSIX | MODE_GROUP_SSIY | MODE_GROUP_I_IND_DISP, parse_Add },	/* ADD */
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
	{ CPUF_GB | CPUF_Z80, 0x00, 0x01, MODE_GROUP_SS | MODE_GROUP_D | MODE_GROUP_I_IND_DISP | MODE_REG_IX | MODE_REG_IY, 0, parse_Dec },			/* DEC */
	{ CPUF_GB | CPUF_Z80, 0x00, 0xF3, 0, 0, parse_Implied },							/* DI */
	{ CPUF_Z80, 0x00, 0x10, MODE_IMM, 0, parse_Djnz },	/* DJNZ */
	{ CPUF_GB | CPUF_Z80, 0x00, 0xFB, 0, 0, parse_Implied },							/* EI */
	{ CPUF_Z80, 0x00, 0x00, MODE_GROUP_EX, MODE_GROUP_EX, parse_Ex },	/* EX */
	{ CPUF_Z80, 0x00, 0xD9, 0, 0, parse_Implied },	/* EXX */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x76, 0, 0, parse_Implied },							/* HALT */
	{ CPUF_Z80, 0xED, 0x46, MODE_IMM, 0, parse_Im },	/* IM */
	{ CPUF_Z80, 0xED, 0x40, MODE_GROUP_D, MODE_IMM_IND | MODE_REG_C_IND, parse_In },	/* IN */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x00, MODE_GROUP_SS | MODE_GROUP_D | MODE_GROUP_I_IND_DISP | MODE_REG_IX | MODE_REG_IY, 0, parse_Dec },			/* INC */
	{ CPUF_Z80, 0xED, 0xAA, 0, 0, parse_Implied },							/* IND */
	{ CPUF_Z80, 0xED, 0xBA, 0, 0, parse_Implied },							/* INDR */
	{ CPUF_Z80, 0xED, 0xA2, 0, 0, parse_Implied },							/* INI */
	{ CPUF_Z80, 0xED, 0xB2, 0, 0, parse_Implied },							/* INIR */
	{ CPUF_GB | CPUF_Z80, 0x00, 0xC3, MODE_CC_Z80 | MODE_IMM | MODE_REG_HL_IND | MODE_REG_IX_IND | MODE_REG_IY_IND, MODE_IMM | MODE_NONE, parse_Jp },	/* JP */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x00, MODE_CC_GB | MODE_IMM, MODE_IMM | MODE_NONE, parse_Jr },	/* JR */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x00, 
		MODE_REG_A | MODE_REG_C_IND | MODE_REG_HL | MODE_REG_SP | MODE_REG_IX | MODE_REG_IY | MODE_REG_CONTROL | MODE_GROUP_D | MODE_GROUP_RR | MODE_GROUP_SS | MODE_IMM_IND | MODE_GROUP_I_IND_DISP,
		MODE_REG_A | MODE_REG_C_IND | MODE_REG_HL | MODE_REG_SP | MODE_REG_IX | MODE_REG_IY | MODE_REG_CONTROL | MODE_REG_SP_IND_DISP | MODE_GROUP_D | MODE_GROUP_RR | MODE_IMM_IND | MODE_IMM | MODE_GROUP_SS | MODE_GROUP_I_IND_DISP, parse_Ld },	/* LD */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x32, MODE_NONE | MODE_REG_A | MODE_REG_HL_IND, MODE_NONE | MODE_REG_A | MODE_REG_HL_IND, parse_Ldd },	/* LDD */
	{ CPUF_Z80, 0xED, 0xB8, 0, 0, parse_Implied },	/* LDDR */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x22, MODE_NONE | MODE_REG_A | MODE_REG_HL_IND, MODE_NONE | MODE_REG_A | MODE_REG_HL_IND, parse_Ldd },	/* LDI */
	{ CPUF_Z80, 0xED, 0xB0, 0, 0, parse_Implied },	/* LDIR */
	{ CPUF_GB, 0x00, 0xE0, MODE_IMM_IND | MODE_REG_A, MODE_IMM_IND | MODE_REG_A, parse_Ldh },	/* LDH */
	{ CPUF_Z80, 0xED, 0x44, 0, 0, parse_Implied },	/* NEG */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x00, 0, 0, parse_Implied },	/* NOP */
	{ CPUF_GB | CPUF_Z80, 0x00, 0x30, MODE_REG_A, MODE_GROUP_D | MODE_IMM | MODE_GROUP_I_IND_DISP, parse_Alu },	/* OR */
	{ CPUF_Z80, 0xED, 0xBB, 0, 0, parse_Implied },	/* OTDR */
	{ CPUF_Z80, 0xED, 0xB3, 0, 0, parse_Implied },	/* OTIR */
	{ CPUF_Z80, 0xED, 0x41, MODE_IMM_IND | MODE_REG_C_IND, MODE_GROUP_D, parse_Out },	/* OUT */
	{ CPUF_Z80, 0xED, 0xAB, 0, 0, parse_Implied },	/* OUTD */
	{ CPUF_Z80, 0xED, 0xA3, 0, 0, parse_Implied },	/* OUTI */
	{ CPUF_GB | CPUF_Z80, 0x00, 0xC1, MODE_GROUP_TT | MODE_REG_IX | MODE_REG_IY, 0, parse_Pop },	/* POP */
	{ CPUF_GB | CPUF_Z80, 0x00, 0xC5, MODE_GROUP_TT | MODE_REG_IX | MODE_REG_IY, 0, parse_Pop },	/* PUSH */
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


static bool_t parse_AddrMode(SAddrMode* pAddrMode)
{
	if(g_CurrentToken.ID.TargetToken >= T_MODE_B
	&& g_CurrentToken.ID.TargetToken <= T_CC_M)
	{
		*pAddrMode = s_AddressModes[g_CurrentToken.ID.TargetToken - T_MODE_B];
		parse_GetToken();
		return pAddrMode->nCpu & g_pOptions->pMachine->nCpu ? true : false;
	}

	if(g_CurrentToken.ID.Token == '[' || g_CurrentToken.ID.Token == '(')
	{
		char endToken = g_CurrentToken.ID.Token == '[' ? ']' : ')';
		SLexBookmark bm;
		lex_Bookmark(&bm);

		parse_GetToken();
		if(g_CurrentToken.ID.TargetToken == T_MODE_SP
		|| g_CurrentToken.ID.TargetToken == T_MODE_IX
		|| g_CurrentToken.ID.TargetToken == T_MODE_IY)
		{
			int regToken = g_CurrentToken.ID.TargetToken;

			parse_GetToken();

			if(g_CurrentToken.ID.Token == T_OP_ADD
			|| g_CurrentToken.ID.Token == T_OP_SUB)
			{
				SExpression* pExpr;

				pExpr = parse_Expression();
				if(pExpr != NULL)
				{
					pExpr = parse_CreateExpression8S(pExpr);
					if(pExpr != NULL)
					{
						if(parse_ExpectChar(endToken))
						{
							pAddrMode->nMode |=
								regToken == T_MODE_SP ? MODE_REG_SP_IND_DISP :
								regToken == T_MODE_IX ? MODE_REG_IX_IND_DISP :
								/*regToken == T_MODE_IY ? */ MODE_REG_IY_IND_DISP;
							pAddrMode->pExpr = pExpr;
							return true;
						}
					}
				}
				expr_Free(pExpr);
			}
			else
			{
				if(parse_ExpectChar(endToken))
				{
					pAddrMode->nMode |=
						regToken == T_MODE_SP ? MODE_REG_SP_IND :
						regToken == T_MODE_IX ? MODE_REG_IX_IND :
						/*regToken == T_MODE_IY ? */ MODE_REG_IY_IND;
					pAddrMode->pExpr = NULL;
					return true;
				}
			}
		}
		else
		{
			SExpression* pExpr = parse_Expression();
			if(pExpr != NULL)
			{
				if(parse_ExpectChar(endToken))
				{
					pAddrMode->nMode |= MODE_IMM_IND;
					pAddrMode->pExpr = pExpr;
					return true;
				}
				expr_Free(pExpr);
			}

		}
		lex_Goto(&bm);
	}

	/* Try expression */
	{
		SExpression* pExpr;
		SLexBookmark bm;

		lex_Bookmark(&bm);
		pExpr = parse_Expression();
		if(pExpr != NULL)
		{
			pAddrMode->nMode |= MODE_IMM;
			pAddrMode->pExpr = pExpr;

			return true;
		}
		
		lex_Goto(&bm);
	}

	return false;
}

bool_t parse_TargetSpecific(void)
{
	if((g_CurrentToken.ID.TargetToken >= T_Z80_ADC && g_CurrentToken.ID.TargetToken <= T_Z80_XOR)
	|| g_CurrentToken.ID.TargetToken == T_POP_SET)
	{
		int nToken = (g_CurrentToken.ID.TargetToken == T_POP_SET ? T_Z80_SET : g_CurrentToken.ID.TargetToken) - T_Z80_ADC;
		SOpcode* pOpcode = &g_aOpcodes[nToken];
		SAddrMode addrMode1;
		SAddrMode addrMode2;

		parse_GetToken();

		addrMode1.nMode = 0;
		addrMode2.nMode = 0;

		if(pOpcode->nAddrMode1 != 0)
		{
			if(parse_AddrMode(&addrMode1))
			{
				if(g_CurrentToken.ID.Token == ',')
				{
					parse_GetToken();
					if(!parse_AddrMode(&addrMode2))
					{
						prj_Error(ERROR_SECOND_OPERAND);
						return true;
					}
				}
				else if(pOpcode->nAddrMode2 != 0
				&& (pOpcode->nAddrMode1 & REGD_A)
				&& (pOpcode->nAddrMode2 & addrMode1.nMode))
				{
					addrMode2 = addrMode1;
					addrMode1.nMode = MODE_REG_A | MODE_GROUP_D;
					addrMode1.eRegD = REGD_A;
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
				if(g_pOptions->pMachine->nCpu & pOpcode->nCpu)
				{
					return pOpcode->pParser(pOpcode, &addrMode1, &addrMode2);
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
