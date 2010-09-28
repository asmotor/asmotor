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

#include "../common/xasm.h"

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
	BOOL (*pParser)(struct _Opcode* pCode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2);
} SOpcode;

extern SOpcode g_aOpcodes[T_Z80_XOR - T_Z80_ADC + 1];

#define MODE_NONE		0x000001
#define MODE_REG_A		0x000002
#define MODE_REG_C_IND	0x004000
#define MODE_REG_HL		0x000004
#define MODE_REG_HL_IND	0x000008
#define MODE_REG_SP		0x000010
#define MODE_REG_SP_IND	0x000020
#define MODE_REG_SP_IND_DISP	\
						0x040000
#define MODE_GROUP_D	0x000040
#define MODE_GROUP_SS	0x000080
#define MODE_GROUP_RR	0x000100
#define MODE_GROUP_TT	0x080000
#define MODE_IMM		0x020000
#define MODE_IMM_8SU	0x000200
#define MODE_IMM_3U		0x000400
#define MODE_IMM_16U	0x000800
#define MODE_IMM_IND	0x008000
#define MODE_IMM_IND_HI	0x010000
#define MODE_IMM_CONST	0x100000
#define MODE_PCREL		0x001000
#define MODE_F			0x002000
#define MODE_EXPR		(MODE_IMM_8SU | MODE_IMM_3U | MODE_IMM_16U | MODE_PCREL | MODE_IMM | MODE_IMM_CONST)

static SExpression* parse_ExpressionPCRel(void)
{
	SExpression* pExpr = parse_Expression();
	if(pExpr == NULL)
		prj_Error(ERROR_INVALID_EXPRESSION);

	pExpr = parse_CreatePCRelExpr(pExpr, -1);
	pExpr = parse_CheckRange(pExpr, -128, 127);
	if(pExpr == NULL)
		prj_Error(ERROR_EXPRESSION_N_BIT, 8);

	return pExpr;
}

static SExpression* parse_CreateExpressionNBit(SExpression* pExpr, int nLowLimit, int nHighLimit, int nBits)
{
	pExpr = parse_CheckRange(pExpr, nLowLimit, nHighLimit);
	if(pExpr == NULL)
		prj_Error(ERROR_EXPRESSION_N_BIT, nBits);

	return pExpr;
}

static SExpression* parse_CreateExpression16SU(SExpression* pExpr)
{
	return parse_CreateExpressionNBit(pExpr, -32768, 65535, 16);
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

static SExpression* parse_CreateExpression8U(SExpression* pExpr)
{
	return parse_CreateExpressionNBit(pExpr, 0, 255, 8);
}

static SExpression* parse_CreateExpression3U(SExpression* pExpr)
{
	return parse_CreateExpressionNBit(pExpr, 0, 7, 3);
}

static SExpression* parse_CreateExpressionPCRel(SExpression* pExpr)
{
	pExpr = parse_CreatePCRelExpr(pExpr, -1);
	return parse_CreateExpression8S(pExpr);
}

static SExpression* parse_CreateExpressionImmHi(SExpression* pExpr)
{
	return parse_CreateANDExpr(parse_CreateExpressionNBit(pExpr, 0xFF00, 0xFFFF, 16), parse_CreateConstExpr(0xFF));
}

static BOOL parse_Alu(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode2->nMode & MODE_GROUP_D)
	{
		sect_OutputAbsByte((UBYTE)(0x80 | pOpcode->nOpcode | pAddrMode2->eRegD));
		return TRUE;
	}

	sect_OutputAbsByte(0xC6 | pOpcode->nOpcode);
	sect_OutputExprByte(pAddrMode2->pExpr);
	return TRUE;
}

static BOOL parse_Add(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->nMode & MODE_REG_A)
		return parse_Alu(pOpcode, pAddrMode1, pAddrMode2);
	else if((pAddrMode1->nMode & MODE_REG_HL) && (pAddrMode2->nMode & MODE_GROUP_SS))
		sect_OutputAbsByte((UBYTE)(0x09 | (pAddrMode2->eRegSS << 4)));
	else if((pAddrMode1->nMode & MODE_REG_SP) && (pAddrMode2->nMode & MODE_IMM_8SU))
	{
		sect_OutputAbsByte(0xE8);
		sect_OutputExprByte(pAddrMode2->pExpr);
	}

	return TRUE;
}

