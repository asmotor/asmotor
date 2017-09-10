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

#if !defined(INTEGER_INSTRUCTIONS_65XX_)
#define INTEGER_INSTRUCTIONS_65XX_

#define MODE_NONE	0x001
#define MODE_IMM	0x002
#define MODE_ZP		0x004
#define MODE_ZP_X	0x008
#define MODE_ZP_Y	0x010
#define MODE_ABS	0x020
#define MODE_ABS_X	0x040
#define MODE_ABS_Y	0x080
#define MODE_IND_X	0x100
#define MODE_IND_Y	0x200
#define MODE_A		0x400
#define MODE_IND	0x800

typedef struct
{
	int	nMode;
	SExpression* pExpr;
} SAddressingMode;

typedef struct Parser
{
	uint8_t	nBaseOpcode;
	int		nAddressingModes;
	bool_t	(*fpParser)(int nToken, uint8_t nBaseOpcode, SAddressingMode* pAddrMode);
} SParser;

static bool_t parse_Standard_All(int nToken, uint8_t nBaseOpcode, SAddressingMode* pAddrMode)
{
	switch(pAddrMode->nMode)
	{
		case MODE_IND_X:
			sect_OutputConst8(nBaseOpcode | (0 << 2));
			sect_OutputExpr8(pAddrMode->pExpr);
			return true;
		case MODE_IMM:
			sect_OutputConst8(nBaseOpcode | (2 << 2));
			sect_OutputExpr8(pAddrMode->pExpr);
			return true;
		case MODE_IND_Y:
			sect_OutputConst8(nBaseOpcode | (4 << 2));
			sect_OutputExpr8(pAddrMode->pExpr);
			return true;
		case MODE_ABS_Y:
			sect_OutputConst8(nBaseOpcode | (6 << 2));
			sect_OutputExpr16(pAddrMode->pExpr);
			return true;
		case MODE_ZP:
			sect_OutputConst8(nBaseOpcode | (1 << 2));
			sect_OutputExpr8(pAddrMode->pExpr);
			return true;
		case MODE_ABS:
			sect_OutputConst8(nBaseOpcode | (3 << 2));
			sect_OutputExpr16(pAddrMode->pExpr);
			return true;
		case MODE_ZP_X:
		case MODE_ZP_Y:
			sect_OutputConst8(nBaseOpcode | (5 << 2));
			sect_OutputExpr8(pAddrMode->pExpr);
			return true;
		case MODE_ABS_X:
			sect_OutputConst8(nBaseOpcode | (7 << 2));
			sect_OutputExpr16(pAddrMode->pExpr);
			return true;
	}

	prj_Fail(MERROR_ILLEGAL_ADDRMODE);
	return true;
}

static bool_t parse_Standard_AbsY7(int nToken, uint8_t nBaseOpcode, SAddressingMode* pAddrMode)
{
	switch(pAddrMode->nMode)
	{
		case MODE_IND_X:
			sect_OutputConst8(nBaseOpcode | (0 << 2));
			sect_OutputExpr8(pAddrMode->pExpr);
			return true;
		case MODE_IMM:
			sect_OutputConst8(nBaseOpcode | (2 << 2));
			sect_OutputExpr8(pAddrMode->pExpr);
			return true;
		case MODE_IND_Y:
			sect_OutputConst8(nBaseOpcode | (4 << 2));
			sect_OutputExpr8(pAddrMode->pExpr);
			return true;
		case MODE_ABS_Y:
			sect_OutputConst8(nBaseOpcode | (7 << 2));
			sect_OutputExpr16(pAddrMode->pExpr);
			return true;
		case MODE_ZP:
			sect_OutputConst8(nBaseOpcode | (1 << 2));
			sect_OutputExpr8(pAddrMode->pExpr);
			return true;
		case MODE_ABS:
			sect_OutputConst8(nBaseOpcode | (3 << 2));
			sect_OutputExpr16(pAddrMode->pExpr);
			return true;
		case MODE_ZP_X:
		case MODE_ZP_Y:
			sect_OutputConst8(nBaseOpcode | (5 << 2));
			sect_OutputExpr8(pAddrMode->pExpr);
			return true;
	}

	prj_Fail(MERROR_ILLEGAL_ADDRMODE);
	return true;
}

