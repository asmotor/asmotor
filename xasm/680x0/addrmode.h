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

#if !defined(AM_M68K_)
#define AM_M68K_

typedef enum
{
	AM_NONE = 0,
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
	AM_FPUREG        = 0x02000000,	// FPU
	AM_BITFIELD      = 0x20000000,	// {offset:width}
	AM_EMPTY         = 0x40000000
} EAddrMode;

typedef enum
{
	SIZE_DEFAULT  = 0x00,
	SIZE_BYTE     = 0x01,
	SIZE_WORD     = 0x02,
	SIZE_LONG     = 0x04,
	SIZE_SINGLE   = 0x08,
	SIZE_DOUBLE   = 0x10,
	SIZE_EXTENDED = 0x20,
	SIZE_PACKED   = 0x40,
} ESize;

typedef enum {
	REG_D0 = 0,
	REG_D1 = 1,
	REG_D2 = 2,
	REG_D3 = 3,
	REG_D4 = 4,
	REG_D5 = 5,
	REG_D6 = 6,
	REG_D7 = 7,
	REG_A0 = 8,
	REG_A1 = 9,
	REG_A2 = 10,
	REG_A3 = 11,
	REG_A4 = 12,
	REG_A5 = 13,
	REG_A6 = 14,
	REG_A7 = 15,
	REG_FP0 = 16,
	REG_FP1 = 17,
	REG_FP2 = 18,
	REG_FP3 = 19,
	REG_FP4 = 20,
	REG_FP5 = 21,
	REG_FP6 = 22,
	REG_FP7 = 23,
	REG_PC = 16,
	REG_NONE = 255
} ERegister;

typedef struct
{
	ERegister nBaseReg;

	ERegister nIndexReg;
	ESize eIndexSize;
	SExpression* pIndexScale;

	SExpression* pDisp;
	ESize eDispSize;
} SModeRegs;

typedef struct
{
	EAddrMode	eMode;

	uint16_t	nDirectReg;

	SExpression* pImmediate;
	ESize	eImmSize;

	SModeRegs	Inner;
	SModeRegs	Outer;

	bool		bBitfield;
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
	if(lex_Current.token == T_ID && strlen(lex_Current.value.string) == 2)
	{
		if(_strnicmp(lex_Current.value.string,".b",2) == 0)
		{
			parse_GetToken();
			return SIZE_BYTE;
		}
		else if(_strnicmp(lex_Current.value.string,".w",2) == 0)
		{
			parse_GetToken();
			return SIZE_WORD;
		}
		else if(_strnicmp(lex_Current.value.string,".l",2) == 0)
		{
			parse_GetToken();
			return SIZE_LONG;
		}
		else if(_strnicmp(lex_Current.value.string,".s",2) == 0)
		{
			parse_GetToken();
			return SIZE_SINGLE;
		}
		else if(_strnicmp(lex_Current.value.string,".d",2) == 0)
		{
			parse_GetToken();
			return SIZE_DOUBLE;
		}
		else if(_strnicmp(lex_Current.value.string,".x",2) == 0)
		{
			parse_GetToken();
			return SIZE_EXTENDED;
		}
		else if(_strnicmp(lex_Current.value.string,".p",2) == 0)
		{
			parse_GetToken();
			return SIZE_PACKED;
		}
	}

	return eDefault;
}

static bool parse_GetIndexReg(SModeRegs* pMode)
{
	if(lex_Current.token >= T_68K_REG_D0
	&& lex_Current.token <= T_68K_REG_D7)
	{
		pMode->nIndexReg = REG_D0 + (lex_Current.token - T_68K_REG_D0);
	}
	else if(lex_Current.token >= T_68K_REG_A0
	&& lex_Current.token <= T_68K_REG_A7)
	{
		pMode->nIndexReg = REG_A0 + (lex_Current.token - T_68K_REG_A0);
	}

	parse_GetToken();

	pMode->eIndexSize = parse_GetSizeSpec(SIZE_WORD);

	if(pMode->eIndexSize != SIZE_WORD
	&& pMode->eIndexSize != SIZE_LONG)
	{
		prj_Error(MERROR_INDEXREG_SIZE);
		return false;
	}

	if(lex_Current.token == T_OP_MULTIPLY)
	{
		parse_GetToken();
		if((pMode->pIndexScale = parse_Expression(1)) == NULL)
		{
			prj_Error(ERROR_EXPECT_EXPR);
			return false;
		}
		pMode->pIndexScale = parse_CheckScaleRange(pMode->pIndexScale);
		pMode->pIndexScale = expr_Bit(pMode->pIndexScale);
	}

	return true;
}

