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

#if !defined(AM_M68K_)
#define AM_M68K_

typedef enum
{
	AM_DREG          = 0x00000001,	// Dn
	AM_AREG          = 0x00000002,	// An
	AM_AIND          = 0x00000004,	// (An)
	AM_AINC          = 0x00000008,	// (An)+
	AM_ADEC          = 0x00000010,	// -(An)
	AM_ADISP         = 0x00000020,	// d16(An)
	AM_AXDISP        = 0x00000040,	// d8(An,Xn)
	AM_PCDISP        = 0x00000080,	// d16(PC)
	AM_PCXDISP       = 0x00000100,	// d8(PC,Xn)
	AM_WORD          = 0x00000200,	// (xxx).W
	AM_LONG          = 0x00000400,	// (xxx).L
	AM_IMM           = 0x00000800,	// #xxx
	AM_AXDISP020     = 0x00010000,	// (bd,An,Xn)
	AM_PREINDAXD020  = 0x00020000,	// ([bd,An,Xn],od)
	AM_POSTINDAXD020 = 0x00040000,	// ([bd,An],Xn,od)
	AM_PCXDISP020    = 0x00080000,	// (bd,PC,Xn)
	AM_PREINDPCXD020 = 0x00100000,	// ([bd,PC,Xn],od)
	AM_POSTINDPCXD020= 0x00200000,	// ([bd,PC],Xn,od)
	AM_SYSREG        = 0x01000000,	// CCR
	AM_BITFIELD      = 0x20000000,	// {offset:width}
	AM_EMPTY         = 0x40000000
} EAddrMode;

typedef enum
{
	SIZE_DEFAULT = 0x0,
	SIZE_BYTE    = 0x1,
	SIZE_WORD    = 0x2,
	SIZE_LONG    = 0x4
} ESize;

typedef struct
{
	int	nBaseReg;

	int nIndexReg;
	ESize eIndexSize;
	SExpression* pIndexScale;

	SExpression* pDisp;
	ESize eDispSize;
} SModeRegs;

typedef struct
{
	EAddrMode	eMode;

	int	nDirectReg;

	SExpression* pImmediate;
	ESize	eImmSize;

	SModeRegs	Inner;
	SModeRegs	Outer;

	bool_t		bBitfield;
	int			nBFOffsetReg;
	SExpression* pBFOffsetExpr;
	int			nBFWidthReg;
	SExpression* pBFWidthExpr;

} SAddrMode;

typedef struct
{
	uint32_t	nSourceModes;
	uint32_t	nDestModes;
} SIntInstruction;

static void parse_OptimizeDisp(SModeRegs* pRegs);

SExpression* parse_CheckScaleRange(SExpression* pExpr)
{
	if((pExpr = expr_CheckRange(pExpr,1,8)) == NULL)
	{
		prj_Error(MERROR_SCALE_RANGE);
		return NULL;
	}

	return pExpr;
}

SExpression* parse_Check16bit(SExpression* pExpr)
{
	if((pExpr = expr_CheckRange(pExpr,-32768,65535)) == NULL)
	{
		prj_Error(ERROR_EXPRESSION_N_BIT, 16);
		return NULL;
	}

	return pExpr;
}

SExpression* parse_Check8bit(SExpression* pExpr)
{
	if((pExpr = expr_CheckRange(pExpr,-128,255)) == NULL)
	{
		prj_Error(ERROR_EXPRESSION_N_BIT, 8);
		return NULL;
	}

	return pExpr;
}

static ESize parse_GetSizeSpec(ESize eDefault)
{
	if(g_CurrentToken.ID.Token == T_ID && strlen(g_CurrentToken.Value.aString) == 2)
	{
		if(_strnicmp(g_CurrentToken.Value.aString,".b",2) == 0)
		{
			parse_GetToken();
			return SIZE_BYTE;
		}
		else if(_strnicmp(g_CurrentToken.Value.aString,".w",2) == 0)
		{
			parse_GetToken();
			return SIZE_WORD;
		}
		else if(_strnicmp(g_CurrentToken.Value.aString,".l",2) == 0)
		{
			parse_GetToken();
			return SIZE_LONG;
		}
	}

	return eDefault;
}