static bool_t parse_Standard_Imm0(int nToken, uint8_t nBaseOpcode, SAddressingMode* pAddrMode)
{
	switch(pAddrMode->nMode)
	{
		case MODE_IMM:
			sect_OutputConst8(nBaseOpcode | (0 << 2));
			sect_OutputExpr8(pAddrMode->pExpr);
			return true;
		case MODE_ZP:
			sect_OutputConst8(nBaseOpcode | (1 << 2));
			sect_OutputExpr8(pAddrMode->pExpr);
			return true;
		case MODE_ABS:
			sect_OutputConst8(nBaseOpcode | (3 << 2));
			sect_OutputExpr16(pAddrMode->pExpr);
			return true;
		case MODE_ZP_X:
		case MODE_ZP_Y:
			sect_OutputConst8(nBaseOpcode | (5 << 2));
			sect_OutputExpr8(pAddrMode->pExpr);
			return true;
		case MODE_ABS_X:
		case MODE_ABS_Y:
			sect_OutputConst8(nBaseOpcode | (7 << 2));
			sect_OutputExpr16(pAddrMode->pExpr);
			return true;
	}

	prj_Fail(MERROR_ILLEGAL_ADDRMODE);
	return true;
}

static bool_t parse_Standard_Rotate(int nToken, uint8_t nBaseOpcode, SAddressingMode* pAddrMode)
{
	switch(pAddrMode->nMode)
	{
		case MODE_A:
			sect_OutputConst8(nBaseOpcode | (2 << 2));
			return true;
		case MODE_ZP:
			sect_OutputConst8(nBaseOpcode | (1 << 2));
			sect_OutputExpr8(pAddrMode->pExpr);
			return true;
		case MODE_ABS:
			sect_OutputConst8(nBaseOpcode | (3 << 2));
			sect_OutputExpr16(pAddrMode->pExpr);
			return true;
		case MODE_ZP_X:
			sect_OutputConst8(nBaseOpcode | (5 << 2));
			sect_OutputExpr8(pAddrMode->pExpr);
			return true;
		case MODE_ABS_X:
			sect_OutputConst8(nBaseOpcode | (7 << 2));
			sect_OutputExpr16(pAddrMode->pExpr);
			return true;
	}

	prj_Fail(MERROR_ILLEGAL_ADDRMODE);
	return true;
}


static bool_t parse_Branch(int nToken, uint8_t nBaseOpcode, SAddressingMode* pAddrMode)
{
	SExpression* pExpr;

	sect_OutputConst8(nBaseOpcode);
	pExpr = expr_PcRelative(pAddrMode->pExpr, -1);
	pExpr = expr_CheckRange(pExpr, -128, 127);
	if(pExpr == NULL)
	{
		prj_Error(ERROR_OPERAND_RANGE);
		return true;
	}
	else
	{
		sect_OutputExpr8(pExpr);
	}

	return true;
}


static bool_t parse_Implied(int nToken, uint8_t nBaseOpcode, SAddressingMode* pAddrMode)
{
	sect_OutputConst8(nBaseOpcode);
	return true;
}


static bool_t parse_JMP(int nToken, uint8_t nBaseOpcode, SAddressingMode* pAddrMode)
{
	if(pAddrMode->nMode == MODE_IND)
		nBaseOpcode += 0x20;

	sect_OutputConst8(nBaseOpcode);
	sect_OutputExpr16(pAddrMode->pExpr);
	return true;
}


static bool_t parse_BRK(int nToken, uint8_t nBaseOpcode, SAddressingMode* pAddrMode)
{
	sect_OutputConst8(nBaseOpcode);
	if(pAddrMode->nMode == MODE_IMM)
		sect_OutputExpr8(pAddrMode->pExpr);
	return true;
}