static BOOL parse_Bit(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	sect_OutputAbsByte(0xCB);
	sect_OutputExprByte(
		parse_CreateORExpr(
			parse_CreateConstExpr(pOpcode->nOpcode | pAddrMode2->eRegD),
			parse_CreateSHLExpr(
				pAddrMode1->pExpr,
				parse_CreateConstExpr(3))
			)
		);

	return TRUE;
}

static BOOL parse_Call(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if((pAddrMode1->nMode & MODE_IMM_16U) && pAddrMode2->nMode == 0)
	{
		sect_OutputAbsByte(pOpcode->nOpcode);
		sect_OutputExprWord(pAddrMode1->pExpr);
	}
	else if((pAddrMode1->nMode & MODE_F) && (pAddrMode2->nMode & MODE_IMM_16U))
	{
		sect_OutputAbsByte((UBYTE)((pOpcode->nOpcode & ~0x19) | (pAddrMode1->eModeF << 3)));
		sect_OutputExprWord(pAddrMode2->pExpr);
	}
	else
		prj_Error(ERROR_OPERAND);

	return TRUE;
}

static BOOL parse_Jp(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if((pAddrMode1->nMode & MODE_REG_HL_IND) && pAddrMode2->nMode == 0)
	{
		sect_OutputAbsByte(0xE9);
		return TRUE;
	}

	return parse_Call(pOpcode, pAddrMode1, pAddrMode2);
}

static BOOL parse_Implied(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	sect_OutputAbsByte(pOpcode->nOpcode);
	return TRUE;
}

static BOOL parse_Dec(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->nMode & MODE_GROUP_SS)
		sect_OutputAbsByte((UBYTE)(0x03 | (pOpcode->nOpcode << 3) | (pAddrMode1->eRegSS << 4)));
	else
		sect_OutputAbsByte((UBYTE)(0x04 | pOpcode->nOpcode | (pAddrMode1->eRegD << 3)));
	return TRUE;
}

static BOOL parse_Ex(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if((pAddrMode1->nMode == 0 && pAddrMode2->nMode == 0)
	|| ((pAddrMode1->nMode & MODE_REG_HL) && (pAddrMode2->nMode & MODE_REG_SP_IND))
	|| ((pAddrMode1->nMode & MODE_REG_SP_IND) && (pAddrMode2->nMode & MODE_REG_HL)))
	{
		sect_OutputAbsByte(pOpcode->nOpcode);
	}
	else
		prj_Error(ERROR_OPERAND);

	return TRUE;
}
static BOOL parse_Jr(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if((pAddrMode1->nMode & MODE_PCREL) && pAddrMode2->nMode == 0)
	{
		sect_OutputAbsByte(0x18);
		sect_OutputExprByte(pAddrMode1->pExpr);
	}
	else if((pAddrMode1->nMode & MODE_F) && (pAddrMode2->nMode & MODE_PCREL))
	{
		sect_OutputAbsByte((UBYTE)(0x20 | (pAddrMode1->eModeF << 3)));
		sect_OutputExprByte(pAddrMode2->pExpr);
	}
	else
		prj_Error(ERROR_OPERAND);

	return TRUE;
}

