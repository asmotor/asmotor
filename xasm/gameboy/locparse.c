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

#include "xasm.h"
#include "expr.h"
#include "parse.h"
#include "section.h"
#include "project.h"
#include "lexer.h"
#include "localasm.h"

typedef enum
{
	REG_B = 0,
	REG_C,
	REG_D,
	REG_E,
	REG_H,
	REG_L,
	REG_HL_IND,
	REG_A
} ERegD;

typedef enum
{
	REG_BC = 0,
	REG_DE,
	REG_HL,
	REG_SP
} ERegSS;

typedef enum
{
	REG_AF = 3
} ERegTT;

typedef enum
{
	REG_BC_IND = 0,
	REG_DE_IND,
	REG_HL_INDINC,
	REG_HL_INDDEC
} ERegRR;

typedef enum
{
	CC_NZ = 0,
	CC_Z,
	CC_NC,
	CC_C
} EModeF;


typedef struct _AddrMode
{
	int	nMode;

	SExpression* pExpr;
	ERegD	eRegD;
	ERegSS	eRegSS;
	ERegRR	eRegRR;
	ERegTT	eRegTT;
	EModeF	eModeF;

} SAddrMode;

typedef struct _Opcode
{
	unsigned char nOpcode;
	int nAddrMode1;
	int nAddrMode2;
	bool_t (*pParser)(struct _Opcode* pCode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2);
} SOpcode;

extern SOpcode g_aOpcodes[T_Z80_XOR - T_Z80_ADC + 1];

#define MODE_NONE		0x0001
#define MODE_REG_A		0x0002
#define MODE_REG_C_IND	0x0004
#define MODE_REG_HL		0x0008
#define MODE_REG_HL_IND	0x0010
#define MODE_REG_SP		0x0020
#define MODE_REG_SP_IND	0x0040
#define MODE_REG_SP_IND_DISP	\
						0x0080
#define MODE_GROUP_D	0x0100
#define MODE_GROUP_SS	0x0200
#define MODE_GROUP_RR	0x0400
#define MODE_GROUP_TT	0x0800
#define MODE_IMM		0x1000
#define MODE_IMM_IND	0x2000
#define MODE_F			0x4000

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
	pExpr = expr_CreatePcRelativeExpr(pExpr, -1);
	return parse_CreateExpression8S(pExpr);
}

static SExpression* parse_CreateExpressionImmHi(SExpression* pExpr)
{
	pExpr = expr_CheckRange(pExpr, 0xFF00, 0xFFFF);
	if(pExpr == NULL)
		prj_Error(MERROR_EXPRESSION_FF00);

	return expr_CreateAndExpr(pExpr, expr_CreateConstExpr(0xFF));
}

static bool_t parse_Alu(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode2->nMode & MODE_GROUP_D)
	{
		sect_OutputAbint8_t((uint8_t)(0x80 | pOpcode->nOpcode | pAddrMode2->eRegD));
		return true;
	}

	sect_OutputAbint8_t(0xC6 | pOpcode->nOpcode);
	sect_OutputExprByte(parse_CreateExpression8SU(pAddrMode2->pExpr));
	return true;
}

static bool_t parse_Add(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->nMode & MODE_REG_A && (pAddrMode2->nMode & (MODE_IMM | MODE_GROUP_D)))
		return parse_Alu(pOpcode, pAddrMode1, pAddrMode2);
	else if((pAddrMode1->nMode & MODE_REG_HL) && (pAddrMode2->nMode & MODE_GROUP_SS))
		sect_OutputAbint8_t((uint8_t)(0x09 | (pAddrMode2->eRegSS << 4)));
	else if((pAddrMode1->nMode & MODE_REG_SP) && (pAddrMode2->nMode & MODE_IMM))
	{
		sect_OutputAbint8_t(0xE8);
		sect_OutputExprByte(parse_CreateExpression8SU(pAddrMode2->pExpr));
	}

	return true;
}