static bool parse_SingleModePart(SModeRegs* pMode)
{
	// parses xxxx, Ax, PC, Xn.S*scale
	SExpression* expr;

	if(lex_Current.token == T_68K_REG_PC)
	{
		if(pMode->nBaseReg != REG_NONE)
			return false;

		pMode->nBaseReg = REG_PC;
		parse_GetToken();
		return true;
	}

	if(lex_Current.token >= T_68K_REG_A0
	&& lex_Current.token <= T_68K_REG_A7)
	{
		ESize sz;

		if(pMode->nBaseReg != REG_NONE)
		{
			if(pMode->nIndexReg != REG_NONE)
			{
				return false;
			}

			return parse_GetIndexReg(pMode);
		}

		int addressRegister = lex_Current.token - T_68K_REG_A0;
		parse_GetToken();
		sz = parse_GetSizeSpec(SIZE_DEFAULT);
		if(sz == SIZE_WORD)
		{
			if(pMode->nIndexReg == REG_NONE)
			{
				pMode->nIndexReg = REG_A0 + addressRegister;
				pMode->eIndexSize = SIZE_WORD;
				return true;
			}
			return false;
		}

		pMode->nBaseReg = REG_A0 + addressRegister;
		return true;
	}

	if(lex_Current.token >= T_68K_REG_D0
	&& lex_Current.token <= T_68K_REG_D7)
	{
		if(pMode->nIndexReg != REG_NONE)
		{
			return false;
		}

		return parse_GetIndexReg(pMode);
	}

	expr = parse_Expression(4);
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

static bool parse_GetInnerMode(SAddrMode* pMode)
{
	for(;;)
	{
		if(!parse_SingleModePart(&pMode->Inner))
			return false;

		if(lex_Current.token == ']')
		{
			parse_GetToken();
			return true;
		}

		if(lex_Current.token == ',')
		{
			parse_GetToken();
			continue;
		}

		return false;
	}
}

static bool parse_GetOuterPart(SAddrMode* pMode)
{
	if(opt_Current->machineOptions->nCpu >= CPUF_68020 && lex_Current.token == '[')
	{
		parse_GetToken();
		return parse_GetInnerMode(pMode);
	}

	return parse_SingleModePart(&pMode->Outer);
}

static bool parse_GetOuterMode(SAddrMode* pMode)
{
	for(;;)
	{
		if(!parse_GetOuterPart(pMode))
			return false;

		if(lex_Current.token == ')')
		{
			parse_GetToken();
			return true;
		}

		if(lex_Current.token == ',')
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
	&& pRegs->pDisp->value.integer == 0)
	{
		expr_Free(pRegs->pDisp);
		pRegs->pDisp = NULL;
	}

	if(pRegs->pIndexScale != NULL
	&& expr_IsConstant(pRegs->pIndexScale)
	&& pRegs->pIndexScale->value.integer == 0)
	{
		expr_Free(pRegs->pIndexScale);
		pRegs->pIndexScale = NULL;
	}

	if(pRegs->nBaseReg == REG_NONE
	&& pRegs->nIndexReg >= REG_A0
	&& pRegs->nIndexReg <= REG_A7
	&& pRegs->eIndexSize == SIZE_LONG)
	{
		pRegs->nBaseReg = pRegs->nIndexReg;
		pRegs->nIndexReg = REG_NONE;
	}
}

#define I_BASE  0x01
#define I_INDEX 0x02
#define I_DISP  0x04
#define O_BASE  0x08
#define O_INDEX 0x10
#define O_DISP  0x20

static bool parse_OptimizeMode(SAddrMode* pMode)
{
	uint32_t inner = 0;

	parse_OptimizeFields(&pMode->Inner);
	parse_OptimizeFields(&pMode->Outer);

	if(pMode->Inner.nBaseReg != REG_NONE)
		inner |= I_BASE;

	if(pMode->Inner.nIndexReg != REG_NONE)
		inner |= I_INDEX;

	if(pMode->Inner.pDisp != NULL)
		inner |= I_DISP;

	if(pMode->Outer.nBaseReg != REG_NONE)
		inner |= O_BASE;

	if(pMode->Outer.nIndexReg != REG_NONE)
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

		if(pMode->Outer.nBaseReg != REG_NONE
		&& pMode->Outer.nIndexReg != REG_NONE)
			return false;

		if(pMode->Outer.nBaseReg != REG_NONE
		&& pMode->Outer.nIndexReg == REG_NONE)
		{
			pMode->Outer.nIndexReg = pMode->Outer.nBaseReg;
			pMode->Outer.eIndexSize = SIZE_LONG;
			pMode->Outer.nBaseReg = REG_NONE;
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
				if(pMode->Inner.nBaseReg == REG_PC)
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
				if(pMode->Inner.nBaseReg == REG_PC)
					pMode->eMode = AM_POSTINDPCXD020;
				else
					pMode->eMode = AM_POSTINDAXD020;
				return true;
		}
	}

	switch(inner)
	{
		default:
			return false;
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

			if(opt_Current->machineOptions->nCpu <= CPUF_68010)
			{
				if(pMode->Outer.eDispSize == SIZE_DEFAULT)
					pMode->Outer.eDispSize = SIZE_WORD;

				if(pMode->Outer.nBaseReg == REG_PC)
					pMode->eMode = AM_PCDISP;
				else
					pMode->eMode = AM_ADISP;

				return true;
			}

			parse_OptimizeDisp(&pMode->Outer);
			if(pMode->Outer.eDispSize == SIZE_WORD)
			{
				if(pMode->Outer.nBaseReg == REG_PC)
					pMode->eMode = AM_PCDISP;
				else
					pMode->eMode = AM_ADISP;

				return true;
			}

			if(pMode->Outer.nBaseReg == REG_PC)
				pMode->eMode = AM_PCXDISP020;
			else
				pMode->eMode = AM_AXDISP020;

			return true;
		case O_BASE | O_INDEX | O_DISP:
			if(opt_Current->machineOptions->nCpu <= CPUF_68010)
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
						if(pMode->Outer.pDisp->value.integer >= -128
						&& pMode->Outer.pDisp->value.integer <= 127)
							pMode->Outer.eDispSize = SIZE_BYTE;
						else if(pMode->Outer.pDisp->value.integer >= -32768
						&& pMode->Outer.pDisp->value.integer <= 32767)
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
				if(pMode->Outer.nBaseReg == REG_PC)
					pMode->eMode = AM_PCXDISP;
				else
					pMode->eMode = AM_AXDISP;
			}
			else
			{
				if(pMode->Outer.nBaseReg == REG_PC)
					pMode->eMode = AM_PCXDISP020;
				else
					pMode->eMode = AM_AXDISP020;
			}
			return true;
	}
}