static BOOL parse_Ld(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if((pAddrMode1->nMode & MODE_GROUP_D) && (pAddrMode2->nMode & MODE_GROUP_D))
	{
		if(pAddrMode1->eRegD == REG_HL_IND && pAddrMode2->eRegD == REG_HL_IND)
			prj_Error(ERROR_OPERAND);
		else
			sect_OutputAbsByte((UBYTE)(0x40 | (pAddrMode1->eRegD << 3) | pAddrMode2->eRegD));
	}
	else if((pAddrMode1->nMode & MODE_REG_A) && (pAddrMode2->nMode & MODE_GROUP_RR))
	{
		sect_OutputAbsByte((UBYTE)(0x0A | (pAddrMode2->eRegRR << 4)));
	}
	else if((pAddrMode1->nMode & MODE_REG_A) && (pAddrMode2->nMode & MODE_REG_C_IND))
	{
		sect_OutputAbsByte(0xF2);
	}
	else if((pAddrMode1->nMode & MODE_REG_A) && (pAddrMode2->nMode & MODE_IMM_IND_HI))
	{
		sect_OutputAbsByte(0xF0);
		sect_OutputExprByte(pAddrMode2->pExpr);
	}
	else if((pAddrMode1->nMode & MODE_REG_A) && (pAddrMode2->nMode & MODE_IMM_IND))
	{
		sect_OutputAbsByte(0xFA);
		sect_OutputExprWord(pAddrMode2->pExpr);
	}
	else if((pAddrMode1->nMode & MODE_GROUP_D) && (pAddrMode2->nMode & MODE_IMM))
	{
		sect_OutputAbsByte((UBYTE)(0x06 | (pAddrMode1->eRegD << 3)));
		sect_OutputExprByte(parse_CreateExpression8SU(pAddrMode2->pExpr));
	}
	else if((pAddrMode1->nMode & MODE_GROUP_RR) && (pAddrMode2->nMode & MODE_REG_A))
	{
		sect_OutputAbsByte((UBYTE)(0x02 | (pAddrMode1->eRegRR << 4)));
	}
	else if((pAddrMode1->nMode & MODE_REG_SP) && (pAddrMode2->nMode & MODE_REG_HL))
	{
		sect_OutputAbsByte(0xF9);
	}
	else if((pAddrMode1->nMode & MODE_REG_HL) && (pAddrMode2->nMode & MODE_REG_SP_IND_DISP))
	{
		sect_OutputAbsByte(0xF8);
		sect_OutputExprByte(pAddrMode2->pExpr);
	}
	else if((pAddrMode1->nMode & MODE_GROUP_SS) && (pAddrMode2->nMode & MODE_IMM))
	{
		sect_OutputAbsByte((UBYTE)(0x01 | (pAddrMode1->eRegSS << 4)));
		sect_OutputExprWord(parse_CreateExpression16U(pAddrMode2->pExpr));
	}
	else if((pAddrMode1->nMode & MODE_REG_C_IND) && (pAddrMode2->nMode & MODE_REG_A))
	{
		sect_OutputAbsByte(0xE2);
	}
	else if((pAddrMode1->nMode & MODE_IMM_IND) && (pAddrMode2->nMode & MODE_REG_SP))
	{
		sect_OutputAbsByte(0x08);
		sect_OutputExprWord(pAddrMode1->pExpr);
	}
	else if((pAddrMode1->nMode & MODE_IMM_IND_HI) && (pAddrMode2->nMode & MODE_REG_A))
	{
		sect_OutputAbsByte(0xE0);
		sect_OutputExprByte(pAddrMode1->pExpr);
	}
	else if((pAddrMode1->nMode & MODE_IMM_IND) && (pAddrMode2->nMode & MODE_REG_A))
	{
		sect_OutputAbsByte(0xEA);
		sect_OutputExprWord(pAddrMode1->pExpr);
	}
	else
		prj_Error(ERROR_OPERAND);

	return TRUE;
}

static BOOL parse_Ldd(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if((pAddrMode1->nMode & MODE_REG_A) && (pAddrMode2->nMode & MODE_REG_HL_IND))
		sect_OutputAbsByte((UBYTE)(pOpcode->nOpcode | 0x08));
	else if((pAddrMode1->nMode & MODE_REG_HL_IND) && (pAddrMode2->nMode & MODE_REG_A))
		sect_OutputAbsByte((UBYTE)pOpcode->nOpcode);
	else
		prj_Error(ERROR_OPERAND);

	return TRUE;
}


static BOOL parse_Ldh(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if((pAddrMode1->nMode & MODE_REG_A) && (pAddrMode2->nMode & MODE_IMM_IND))
	{
		sect_OutputAbsByte((UBYTE)(pOpcode->nOpcode | 0x10));
		sect_OutputExprByte(parse_CreateExpressionImmHi(pAddrMode2->pExpr));
	}
	else if((pAddrMode1->nMode & MODE_IMM_IND) && (pAddrMode2->nMode & MODE_REG_A))
	{
		sect_OutputAbsByte(pOpcode->nOpcode);
		sect_OutputExprByte(parse_CreateExpressionImmHi(pAddrMode1->pExpr));
	}
	else
		prj_Error(ERROR_OPERAND);

	return TRUE;
}