static bool_t parse_Bit(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	sect_OutputAbint8_t(0xCB);
	sect_OutputExprByte(
		expr_CreateOrExpr(
			expr_CreateConstExpr(pOpcode->nOpcode | pAddrMode2->eRegD),
			expr_CreateShlExpr(
			parse_CreateExpression3U(pAddrMode1->pExpr),
				expr_CreateConstExpr(3))
			)
		);

	return true;
}

static bool_t parse_Call(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if((pAddrMode1->nMode & MODE_IMM) && pAddrMode2->nMode == 0)
	{
		sect_OutputAbint8_t(pOpcode->nOpcode);
		sect_OutputExprWord(parse_CreateExpression16U(pAddrMode1->pExpr));
	}
	else if((pAddrMode1->nMode & MODE_F) && (pAddrMode2->nMode & MODE_IMM))
	{
		sect_OutputAbint8_t((uint8_t)((pOpcode->nOpcode & ~0x19) | (pAddrMode1->eModeF << 3)));
		sect_OutputExprWord(parse_CreateExpression16U(pAddrMode2->pExpr));
	}
	else
		prj_Error(ERROR_OPERAND);

	return true;
}

static bool_t parse_Jp(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if((pAddrMode1->nMode & MODE_REG_HL_IND) && pAddrMode2->nMode == 0)
	{
		sect_OutputAbint8_t(0xE9);
		return true;
	}

	return parse_Call(pOpcode, pAddrMode1, pAddrMode2);
}

static bool_t parse_Implied(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	sect_OutputAbint8_t(pOpcode->nOpcode);
	return true;
}

static bool_t parse_Dec(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->nMode & MODE_GROUP_SS)
		sect_OutputAbint8_t((uint8_t)(0x03 | (pOpcode->nOpcode << 3) | (pAddrMode1->eRegSS << 4)));
	else
		sect_OutputAbint8_t((uint8_t)(0x04 | pOpcode->nOpcode | (pAddrMode1->eRegD << 3)));
	return true;
}

static bool_t parse_Jr(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if((pAddrMode1->nMode & MODE_IMM) && pAddrMode2->nMode == 0)
	{
		sect_OutputAbint8_t(0x18);
		sect_OutputExprByte(parse_CreateExpressionPCRel(pAddrMode1->pExpr));
	}
	else if((pAddrMode1->nMode & MODE_F) && (pAddrMode2->nMode & MODE_IMM))
	{
		sect_OutputAbint8_t((uint8_t)(0x20 | (pAddrMode1->eModeF << 3)));
		sect_OutputExprByte(parse_CreateExpressionPCRel(pAddrMode2->pExpr));
	}
	else
		prj_Error(ERROR_OPERAND);

	return true;
}