static bool_t parse_DOP(int nToken, uint8_t nBaseOpcode, SAddressingMode* pAddrMode)
{
	switch(pAddrMode->nMode)
	{
		case MODE_IMM:
			sect_OutputConst8(0x80);
			sect_OutputExpr8(pAddrMode->pExpr);
			return true;
		case MODE_ZP:
			sect_OutputConst8(0x04);
			sect_OutputExpr8(pAddrMode->pExpr);
			return true;
		case MODE_ZP_X:
			sect_OutputConst8(0x14);
			sect_OutputExpr8(pAddrMode->pExpr);
			return true;
	}
	return false;
}

static SParser g_Parsers[T_6502U_XAS - T_6502_ADC + 1] = 
{
	{ 0x61, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* ADC */
	{ 0x21, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* AND */
	{ 0x02, MODE_A | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X, parse_Standard_Rotate },	/* ASL */
	{ 0x20, MODE_ZP | MODE_ABS, parse_Standard_All },	/* BIT */
	{ 0x10, MODE_ABS, parse_Branch },	/* BPL */
	{ 0x30, MODE_ABS, parse_Branch },	/* BMI */
	{ 0x50, MODE_ABS, parse_Branch },	/* BVC */
	{ 0x70, MODE_ABS, parse_Branch },	/* BVS */
	{ 0x90, MODE_ABS, parse_Branch },	/* BCC */
	{ 0xB0, MODE_ABS, parse_Branch },	/* BCS */
	{ 0xD0, MODE_ABS, parse_Branch },	/* BNE */
	{ 0xF0, MODE_ABS, parse_Branch },	/* BEQ */
	{ 0x00, MODE_NONE | MODE_IMM, parse_BRK },	/* BRK */
	{ 0xC1, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* CMP */
	{ 0xE0, MODE_IMM | MODE_ZP | MODE_ABS, parse_Standard_Imm0 },	/* CPX */
	{ 0xC0, MODE_IMM | MODE_ZP | MODE_ABS, parse_Standard_Imm0 },	/* CPY */
	{ 0xC2, MODE_ZP | MODE_ABS | MODE_ZP_X | MODE_ABS_X, parse_Standard_All },	/* DEC */
	{ 0x41, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* EOR */
	{ 0x18, 0, parse_Implied },	/* CLC */
	{ 0x38, 0, parse_Implied },	/* SEC */
	{ 0x58, 0, parse_Implied },	/* CLI */
	{ 0x78, 0, parse_Implied },	/* SEI */
	{ 0xB8, 0, parse_Implied },	/* CLV */
	{ 0xD8, 0, parse_Implied },	/* CLD */
	{ 0xF8, 0, parse_Implied },	/* SED */
	{ 0xE2, MODE_ZP | MODE_ABS | MODE_ZP_X | MODE_ABS_X, parse_Standard_All },	/* INC */
	{ 0x4C, MODE_ABS | MODE_IND, parse_JMP },	/* JMP */
	{ 0x20, MODE_ABS, parse_JMP },	/* JSR */
	{ 0xA1, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* LDA */
	{ 0xA2, MODE_IMM | MODE_ZP | MODE_ABS | MODE_ZP_Y | MODE_ABS_Y, parse_Standard_Imm0 },	/* LDX */
	{ 0xA0, MODE_IMM | MODE_ZP | MODE_ABS | MODE_ZP_X | MODE_ABS_X, parse_Standard_Imm0 },	/* LDY */
	{ 0x42, MODE_A | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X, parse_Standard_Rotate },	/* LSR */
	{ 0xEA, 0, parse_Implied },	/* NOP */
	{ 0x01, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* ORA */
	{ 0xAA, 0, parse_Implied },	/* TAX */
	{ 0x8A, 0, parse_Implied },	/* TXA */
	{ 0xCA, 0, parse_Implied },	/* DEX */
	{ 0xE8, 0, parse_Implied },	/* INX */
	{ 0xA8, 0, parse_Implied },	/* TAY */
	{ 0x98, 0, parse_Implied },	/* TYA */
	{ 0x88, 0, parse_Implied },	/* DEY */
	{ 0xC8, 0, parse_Implied },	/* INY */
	{ 0x22, MODE_A | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X, parse_Standard_Rotate },	/* ROL */
	{ 0x62, MODE_A | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X, parse_Standard_Rotate },	/* ROR */
	{ 0x40, 0, parse_Implied },	/* RTI */
	{ 0x60, 0, parse_Implied },	/* RTS */
	{ 0xE1, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* SBC */
	{ 0x81, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* STA */
	{ 0x9A, 0, parse_Implied },	/* TXS */
	{ 0xBA, 0, parse_Implied },	/* TSX */
	{ 0x48, 0, parse_Implied },	/* PHA */
	{ 0x68, 0, parse_Implied },	/* PLA */
	{ 0x08, 0, parse_Implied },	/* PHP */
	{ 0x28, 0, parse_Implied },	/* PLP */
	{ 0x82, MODE_ZP | MODE_ABS | MODE_ZP_Y, parse_Standard_Imm0 },	/* STX */
	{ 0x80, MODE_ZP | MODE_ABS | MODE_ZP_X, parse_Standard_Imm0 },	/* STY */
	
	/* Undocumented instructions */
	
	{ 0x0B, MODE_IMM, parse_Standard_Imm0 },	/* AAC */
	{ 0x83, MODE_ZP | MODE_ZP_Y | MODE_IND_X | MODE_ABS, parse_Standard_All },	/* AAX */
	{ 0x6B, MODE_IMM, parse_Standard_Imm0 },	/* ARR */
	{ 0x4B, MODE_IMM, parse_Standard_Imm0 },	/* ASR */
	{ 0xAB, MODE_IMM, parse_Standard_Imm0 },	/* ATX */
	{ 0x83, MODE_ABS_Y | MODE_IND_Y, parse_Standard_AbsY7 },	/* AXA */
	{ 0xCB, MODE_IMM, parse_Standard_Imm0 },	/* AXS */
	{ 0xC3, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* DCP */
	{ 0x00, MODE_ZP | MODE_ZP_X | MODE_IMM, parse_DOP },	/* DOP */
	{ 0xE3, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* ISC */
	{ 0x02, 0, parse_Implied },	/* KIL */
	{ 0xA3, MODE_ABS_Y, parse_Standard_All },	/* LAR */
	{ 0xA3, MODE_ZP | MODE_ZP_Y | MODE_ABS | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_AbsY7 },	/* LAX */
	{ 0x43, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* RLA */
	{ 0x63, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* RRA */
	{ 0x03, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* SLO */
	{ 0x43, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* SRE */
	{ 0x82, MODE_ABS_Y, parse_Standard_AbsY7 },	/* SXA */
	{ 0x80, MODE_ABS_X, parse_Standard_All },	/* SYA */
	{ 0x00, MODE_ABS | MODE_ABS_X, parse_Standard_All },	/* TOP */
	{ 0x8B, MODE_IMM, parse_Standard_Imm0 },	/* XAA */
	{ 0x83, MODE_ABS_Y, parse_Standard_All },	/* XAS */
};


bool_t parse_AddressingMode(SAddressingMode* pAddrMode, int nAllowedModes)
{
	SLexBookmark bm;
	lex_Bookmark(&bm);

	if((nAllowedModes & MODE_A) && g_CurrentToken.ID.TargetToken == T_6502_REG_A)
	{
		parse_GetToken();
		pAddrMode->nMode = MODE_A;
		pAddrMode->pExpr = NULL;
		return true;
	}
	else if((nAllowedModes & MODE_IMM) && g_CurrentToken.ID.Token == '#')
	{
		parse_GetToken();
		pAddrMode->nMode = MODE_IMM;
		pAddrMode->pExpr = parse_ExpressionSU8();
		return true;
	}

	if((nAllowedModes & (MODE_IND_X | MODE_IND_Y)) && g_CurrentToken.ID.Token == '(')
	{
		parse_GetToken();
		pAddrMode->pExpr = parse_ExpressionSU8();

		if(pAddrMode->pExpr != NULL)
		{
			if(g_CurrentToken.ID.Token == ',')
			{
				parse_GetToken();
				if(g_CurrentToken.ID.TargetToken == T_6502_REG_X)
				{
					parse_GetToken();
					if(parse_ExpectChar(')'))
					{
						pAddrMode->nMode = MODE_IND_X;
						return true;
					}
				}
			}
			else if(g_CurrentToken.ID.Token == ')')
			{
				parse_GetToken();
				if(g_CurrentToken.ID.Token == ',')
				{
					parse_GetToken();
					if(g_CurrentToken.ID.TargetToken == T_6502_REG_Y)
					{
						parse_GetToken();
						pAddrMode->nMode = MODE_IND_Y;
						return true;
					}
				}
			}
		}

		lex_Goto(&bm);
	}

	if(nAllowedModes & MODE_IND)
	{
		if(g_CurrentToken.ID.Token == '(')
		{
			parse_GetToken();

			pAddrMode->pExpr = parse_Expression(2);
			if(pAddrMode->pExpr != NULL)
			{
				if(parse_ExpectChar(')'))
				{
					pAddrMode->nMode = MODE_IND;
					return true;
				}
			}
		}
		lex_Goto(&bm);
	}

	if(nAllowedModes & (MODE_ZP | MODE_ZP_X | MODE_ZP_Y | MODE_ABS | MODE_ABS_X | MODE_ABS_Y))
	{
		pAddrMode->pExpr = parse_Expression(2);

		if(pAddrMode->pExpr != NULL)
		{
			if(expr_IsConstant(pAddrMode->pExpr)
			&& 0 <= pAddrMode->pExpr->Value.Value && pAddrMode->pExpr->Value.Value <= 255)
			{
				if(g_CurrentToken.ID.Token == ',')
				{
					parse_GetToken();
					if(g_CurrentToken.ID.TargetToken == T_6502_REG_X)
					{
						parse_GetToken();
						pAddrMode->nMode = MODE_ZP_X;
						return true;
					}
					else if(g_CurrentToken.ID.TargetToken == T_6502_REG_Y)
					{
						parse_GetToken();
						pAddrMode->nMode = MODE_ZP_Y;
						return true;
					}
				}
				pAddrMode->nMode = MODE_ZP;
				return true;
			}

			if(g_CurrentToken.ID.Token == ',')
			{
				parse_GetToken();

				if(g_CurrentToken.ID.TargetToken == T_6502_REG_X)
				{
					parse_GetToken();
					pAddrMode->nMode = MODE_ABS_X;
					return true;
				}
				else if(g_CurrentToken.ID.TargetToken == T_6502_REG_Y)
				{
					parse_GetToken();
					pAddrMode->nMode = MODE_ABS_Y;
					return true;
				}
			}

			pAddrMode->nMode = MODE_ABS;
			return true;
		}

		lex_Goto(&bm);
	}

	if((nAllowedModes == 0) || (nAllowedModes & MODE_NONE))
	{
		pAddrMode->nMode = MODE_NONE;
		pAddrMode->pExpr = NULL;
		return true;
	}

	if(nAllowedModes & MODE_A)
	{
		pAddrMode->nMode = MODE_A;
		pAddrMode->pExpr = NULL;
		return true;
	}

	return false;
}


bool_t parse_IntegerInstruction(void)
{
	if(T_6502_ADC <= g_CurrentToken.ID.TargetToken && g_CurrentToken.ID.TargetToken <= T_6502U_XAS)
	{
		SAddressingMode addrMode;
		eTargetToken nToken = g_CurrentToken.ID.TargetToken;
		SParser* pParser = &g_Parsers[nToken - T_6502_ADC];

		parse_GetToken();
		if(parse_AddressingMode(&addrMode, pParser->nAddressingModes))
			return pParser->fpParser(nToken, pParser->nBaseOpcode, &addrMode);
		else
			prj_Error(MERROR_ILLEGAL_ADDRMODE);
	}

	return false;
}


#endif