static BOOL parse_Pop(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	sect_OutputAbsByte((UBYTE)(pOpcode->nOpcode | (pAddrMode1->eRegTT << 4)));
	return TRUE;
}

static BOOL parse_Rotate(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	sect_OutputAbsByte(0xCB);
	sect_OutputAbsByte((UBYTE)(pOpcode->nOpcode | pAddrMode1->eRegD));
	return TRUE;
}

static BOOL parse_Rr(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->eRegD == REG_A)
		prj_Warn(MERROR_SUGGEST_OPCODE, "RRA");
	return parse_Rotate(pOpcode, pAddrMode1, pAddrMode2);
}

static BOOL parse_Rl(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->eRegD == REG_A)
		prj_Warn(MERROR_SUGGEST_OPCODE, "RLA");
	return parse_Rotate(pOpcode, pAddrMode1, pAddrMode2);
}

static BOOL parse_Rrc(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->eRegD == REG_A)
		prj_Warn(MERROR_SUGGEST_OPCODE, "RRCA");
	return parse_Rotate(pOpcode, pAddrMode1, pAddrMode2);
}

static BOOL parse_Rlc(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->eRegD == REG_A)
		prj_Warn(MERROR_SUGGEST_OPCODE, "RLCA");
	return parse_Rotate(pOpcode, pAddrMode1, pAddrMode2);
}

static BOOL parse_Ret(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	if(pAddrMode1->nMode & MODE_F)
		sect_OutputAbsByte((UBYTE)(0xC0 | (pAddrMode1->eModeF << 3)));
	else
		sect_OutputAbsByte(0xC9);

	return TRUE;
}

static BOOL parse_Rst(SOpcode* pOpcode, SAddrMode* pAddrMode1, SAddrMode* pAddrMode2)
{
	SLONG val = pAddrMode1->pExpr->Value.Value;
	if(val == (val & 0x38))
	{
		sect_OutputAbsByte((UBYTE)(pOpcode->nOpcode | val));
		return TRUE;
	}

	prj_Error(ERROR_OPERAND_RANGE);
	return TRUE;
}