static bool_t parse_GetIndexReg(SModeRegs* pMode)
{
	if(g_CurrentToken.ID.TargetToken >= T_68K_REG_D0
	&& g_CurrentToken.ID.TargetToken <= T_68K_REG_D7)
	{
		pMode->nIndexReg = g_CurrentToken.ID.TargetToken - T_68K_REG_D0;
	}
	else if(g_CurrentToken.ID.TargetToken >= T_68K_REG_A0
	&& g_CurrentToken.ID.TargetToken <= T_68K_REG_A7)
	{
		pMode->nIndexReg = g_CurrentToken.ID.TargetToken - T_68K_REG_A0 + 8;
	}

	parse_GetToken();

	pMode->eIndexSize = parse_GetSizeSpec(SIZE_WORD);

	if(pMode->eIndexSize != SIZE_WORD
	&& pMode->eIndexSize != SIZE_LONG)
	{
		prj_Error(MERROR_INDEXREG_SIZE);
		return false;
	}

	if(g_CurrentToken.ID.TargetToken == T_OP_MUL)
	{
		parse_GetToken();
		if((pMode->pIndexScale = parse_Expression()) == NULL)
		{
			prj_Error(ERROR_EXPECT_EXPR);
			return false;
		}
		pMode->pIndexScale = parse_CheckScaleRange(pMode->pIndexScale);
		pMode->pIndexScale = expr_CreateBitExpr(pMode->pIndexScale);
	}

	return true;
}

static bool_t parse_SingleModePart(SModeRegs* pMode)
{
	// parses xxxx, Ax, PC, Xn.S*scale
	SExpression* expr;

	if(g_CurrentToken.ID.TargetToken == T_68K_REG_PC)
	{
		if(pMode->nBaseReg != -1)
			return false;

		pMode->nBaseReg = 16;
		parse_GetToken();
		return true;
	}

	if(g_CurrentToken.ID.TargetToken >= T_68K_REG_A0
	&& g_CurrentToken.ID.TargetToken <= T_68K_REG_A7)
	{
		int reg;
		ESize sz;

		if(pMode->nBaseReg != -1)
		{
			if(pMode->nIndexReg != -1)
			{
				return false;
			}

			return parse_GetIndexReg(pMode);
		}

		reg = g_CurrentToken.ID.TargetToken - T_68K_REG_A0;
		parse_GetToken();
		sz = parse_GetSizeSpec(SIZE_DEFAULT);
		if(sz == SIZE_WORD)
		{
			if(pMode->nIndexReg == -1)
			{
				pMode->nIndexReg = reg + 8;
				pMode->eIndexSize = SIZE_WORD;
				return true;
			}
			return false;
		}

		pMode->nBaseReg = reg;
		return true;
	}

	if(g_CurrentToken.ID.TargetToken >= T_68K_REG_D0
	&& g_CurrentToken.ID.TargetToken <= T_68K_REG_D7)
	{
		if(pMode->nIndexReg != -1)
		{
			return false;
		}

		return parse_GetIndexReg(pMode);
	}

	expr = parse_Expression();
	if(expr != NULL)
	{
		if(pMode->pDisp != NULL)
			return false;

		pMode->pDisp = expr;
		pMode->eDispSize = parse_GetSizeSpec(SIZE_DEFAULT);
		return true;
	}

	return false;
}

static bool_t parse_GetInnerMode(SAddrMode* pMode)
{
	for(;;)
	{
		if(!parse_SingleModePart(&pMode->Inner))
			return false;

		if(g_CurrentToken.ID.TargetToken == ']')
		{
			parse_GetToken();
			return true;
		}

		if(g_CurrentToken.ID.TargetToken == ',')
		{
			parse_GetToken();
			continue;
		}

		return false;
	}
}

static bool_t parse_GetOuterPart(SAddrMode* pMode)
{
	if(g_pOptions->pMachine->nCpu >= CPUF_68020 && g_CurrentToken.ID.TargetToken == '[')
	{
		parse_GetToken();
		return parse_GetInnerMode(pMode);
	}

	return parse_SingleModePart(&pMode->Outer);
}

static bool_t parse_GetOuterMode(SAddrMode* pMode)
{
	for(;;)
	{
		if(!parse_GetOuterPart(pMode))
			return false;

		if(g_CurrentToken.ID.TargetToken == ')')
		{
			parse_GetToken();
			return true;
		}

		if(g_CurrentToken.ID.TargetToken == ',')
		{
			parse_GetToken();
			continue;
		}

		return false;
	}

	return false;
}