static void parse_OptimizeDisp(SModeRegs* pRegs)
{
	if(pRegs->pDisp != NULL
	&& pRegs->eDispSize == SIZE_DEFAULT)
	{
		if(expr_IsConstant(pRegs->pDisp))
		{
			if(pRegs->pDisp->value.integer >= -32768
			&& pRegs->pDisp->value.integer <= 32767)
				pRegs->eDispSize = SIZE_WORD;
			else
				pRegs->eDispSize = SIZE_LONG;
		}
		else
			pRegs->eDispSize = SIZE_LONG;
	}
}


static bool parse_GetAddrMode(SAddrMode* pMode)
{
	pMode->Inner.nBaseReg = REG_NONE;
	pMode->Inner.nIndexReg = REG_NONE;
	pMode->Inner.pIndexScale = NULL;
	pMode->Inner.pDisp = NULL;
	pMode->Outer.nBaseReg = REG_NONE;
	pMode->Outer.nIndexReg = REG_NONE;
	pMode->Outer.pIndexScale = NULL;
	pMode->Outer.pDisp = NULL;
	pMode->bBitfield = false;

	if(lex_Current.token >= T_68K_SYSREG_FIRST
	&& lex_Current.token <= T_68K_SYSREG_LAST)
	{
		pMode->eMode = AM_SYSREG;
		pMode->nDirectReg = lex_Current.token;
		parse_GetToken();
		return true;
	}

	if(lex_Current.token >= T_68K_REG_D0
	&& lex_Current.token <= T_68K_REG_D7)
	{
		pMode->eMode = AM_DREG;
		pMode->nDirectReg = lex_Current.token - T_68K_REG_D0;
		parse_GetToken();
		return true;
	}

	if(lex_Current.token >= T_68K_REG_A0
	&& lex_Current.token <= T_68K_REG_A7)
	{
		pMode->eMode = AM_AREG;
		pMode->nDirectReg = lex_Current.token - T_68K_REG_A0;
		parse_GetToken();
		return true;
	}

	if(lex_Current.token >= T_68K_REG_A0_IND
	&& lex_Current.token <= T_68K_REG_A7_IND)
	{
		pMode->eMode = AM_AIND;
		pMode->Outer.nBaseReg = REG_A0 + (lex_Current.token - T_68K_REG_A0_IND);
		parse_GetToken();
		return true;
	}

	if(lex_Current.token >= T_68K_REG_A0_DEC
	&& lex_Current.token <= T_68K_REG_A7_DEC)
	{
		pMode->eMode = AM_ADEC;
		pMode->Outer.nBaseReg = REG_A0 + (lex_Current.token - T_68K_REG_A0_DEC);
		parse_GetToken();
		return true;
	}

	if(lex_Current.token >= T_68K_REG_A0_INC
	&& lex_Current.token <= T_68K_REG_A7_INC)
	{
		pMode->eMode = AM_AINC;
		pMode->Outer.nBaseReg = REG_A0 + (lex_Current.token - T_68K_REG_A0_INC);
		parse_GetToken();
		return true;
	}

	if(lex_Current.token >= T_FPUREG_0
	&& lex_Current.token <= T_FPUREG_7)
	{
		pMode->eMode = AM_FPUREG;
		pMode->Outer.nBaseReg = REG_FP0 + (lex_Current.token - T_FPUREG_0);
		parse_GetToken();
		return true;
	}

	if(lex_Current.token == '#')
	{
		parse_GetToken();
		pMode->eMode = AM_IMM;
		pMode->pImmediate = parse_Expression(4);
		return pMode->pImmediate != NULL;
	}

	pMode->Outer.pDisp = parse_Expression(4);
	if(pMode->Outer.pDisp != NULL)
		pMode->Outer.eDispSize = parse_GetSizeSpec(SIZE_DEFAULT);

	// parse (xxxx)
	if(lex_Current.token == '(')
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
		if(lex_Current.token == T_68K_REG_PC_IND)
		{
			pMode->eMode = AM_PCDISP;
			pMode->Outer.nBaseReg = REG_PC;
			pMode->Outer.eDispSize = SIZE_WORD;
			parse_GetToken();
			return true;
		}
		else if(lex_Current.token >= T_68K_REG_A0_IND
		&& lex_Current.token <= T_68K_REG_A7_IND)
		{
			if((pMode->Outer.pDisp = parse_Check16bit(pMode->Outer.pDisp)) != NULL)
			{
				pMode->eMode = AM_ADISP;
				pMode->Outer.nBaseReg = REG_A0 + (lex_Current.token - T_68K_REG_A0_IND);
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