SOpcode g_aOpcodes[T_Z80_XOR - T_Z80_ADC + 1] =
{
	{ 0x08, MODE_REG_A, MODE_GROUP_D | MODE_IMM_8SU, parse_Alu },	/* ADC */
	{ 0x00, MODE_REG_A | MODE_REG_HL | MODE_REG_SP, MODE_GROUP_D | MODE_IMM_8SU | MODE_GROUP_SS, parse_Add },	/* ADD */
	{ 0x20, MODE_REG_A, MODE_GROUP_D | MODE_IMM_8SU, parse_Alu },	/* AND */
	{ 0x40, MODE_IMM_3U, MODE_GROUP_D, parse_Bit },				/* BIT */
	{ 0xCD, MODE_F | MODE_IMM_16U, MODE_IMM_16U | MODE_NONE, parse_Call },	/* CALL */
	{ 0x3F, 0, 0, parse_Implied },							/* CCF */
	{ 0x2F, 0, 0, parse_Implied },							/* CPL */
	{ 0x38, MODE_REG_A, MODE_GROUP_D | MODE_IMM_8SU, parse_Alu },	/* CP */
	{ 0x27, 0, 0, parse_Implied },							/* DAA */
	{ 0x01, MODE_GROUP_SS | MODE_GROUP_D, 0, parse_Dec },			/* DEC */
	{ 0xF3, 0, 0, parse_Implied },							/* DI */
	{ 0xFB, 0, 0, parse_Implied },							/* EI */
	{ 0xE3, MODE_REG_HL | MODE_REG_SP_IND | MODE_NONE, MODE_REG_HL | MODE_REG_SP_IND | MODE_NONE, parse_Ex },	/* EX */
	{ 0x76, 0, 0, parse_Implied },							/* HALT */
	{ 0x00, MODE_GROUP_SS | MODE_GROUP_D, 0, parse_Dec },			/* INC */
	{ 0xC3, MODE_F | MODE_IMM_16U | MODE_REG_HL_IND, MODE_IMM_16U | MODE_NONE, parse_Jp },	/* JP */
	{ 0x00, MODE_F | MODE_PCREL, MODE_PCREL | MODE_NONE, parse_Jr },	/* JR */
	{ 0x00,
		MODE_REG_A | MODE_REG_C_IND | MODE_REG_HL | MODE_REG_SP | MODE_GROUP_D | MODE_GROUP_RR | MODE_GROUP_SS | MODE_IMM_IND,
		MODE_REG_A | MODE_REG_C_IND | MODE_REG_HL | MODE_REG_SP | MODE_REG_SP_IND_DISP | MODE_GROUP_D | MODE_GROUP_RR | MODE_IMM_IND | MODE_IMM_IND_HI | MODE_IMM, parse_Ld },	/* LD */
	{ 0x32, MODE_REG_A | MODE_REG_HL_IND, MODE_REG_A | MODE_REG_HL_IND, parse_Ldd },	/* LDD */
	{ 0x22, MODE_REG_A | MODE_REG_HL_IND, MODE_REG_A | MODE_REG_HL_IND, parse_Ldd },	/* LDI */
	{ 0xE0, MODE_IMM_IND | MODE_REG_A, MODE_IMM_IND | MODE_REG_A, parse_Ldh },	/* LDH */
	{ 0x00, 0, 0, parse_Implied },	/* LDH */
	{ 0x30, MODE_REG_A, MODE_GROUP_D | MODE_IMM_8SU, parse_Alu },	/* OR */
	{ 0xC1, MODE_GROUP_TT, 0, parse_Pop },	/* POP */
	{ 0xC5, MODE_GROUP_TT, 0, parse_Pop },	/* PUSH */
	{ 0x80, MODE_IMM_3U, MODE_GROUP_D, parse_Bit },				/* RES */
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
	{ 0xC7, MODE_IMM_CONST, 0, parse_Rst },	/* RST */
	{ 0x18, MODE_REG_A, MODE_GROUP_D | MODE_IMM_8SU, parse_Alu },	/* SBC */
	{ 0x37, 0, 0, parse_Implied },	/* SCF */
	{ 0xC0, MODE_IMM_3U, MODE_GROUP_D, parse_Bit },				/* SET */
	{ 0x20, MODE_GROUP_D, 0, parse_Rotate },	/* SLA */
	{ 0x28, MODE_GROUP_D, 0, parse_Rotate },	/* SRA */
	{ 0x38, MODE_GROUP_D, 0, parse_Rotate },	/* SRL */
	{ 0x10, 0, 0, parse_Implied },	/* STOP */
	{ 0x10, MODE_REG_A, MODE_GROUP_D | MODE_IMM_8SU, parse_Alu },	/* SUB */
	{ 0x30, MODE_GROUP_D, 0, parse_Rotate },	/* SWAP */
	{ 0x28, MODE_REG_A, MODE_GROUP_D | MODE_IMM_8SU, parse_Alu },	/* XOR */
};