static bool_t parse_Ld(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if((pAddrMode1->nMode & MODE_GROUP_D) && (pAddrMode2->nMode & MODE_GROUP_D))
	{
		if(pAddrMode1->eRegD == REG_HL_IND && pAddrMode2->eRegD == REG_HL_IND)
			prj_Error(ERROR_OPERAND);
		else
			sect_OutputAbint8_t((uint8_t)(0x40 | (pAddrMode1->eRegD << 3) | pAddrMode2->eRegD));
	}
	else if((pAddrMode1->nMode & MODE_REG_A) && (pAddrMode2->nMode & MODE_GROUP_RR))
	{
		sect_OutputAbint8_t((uint8_t)(0x0A | (pAddrMode2->eRegRR << 4)));
	}
	else if((pAddrMode1->nMode & MODE_REG_A) && (pAddrMode2->nMode & MODE_REG_C_IND))
	{
		sect_OutputAbint8_t(0xF2);
	}
	else if((pAddrMode1->nMode & MODE_REG_A) && (pAddrMode2->nMode & MODE_IMM_IND))
	{
		if((pAddrMode2->pExpr->Flags & EXPRF_isCONSTANT)
		&& pAddrMode2->pExpr->Value.Value >= 0xFF00 
		&& pAddrMode2->pExpr->Value.Value <= 0xFFFF)
		{
			sect_OutputAbint8_t(0xF0);
			sect_OutputExprByte(parse_CreateExpressionImmHi(pAddrMode2->pExpr));
		}
		else
		{
			sect_OutputAbint8_t(0xFA);
			sect_OutputExprWord(parse_CreateExpression16U(pAddrMode2->pExpr));
		}
	}
	else if((pAddrMode1->nMode & MODE_GROUP_D) && (pAddrMode2->nMode & MODE_IMM))
	{
		sect_OutputAbint8_t((uint8_t)(0x06 | (pAddrMode1->eRegD << 3)));
		sect_OutputExprByte(parse_CreateExpression8SU(pAddrMode2->pExpr));
	}
	else if((pAddrMode1->nMode & MODE_GROUP_RR) && (pAddrMode2->nMode & MODE_REG_A))
	{
		sect_OutputAbint8_t((uint8_t)(0x02 | (pAddrMode1->eRegRR << 4)));
	}
	else if((pAddrMode1->nMode & MODE_REG_SP) && (pAddrMode2->nMode & MODE_REG_HL))
	{
		sect_OutputAbint8_t(0xF9);
	}
	else if((pAddrMode1->nMode & MODE_REG_HL) && (pAddrMode2->nMode & MODE_REG_SP_IND_DISP))
	{
		sect_OutputAbint8_t(0xF8);
		sect_OutputExprByte(pAddrMode2->pExpr);
	}
	else if((pAddrMode1->nMode & MODE_GROUP_SS) && (pAddrMode2->nMode & MODE_IMM))
	{
		sect_OutputAbint8_t((uint8_t)(0x01 | (pAddrMode1->eRegSS << 4)));
		sect_OutputExprWord(parse_CreateExpression16U(pAddrMode2->pExpr));
	}
	else if((pAddrMode1->nMode & MODE_REG_C_IND) && (pAddrMode2->nMode & MODE_REG_A))
	{
		sect_OutputAbint8_t(0xE2);
	}
	else if((pAddrMode1->nMode & MODE_IMM_IND) && (pAddrMode2->nMode & MODE_REG_SP))
	{
		sect_OutputAbint8_t(0x08);
		sect_OutputExprWord(parse_CreateExpression16U(pAddrMode1->pExpr));
	}
	else if((pAddrMode1->nMode & MODE_IMM_IND) && (pAddrMode2->nMode & MODE_REG_A))
	{
		if((pAddrMode1->pExpr->Flags & EXPRF_isCONSTANT)
		&& pAddrMode1->pExpr->Value.Value >= 0xFF00 
		&& pAddrMode1->pExpr->Value.Value <= 0xFFFF)
		{
			sect_OutputAbint8_t(0xE0);
			sect_OutputExprByte(parse_CreateExpressionImmHi(pAddrMode1->pExpr));
		}
		else
		{
			sect_OutputAbint8_t(0xEA);
			sect_OutputExprWord(parse_CreateExpression16U(pAddrMode1->pExpr));
		}
	}
	else
		prj_Error(ERROR_OPERAND);

	return true;
}

static bool_t parse_Ldd(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if((pAddrMode1->nMode & MODE_REG_A) && (pAddrMode2->nMode & MODE_REG_HL_IND))
		sect_OutputAbint8_t((uint8_t)(pOpcode->nOpcode | 0x08));
	else if((pAddrMode1->nMode & MODE_REG_HL_IND) && (pAddrMode2->nMode & MODE_REG_A))
		sect_OutputAbint8_t((uint8_t)pOpcode->nOpcode);
	else
		prj_Error(ERROR_OPERAND);

	return true;
}


static bool_t parse_Ldh(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if((pAddrMode1->nMode & MODE_REG_A) && (pAddrMode2->nMode & MODE_IMM_IND))
	{
		sect_OutputAbint8_t((uint8_t)(pOpcode->nOpcode | 0x10));
		sect_OutputExprByte(parse_CreateExpressionImmHi(pAddrMode2->pExpr));
	}
	else if((pAddrMode1->nMode & MODE_IMM_IND) && (pAddrMode2->nMode & MODE_REG_A))
	{
		sect_OutputAbint8_t(pOpcode->nOpcode);
		sect_OutputExprByte(parse_CreateExpressionImmHi(pAddrMode1->pExpr));
	}
	else
		prj_Error(ERROR_OPERAND);

	return true;
}