static void parse_OptimizeFields(SModeRegs* pRegs)
{
	if(pRegs->pDisp != NULL
	&& expr_IsConstant(pRegs->pDisp)
	&& pRegs->pDisp->Value.Value == 0)
	{
		expr_Free(pRegs->pDisp);
		pRegs->pDisp = NULL;
	}

	if(pRegs->pIndexScale != NULL
	&& expr_IsConstant(pRegs->pIndexScale)
	&& pRegs->pIndexScale->Value.Value == 0)
	{
		expr_Free(pRegs->pIndexScale);
		pRegs->pIndexScale = NULL;
	}

	if(pRegs->nBaseReg == -1
	&& pRegs->nIndexReg >= 8
	&& pRegs->nIndexReg <= 15
	&& pRegs->eIndexSize == SIZE_LONG)
	{
		pRegs->nBaseReg = pRegs->nIndexReg;
		pRegs->nIndexReg = -1;
	}
}

#define I_BASE  0x01
#define I_INDEX 0x02
#define I_DISP  0x04
#define O_BASE  0x08
#define O_INDEX 0x10
#define O_DISP  0x20

static bool_t parse_OptimizeMode(SAddrMode* pMode)
{
	int inner = 0;

	parse_OptimizeFields(&pMode->Inner);
	parse_OptimizeFields(&pMode->Outer);

	if(pMode->Inner.nBaseReg != -1)
		inner |= I_BASE;

	if(pMode->Inner.nIndexReg != -1)
		inner |= I_INDEX;

	if(pMode->Inner.pDisp != NULL)
		inner |= I_DISP;

	if(pMode->Outer.nBaseReg != -1)
		inner |= O_BASE;

	if(pMode->Outer.nIndexReg != -1)
		inner |= O_INDEX;

	if(pMode->Outer.pDisp != NULL)
		inner |= O_DISP;

	if((inner & (I_BASE|I_INDEX|I_DISP)) != 0)
	{
		if(pMode->Inner.pDisp != NULL
		&& pMode->Inner.eDispSize == SIZE_BYTE)
			pMode->Inner.eDispSize = SIZE_WORD;

		if(pMode->Outer.pDisp != NULL
		&& pMode->Outer.eDispSize == SIZE_BYTE)
			pMode->Outer.eDispSize = SIZE_WORD;

		if(pMode->Outer.nBaseReg != -1
		&& pMode->Outer.nIndexReg != -1)
			return false;

		if(pMode->Outer.nBaseReg != -1
		&& pMode->Outer.nIndexReg == -1)
		{
			pMode->Outer.nIndexReg = pMode->Outer.nBaseReg;
			pMode->Outer.eIndexSize = SIZE_LONG;
			pMode->Outer.nBaseReg = -1;
			pMode->Outer.pIndexScale = NULL;
		}

		switch(inner)
		{
			default:
				return false;
			case I_BASE:
			case          I_INDEX:
			case I_BASE | I_INDEX:
			case                    I_DISP:
			case I_BASE           | I_DISP:
			case          I_INDEX | I_DISP:
			case I_BASE | I_INDEX | I_DISP:
			case                             O_DISP:
			case I_BASE                    | O_DISP:
			case          I_INDEX          | O_DISP:
			case I_BASE | I_INDEX          | O_DISP:
			case                    I_DISP | O_DISP:
			case I_BASE           | I_DISP | O_DISP:
			case          I_INDEX | I_DISP | O_DISP:
			case I_BASE | I_INDEX | I_DISP | O_DISP:
				// ([bd,An,Xn],od)
				if(pMode->Inner.nBaseReg == 16)
					pMode->eMode = AM_PREINDPCXD020;
				else
					pMode->eMode = AM_PREINDAXD020;
				return true;

			//case I_BASE:
			//case          I_DISP:
			//case I_BASE | I_DISP:
			case                   O_INDEX:
			case I_BASE          | O_INDEX:
			case          I_DISP | O_INDEX:
			case I_BASE | I_DISP | O_INDEX:
			//case                             O_DISP:
			//case I_BASE                    | O_DISP:
			//case          I_DISP           | O_DISP:
			//case I_BASE | I_DISP           | O_DISP:
			//case                 | O_INDEX | O_DISP:
			case I_BASE          | O_INDEX | O_DISP:
			case          I_DISP | O_INDEX | O_DISP:
			case I_BASE | I_DISP | O_INDEX | O_DISP:
				// ([bd,An],Xn,od)
				if(pMode->Inner.nBaseReg == 16)
					pMode->eMode = AM_POSTINDPCXD020;
				else
					pMode->eMode = AM_POSTINDAXD020;
				return true;
		}
	}

	switch(inner)
	{
		case O_BASE:
			pMode->eMode = AM_AIND;
			return true;
		case          O_INDEX:
		case          O_INDEX | O_DISP:
			pMode->eMode = AM_AXDISP020;
			return true;
		case O_BASE | O_INDEX:
			pMode->eMode = AM_AXDISP;
			return true;
		case                    O_DISP:
			if(pMode->Outer.eDispSize == SIZE_BYTE)
			{
				prj_Error(MERROR_DISP_SIZE);
				return false;
			}
			if(pMode->Outer.eDispSize == SIZE_WORD)
				pMode->eMode = AM_WORD;
			else
				pMode->eMode = AM_LONG;
			return true;
		case O_BASE           | O_DISP:
			if(pMode->Outer.eDispSize == SIZE_BYTE)
			{
				prj_Error(MERROR_DISP_SIZE);
				return false;
			}

			if(g_pOptions->pMachine->nCpu <= CPUF_68010)
			{
				if(pMode->Outer.eDispSize == SIZE_DEFAULT)
					pMode->Outer.eDispSize = SIZE_WORD;

				if(pMode->Outer.nBaseReg == 16)
					pMode->eMode = AM_PCDISP;
				else
					pMode->eMode = AM_ADISP;

				return true;
			}

			parse_OptimizeDisp(&pMode->Outer);
			if(pMode->Outer.eDispSize == SIZE_WORD)
			{
				if(pMode->Outer.nBaseReg == 16)
					pMode->eMode = AM_PCDISP;
				else
					pMode->eMode = AM_ADISP;

				return true;
			}

			if(pMode->Outer.nBaseReg == 16)
				pMode->eMode = AM_PCXDISP020;
			else
				pMode->eMode = AM_AXDISP020;

			return true;
		case O_BASE | O_INDEX | O_DISP:
			if(g_pOptions->pMachine->nCpu <= CPUF_68010)
			{
				if(pMode->Outer.eDispSize == SIZE_DEFAULT)
					pMode->Outer.eDispSize = SIZE_BYTE;
			}
			else
			{
				if(pMode->Outer.eDispSize == SIZE_DEFAULT)
				{
					if(expr_IsConstant(pMode->Outer.pDisp))
					{
						if(pMode->Outer.pDisp->Value.Value >= -128
						&& pMode->Outer.pDisp->Value.Value <= 127)
							pMode->Outer.eDispSize = SIZE_BYTE;
						else if(pMode->Outer.pDisp->Value.Value >= -32768
						&& pMode->Outer.pDisp->Value.Value <= 32767)
							pMode->Outer.eDispSize = SIZE_WORD;
						else
							pMode->Outer.eDispSize = SIZE_LONG;
					}
					else
						pMode->Outer.eDispSize = SIZE_BYTE;
				}
			}

			if(pMode->Outer.eDispSize == SIZE_BYTE)
			{
				if(pMode->Outer.nBaseReg == 16)
					pMode->eMode = AM_PCXDISP;
				else
					pMode->eMode = AM_AXDISP;
			}
			else
			{
				if(pMode->Outer.nBaseReg == 16)
					pMode->eMode = AM_PCXDISP020;
				else
					pMode->eMode = AM_AXDISP020;
			}
			return true;
	}

	return false;
}