static BOOL parse_AddrMode(SAddrMode* pAddrMode, int nAllowedModes)
{
	if(g_CurrentToken.ID.TargetToken >= T_MODE_B
	&& g_CurrentToken.ID.TargetToken <= T_MODE_A)
	{
		if(g_CurrentToken.ID.TargetToken == T_MODE_A && (nAllowedModes & MODE_REG_A))
			pAddrMode->nMode |= MODE_REG_A;

		if(g_CurrentToken.ID.TargetToken == T_MODE_HL_IND && (nAllowedModes & MODE_REG_HL_IND))
			pAddrMode->nMode |= MODE_REG_HL_IND;

		if(nAllowedModes & MODE_GROUP_D)
		{
			pAddrMode->nMode |= MODE_GROUP_D;
			pAddrMode->eRegD = g_CurrentToken.ID.TargetToken - T_MODE_B + REG_B;
		}

		if(g_CurrentToken.ID.TargetToken == T_MODE_C
		&& (nAllowedModes & MODE_F))
		{
			pAddrMode->nMode |= MODE_F;
			pAddrMode->eModeF = CC_C;
		}

		if(pAddrMode->nMode)
		{
			parse_GetToken();
			return TRUE;
		}
		return FALSE;
	}

	if(g_CurrentToken.ID.TargetToken == T_MODE_AF
	&& (nAllowedModes & MODE_GROUP_TT))
	{
		pAddrMode->nMode |= MODE_GROUP_TT;
		pAddrMode->eRegTT = REG_AF;

		parse_GetToken();
		return TRUE;
	}

	if(g_CurrentToken.ID.TargetToken >= T_MODE_BC
	&& g_CurrentToken.ID.TargetToken <= T_MODE_SP)
	{
		if(g_CurrentToken.ID.TargetToken <= T_MODE_HL
		&& (nAllowedModes & MODE_GROUP_TT))
		{
			pAddrMode->nMode |= MODE_GROUP_TT;
			pAddrMode->eRegTT = g_CurrentToken.ID.TargetToken - T_MODE_BC + REG_BC;
		}

		if(g_CurrentToken.ID.TargetToken == T_MODE_HL && (nAllowedModes & MODE_REG_HL))
			pAddrMode->nMode |= MODE_REG_HL;

		if(g_CurrentToken.ID.TargetToken == T_MODE_SP && (nAllowedModes & MODE_REG_SP))
			pAddrMode->nMode |= MODE_REG_SP;

		if(nAllowedModes & MODE_GROUP_SS)
		{
			pAddrMode->nMode |= MODE_GROUP_SS;
			pAddrMode->eRegSS = g_CurrentToken.ID.TargetToken - T_MODE_BC + REG_BC;
		}

		if(pAddrMode->nMode)
		{
			parse_GetToken();
			return TRUE;
		}
		return FALSE;
	}

	if(g_CurrentToken.ID.TargetToken >= T_MODE_BC_IND
	&& g_CurrentToken.ID.TargetToken <= T_MODE_HL_INDDEC
	&& (nAllowedModes & MODE_GROUP_RR))
	{
		pAddrMode->nMode |= MODE_GROUP_RR;
		pAddrMode->eRegRR = g_CurrentToken.ID.TargetToken - T_MODE_BC_IND + REG_BC_IND;
		parse_GetToken();

		return TRUE;
	}

	if(g_CurrentToken.ID.TargetToken >= T_CC_NZ
	&& g_CurrentToken.ID.TargetToken <= T_CC_NC
	&& (nAllowedModes & MODE_F))
	{
		pAddrMode->nMode |= MODE_F;
		pAddrMode->eModeF = g_CurrentToken.ID.TargetToken - T_CC_NZ + CC_NZ;
		parse_GetToken();
		return TRUE;
	}

	if(g_CurrentToken.ID.TargetToken == T_MODE_SP_IND
	&& (nAllowedModes & MODE_REG_SP_IND))
	{
		pAddrMode->nMode |= MODE_REG_SP_IND;
		parse_GetToken();
		return TRUE;
	}

	if(g_CurrentToken.ID.TargetToken == T_MODE_C_IND
	&& (nAllowedModes & MODE_REG_C_IND))
	{
		pAddrMode->nMode |= MODE_REG_C_IND;
		parse_GetToken();
		return TRUE;
	}

	if((nAllowedModes & MODE_REG_SP_IND_DISP)
	&&	g_CurrentToken.ID.Token == '[')
	{
		SLexBookmark bm;
		lex_Bookmark(&bm);

		parse_GetToken();
		if(g_CurrentToken.ID.TargetToken == T_MODE_SP)
		{
			parse_GetToken();
			if(g_CurrentToken.ID.Token == T_OP_ADD)
			{
				SExpression* pExpr;

				parse_GetToken();
				pExpr = parse_Expression();
				if(pExpr != NULL)
				{
					pExpr = parse_CreateExpression8U(pExpr);
					if(pExpr != NULL)
					{
						if(parse_ExpectChar(']'))
						{
							pAddrMode->nMode |= MODE_REG_SP_IND_DISP;
							pAddrMode->pExpr = pExpr;
							return TRUE;
						}
					}
				}
				parse_FreeExpression(pExpr);
			}
		}
		lex_Goto(&bm);
	}

	if((nAllowedModes & (MODE_IMM_IND | MODE_IMM_IND_HI))
	&&	g_CurrentToken.ID.Token == '[')
	{
		SExpression* pExpr;
		SLexBookmark bm;

		lex_Bookmark(&bm);

		parse_GetToken();
		pExpr = parse_Expression();
		if(pExpr != NULL)
		{
			if(parse_ExpectChar(']'))
			{
				if(nAllowedModes & MODE_IMM_IND_HI)
				{
					if(((pExpr->Flags & EXPRF_isCONSTANT) && pExpr->Value.Value >= 0xFF00 && pExpr->Value.Value <= 0xFFFF)
					|| ((pExpr->Flags & EXPRF_isRELOC) && (nAllowedModes & MODE_IMM_IND) == 0))
					{
						pAddrMode->nMode |= MODE_IMM_IND_HI;
						pAddrMode->pExpr = parse_CreateExpressionImmHi(pExpr);
						return TRUE;
					}
				}

				if(nAllowedModes & MODE_IMM_IND)
				{
					pAddrMode->nMode |= MODE_IMM_IND;
					pAddrMode->pExpr = parse_CreateExpression16U(pExpr);
					return TRUE;
				}
			}
			parse_FreeExpression(pExpr);
		}

		lex_Goto(&bm);
	}

	if(nAllowedModes & MODE_EXPR)
	{
		SExpression* pExpr;
		SLexBookmark bm;

		lex_Bookmark(&bm);
		pExpr = parse_Expression();
		if(pExpr != NULL)
		{
			if((nAllowedModes & MODE_IMM_CONST)
			&& (pExpr->Flags & EXPRF_isCONSTANT))
			{
				pAddrMode->nMode |= MODE_IMM_CONST;
				pAddrMode->pExpr = pExpr;
			}
			else if(nAllowedModes & MODE_IMM)
			{
				pAddrMode->nMode |= MODE_IMM;
				pAddrMode->pExpr = pExpr;
			}
			else if(nAllowedModes & MODE_IMM_16U)
			{
				pAddrMode->nMode |= MODE_IMM_16U;
				pAddrMode->pExpr = parse_CreateExpression16U(pExpr);
			}
			else if(nAllowedModes & MODE_IMM_8SU)
			{
				pAddrMode->nMode |= MODE_IMM_8SU;
				pAddrMode->pExpr = parse_CreateExpression8SU(pExpr);
			}
			else if(nAllowedModes & MODE_IMM_3U)
			{
				pAddrMode->nMode |= MODE_IMM_3U;
				pAddrMode->pExpr = parse_CreateExpression3U(pExpr);
			}
			else if(nAllowedModes & MODE_PCREL)
			{
				pAddrMode->nMode |= MODE_PCREL;
				pAddrMode->pExpr = parse_CreateExpressionPCRel(pExpr);
			}

			return pAddrMode->nMode ? TRUE : FALSE;
		}
		
		lex_Goto(&bm);
	}

	return FALSE;
}