static bool_t parse_Pop(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	sect_OutputAbint8_t((uint8_t)(pOpcode->nOpcode | (pAddrMode1->eRegTT << 4)));
	return true;
}

static bool_t parse_Rotate(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	sect_OutputAbint8_t(0xCB);
	sect_OutputAbint8_t((uint8_t)(pOpcode->nOpcode | pAddrMode1->eRegD));
	return true;
}

static bool_t parse_Rr(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->eRegD == REG_A)
		prj_Warn(MERROR_SUGGEST_OPCODE, "RRA");
	return parse_Rotate(pOpcode, pAddrMode1, pAddrMode2);
}

static bool_t parse_Rl(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->eRegD == REG_A)
		prj_Warn(MERROR_SUGGEST_OPCODE, "RLA");
	return parse_Rotate(pOpcode, pAddrMode1, pAddrMode2);
}

static bool_t parse_Rrc(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->eRegD == REG_A)
		prj_Warn(MERROR_SUGGEST_OPCODE, "RRCA");
	return parse_Rotate(pOpcode, pAddrMode1, pAddrMode2);
}

static bool_t parse_Rlc(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->eRegD == REG_A)
		prj_Warn(MERROR_SUGGEST_OPCODE, "RLCA");
	return parse_Rotate(pOpcode, pAddrMode1, pAddrMode2);
}

static bool_t parse_Ret(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->nMode & MODE_F)
		sect_OutputAbint8_t((uint8_t)(0xC0 | (pAddrMode1->eModeF << 3)));
	else
		sect_OutputAbint8_t(0xC9);

	return true;
}

static bool_t parse_Rst(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->pExpr->Flags & EXPRF_isCONSTANT)
	{
		int32_t val = pAddrMode1->pExpr->Value.Value;
		if(val == (val & 0x38))
			sect_OutputAbint8_t((uint8_t)(pOpcode->nOpcode | val));
		else
			prj_Error(ERROR_OPERAND_RANGE);
	}
	else
		prj_Error(ERROR_EXPR_CONST);

	return true;
}

static bool_t parse_Stop(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	sect_OutputAbint8_t(0x10);
	sect_OutputAbint8_t(0x00);
	return true;
}