static void parse_OptimizeDisp(SModeRegs* pRegs)
{
	if(pRegs->pDisp != NULL
	&& pRegs->eDispSize == SIZE_DEFAULT)
	{
		if(expr_IsConstant(pRegs->pDisp))
		{
			if(pRegs->pDisp->Value.Value >= -32768
			&& pRegs->pDisp->Value.Value <= 32767)
				pRegs->eDispSize = SIZE_WORD;
			else
				pRegs->eDispSize = SIZE_LONG;
		}
		else
			pRegs->eDispSize = SIZE_LONG;
	}
}


static bool_t parse_GetAddrMode(SAddrMode* pMode)
{
	pMode->Inner.nBaseReg = -1;
	pMode->Inner.nIndexReg = -1;
	pMode->Inner.pIndexScale = NULL;
	pMode->Inner.pDisp = NULL;
	pMode->Outer.nBaseReg = -1;
	pMode->Outer.nIndexReg = -1;
	pMode->Outer.pIndexScale = NULL;
	pMode->Outer.pDisp = NULL;
	pMode->bBitfield = false;

	if(g_CurrentToken.ID.TargetToken >= T_68K_SYSREG_FIRST
	&& g_CurrentToken.ID.TargetToken <= T_68K_SYSREG_LAST)
	{
		pMode->eMode = AM_SYSREG;
		pMode->nDirectReg = g_CurrentToken.ID.TargetToken;
		parse_GetToken();
		return true;
	}

	if(g_CurrentToken.ID.TargetToken >= T_68K_REG_D0
	&& g_CurrentToken.ID.TargetToken <= T_68K_REG_D7)
	{
		pMode->eMode = AM_DREG;
		pMode->nDirectReg = g_CurrentToken.ID.TargetToken - T_68K_REG_D0;
		parse_GetToken();
		return true;
	}

	if(g_CurrentToken.ID.TargetToken >= T_68K_REG_A0
	&& g_CurrentToken.ID.TargetToken <= T_68K_REG_A7)
	{
		pMode->eMode = AM_AREG;
		pMode->nDirectReg = g_CurrentToken.ID.TargetToken - T_68K_REG_A0;
		parse_GetToken();
		return true;
	}

	if(g_CurrentToken.ID.TargetToken >= T_68K_REG_A0_IND
	&& g_CurrentToken.ID.TargetToken <= T_68K_REG_A7_IND)
	{
		pMode->eMode = AM_AIND;
		pMode->Outer.nBaseReg = g_CurrentToken.ID.TargetToken - T_68K_REG_A0_IND;
		parse_GetToken();
		return true;
	}

	if(g_CurrentToken.ID.TargetToken >= T_68K_REG_A0_DEC
	&& g_CurrentToken.ID.TargetToken <= T_68K_REG_A7_DEC)
	{
		pMode->eMode = AM_ADEC;
		pMode->Outer.nBaseReg = g_CurrentToken.ID.TargetToken - T_68K_REG_A0_DEC;
		parse_GetToken();
		return true;
	}

	if(g_CurrentToken.ID.TargetToken >= T_68K_REG_A0_INC
	&& g_CurrentToken.ID.TargetToken <= T_68K_REG_A7_INC)
	{
		pMode->eMode = AM_AINC;
		pMode->Outer.nBaseReg = g_CurrentToken.ID.TargetToken - T_68K_REG_A0_INC;
		parse_GetToken();
		return true;
	}

	if(g_CurrentToken.ID.TargetToken == '#')
	{
		parse_GetToken();
		pMode->eMode = AM_IMM;
		pMode->pImmediate = parse_Expression();
		return pMode->pImmediate != NULL;
	}

	pMode->Outer.pDisp = parse_Expression();
	if(pMode->Outer.pDisp != NULL)
		pMode->Outer.eDispSize = parse_GetSizeSpec(SIZE_DEFAULT);

	// parse (xxxx)
	if(g_CurrentToken.ID.TargetToken == '(')
	{
		parse_GetToken();

		if(parse_GetOuterMode(pMode))
		{
			return parse_OptimizeMode(pMode);
		}
		return false;
	}

	if(pMode->Outer.pDisp != NULL)
	{
		if(g_CurrentToken.ID.TargetToken == T_68K_REG_PC_IND)
		{
			pMode->eMode = AM_PCDISP;
			pMode->Outer.nBaseReg = 16;
			pMode->Outer.eDispSize = SIZE_WORD;
			parse_GetToken();
			return true;
		}
		else if(g_CurrentToken.ID.TargetToken >= T_68K_REG_A0_IND
		&& g_CurrentToken.ID.TargetToken <= T_68K_REG_A7_IND)
		{
			if((pMode->Outer.pDisp = parse_Check16bit(pMode->Outer.pDisp)) != NULL)
			{
				pMode->eMode = AM_ADISP;
				pMode->Outer.nBaseReg = g_CurrentToken.ID.TargetToken - T_68K_REG_A0_IND;
				pMode->Outer.eDispSize = SIZE_WORD;
				parse_GetToken();
				return true;
			}
		}
	}

	if(parse_GetOuterMode(pMode))
	{
		return parse_OptimizeMode(pMode);
	}

	parse_OptimizeDisp(&pMode->Outer);

	if(pMode->Outer.pDisp != NULL)
	{
		if(pMode->Outer.eDispSize == SIZE_WORD)
		{
			pMode->eMode = AM_WORD;
			return true;
		}
		else if(pMode->Outer.eDispSize == SIZE_LONG)
		{
			pMode->eMode = AM_LONG;
			return true;
		}
		else
			prj_Error(MERROR_DISP_SIZE);
	}

	return false;
}


#endif