BOOL parse_TargetSpecific(void)
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
			BOOL bExpectComma = TRUE;

			if(!parse_AddrMode(&addrMode1, pOpcode->nAddrMode1))
			{
				if((pOpcode->nAddrMode1 & (MODE_NONE | MODE_REG_A)) == MODE_REG_A)
				{
					addrMode1.nMode = MODE_REG_A;
					bExpectComma = FALSE;
				}
				else if(pOpcode->nAddrMode1 == 0 || (pOpcode->nAddrMode1 & MODE_NONE))
					addrMode1.nMode = 0;
				else
				{
					prj_Error(ERROR_FIRST_OPERAND);
					return TRUE;
				}
			}
			
			if(pOpcode->nAddrMode2 != 0)
			{
				BOOL done = FALSE;

				if(bExpectComma)
				{
					if(g_CurrentToken.ID.Token != ',' && (pOpcode->nAddrMode2 & MODE_NONE))
					{
						addrMode2.nMode = 0;
						done = TRUE;
					}
					else if(!parse_ExpectComma())
						return TRUE;
				}

				if(!done && !parse_AddrMode(&addrMode2, pOpcode->nAddrMode2))
				{
					if(pOpcode->nAddrMode2 & MODE_NONE)
						addrMode2.nMode = 0;
					else
					{
						prj_Error(ERROR_SECOND_OPERAND);
						return TRUE;
					}
				}
			}
		}

		return pOpcode->pParser(pOpcode, &addrMode1, &addrMode2);
	}

	return FALSE;
}


SExpression* parse_TargetFunction(void)
{
	return NULL;
}