SOpcode g_aOpcodes[T_Z80_XOR - T_Z80_ADC + 1] =
{
	{ 0x08, MODE_REG_A, MODE_GROUP_D | MODE_IMM, parse_Alu },	/* ADC */
	{ 0x00, MODE_REG_A | MODE_REG_HL | MODE_REG_SP, MODE_GROUP_D | MODE_IMM | MODE_GROUP_SS, parse_Add },	/* ADD */
	{ 0x20, MODE_REG_A, MODE_GROUP_D | MODE_IMM, parse_Alu },	/* AND */
	{ 0x40, MODE_IMM, MODE_GROUP_D, parse_Bit },				/* BIT */
	{ 0xCD, MODE_F | MODE_IMM, MODE_IMM | MODE_NONE, parse_Call },	/* CALL */
	{ 0x3F, 0, 0, parse_Implied },							/* CCF */
	{ 0x2F, 0, 0, parse_Implied },							/* CPL */
	{ 0x38, MODE_REG_A, MODE_GROUP_D | MODE_IMM, parse_Alu },	/* CP */
	{ 0x27, 0, 0, parse_Implied },							/* DAA */
	{ 0x01, MODE_GROUP_SS | MODE_GROUP_D, 0, parse_Dec },			/* DEC */
	{ 0xF3, 0, 0, parse_Implied },							/* DI */
	{ 0xFB, 0, 0, parse_Implied },							/* EI */
	{ 0x76, 0, 0, parse_Implied },							/* HALT */
	{ 0x00, MODE_GROUP_SS | MODE_GROUP_D, 0, parse_Dec },			/* INC */
	{ 0xC3, MODE_F | MODE_IMM | MODE_REG_HL_IND, MODE_IMM | MODE_NONE, parse_Jp },	/* JP */
	{ 0x00, MODE_F | MODE_IMM, MODE_IMM | MODE_NONE, parse_Jr },	/* JR */
	{ 0x00,
		MODE_REG_A | MODE_REG_C_IND | MODE_REG_HL | MODE_REG_SP | MODE_GROUP_D | MODE_GROUP_RR | MODE_GROUP_SS | MODE_IMM_IND,
		MODE_REG_A | MODE_REG_C_IND | MODE_REG_HL | MODE_REG_SP | MODE_REG_SP_IND_DISP | MODE_GROUP_D | MODE_GROUP_RR | MODE_IMM_IND | MODE_IMM, parse_Ld },	/* LD */
	{ 0x32, MODE_REG_A | MODE_REG_HL_IND, MODE_REG_A | MODE_REG_HL_IND, parse_Ldd },	/* LDD */
	{ 0x22, MODE_REG_A | MODE_REG_HL_IND, MODE_REG_A | MODE_REG_HL_IND, parse_Ldd },	/* LDI */
	{ 0xE0, MODE_IMM_IND | MODE_REG_A, MODE_IMM_IND | MODE_REG_A, parse_Ldh },	/* LDH */
	{ 0x00, 0, 0, parse_Implied },	/* LDH */
	{ 0x30, MODE_REG_A, MODE_GROUP_D | MODE_IMM, parse_Alu },	/* OR */
	{ 0xC1, MODE_GROUP_TT, 0, parse_Pop },	/* POP */
	{ 0xC5, MODE_GROUP_TT, 0, parse_Pop },	/* PUSH */
	{ 0x80, MODE_IMM, MODE_GROUP_D, parse_Bit },				/* RES */
	{ 0xC0, MODE_NONE | MODE_F, 0, parse_Ret },	/* RET */
	{ 0xD9, 0, 0, parse_Implied },	/* RETI */
	{ 0x07, 0, 0, parse_Implied },	/* RLCA */
	{ 0x00, MODE_GROUP_D, 0, parse_Rlc },	/* RLC */
	{ 0x17, 0, 0, parse_Implied },	/* RLA */
	{ 0x10, MODE_GROUP_D, 0, parse_Rl },	/* RL */
	{ 0x08, MODE_GROUP_D, 0, parse_Rrc },	/* RRC */
	{ 0x0F, 0, 0, parse_Implied },	/* RRCA */
	{ 0x1F, 0, 0, parse_Implied },	/* RRA */
	{ 0x18, MODE_GROUP_D, 0, parse_Rr },	/* RR */
	{ 0xC7, MODE_IMM, 0, parse_Rst },	/* RST */
	{ 0x18, MODE_REG_A, MODE_GROUP_D | MODE_IMM, parse_Alu },	/* SBC */
	{ 0x37, 0, 0, parse_Implied },	/* SCF */
	{ 0xC0, MODE_IMM, MODE_GROUP_D, parse_Bit },				/* SET */
	{ 0x20, MODE_GROUP_D, 0, parse_Rotate },	/* SLA */
	{ 0x28, MODE_GROUP_D, 0, parse_Rotate },	/* SRA */
	{ 0x38, MODE_GROUP_D, 0, parse_Rotate },	/* SRL */
	{ 0x10, 0, 0, parse_Stop },	/* STOP */
	{ 0x10, MODE_REG_A, MODE_GROUP_D | MODE_IMM, parse_Alu },	/* SUB */
	{ 0x30, MODE_GROUP_D, 0, parse_Rotate },	/* SWAP */
	{ 0x28, MODE_REG_A, MODE_GROUP_D | MODE_IMM, parse_Alu },	/* XOR */
};


static bool_t parse_AddrMode(SAddrMode* pAddrMode)
{
	if(g_CurrentToken.ID.TargetToken >= T_MODE_B
	&& g_CurrentToken.ID.TargetToken <= T_MODE_A)
	{
		if(g_CurrentToken.ID.TargetToken == T_MODE_A)
			pAddrMode->nMode |= MODE_REG_A;

		if(g_CurrentToken.ID.TargetToken == T_MODE_HL_IND)
			pAddrMode->nMode |= MODE_REG_HL_IND;

		if(g_CurrentToken.ID.TargetToken == T_MODE_C)
		{
			pAddrMode->nMode |= MODE_F;
			pAddrMode->eModeF = CC_C;
		}

		pAddrMode->nMode |= MODE_GROUP_D;
		pAddrMode->eRegD = g_CurrentToken.ID.TargetToken - T_MODE_B + REG_B;

		parse_GetToken();
		return true;
	}

	if(g_CurrentToken.ID.TargetToken == T_MODE_AF)
	{
		pAddrMode->nMode |= MODE_GROUP_TT;
		pAddrMode->eRegTT = REG_AF;

		parse_GetToken();
		return true;
	}

	if(g_CurrentToken.ID.TargetToken >= T_MODE_BC
	&& g_CurrentToken.ID.TargetToken <= T_MODE_SP)
	{
		if(g_CurrentToken.ID.TargetToken <= T_MODE_HL)
		{
			pAddrMode->nMode |= MODE_GROUP_TT;
			pAddrMode->eRegTT = g_CurrentToken.ID.TargetToken - T_MODE_BC + REG_BC;
		}

		if(g_CurrentToken.ID.TargetToken == T_MODE_HL)
			pAddrMode->nMode |= MODE_REG_HL;

		if(g_CurrentToken.ID.TargetToken == T_MODE_SP)
			pAddrMode->nMode |= MODE_REG_SP;

		pAddrMode->nMode |= MODE_GROUP_SS;
		pAddrMode->eRegSS = g_CurrentToken.ID.TargetToken - T_MODE_BC + REG_BC;

		parse_GetToken();
		return true;
	}

	if(g_CurrentToken.ID.TargetToken >= T_MODE_BC_IND
	&& g_CurrentToken.ID.TargetToken <= T_MODE_HL_INDDEC)
	{
		pAddrMode->nMode |= MODE_GROUP_RR;
		pAddrMode->eRegRR = g_CurrentToken.ID.TargetToken - T_MODE_BC_IND + REG_BC_IND;

		parse_GetToken();
		return true;
	}

	if(g_CurrentToken.ID.TargetToken >= T_CC_NZ
	&& g_CurrentToken.ID.TargetToken <= T_CC_NC)
	{
		pAddrMode->nMode |= MODE_F;
		pAddrMode->eModeF = g_CurrentToken.ID.TargetToken - T_CC_NZ + CC_NZ;

		parse_GetToken();
		return true;
	}

	if(g_CurrentToken.ID.TargetToken == T_MODE_SP_IND)
	{
		pAddrMode->nMode |= MODE_REG_SP_IND;
		parse_GetToken();
		return true;
	}

	if(g_CurrentToken.ID.TargetToken == T_MODE_C_IND)
	{
		pAddrMode->nMode |= MODE_REG_C_IND;
		parse_GetToken();
		return true;
	}

	if(g_CurrentToken.ID.Token == '[')
	{
		SLexBookmark bm;
		lex_Bookmark(&bm);

		parse_GetToken();
		if(g_CurrentToken.ID.TargetToken == T_MODE_SP)
		{
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
						if(parse_ExpectChar(']'))
						{
							pAddrMode->nMode |= MODE_REG_SP_IND_DISP;
							pAddrMode->pExpr = pExpr;
							return true;
						}
					}
				}
				expr_Free(pExpr);
			}
		}
		else
		{
			SExpression* pExpr = parse_Expression();
			if(pExpr != NULL)
			{
				if(parse_ExpectChar(']'))
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
				&& (pOpcode->nAddrMode1 & REG_A)
				&& (pOpcode->nAddrMode2 & addrMode1.nMode))
				{
					addrMode2 = addrMode1;
					addrMode1.nMode = MODE_REG_A | MODE_GROUP_D;
					addrMode1.eRegD = REG_A;
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
				return pOpcode->pParser(pOpcode, &addrMode1, &addrMode2);
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
