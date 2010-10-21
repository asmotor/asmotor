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

#if !defined(INTEGER_INSTRUCTIONS_M68K_)
#define INTEGER_INSTRUCTIONS_M68K_

#include <assert.h>

#include "loccpu.h"

static BOOL parse_IntegerOp(int op, ESize inssz, SAddrMode* src, SAddrMode* dest);

static BOOL parse_OutputExtWords(SAddrMode* mode)
{
	switch(mode->eMode)
	{
		case AM_IMM:
		{
			switch(mode->eImmSize)
			{
				case SIZE_BYTE:
				{
					mode->pImmediate = parse_Check8bit(mode->pImmediate);
					if(mode->pImmediate)
					{
						sect_OutputExprWord(mode->pImmediate);
						return TRUE;
					}
					return FALSE;
				}
				default:
				case SIZE_WORD:
				{
					mode->pImmediate = parse_Check16bit(mode->pImmediate);
					if(mode->pImmediate)
					{
						sect_OutputExprWord(mode->pImmediate);
						return TRUE;
					}
					return FALSE;
				}
				case SIZE_LONG:
				{
					if(mode->pImmediate)
					{
						sect_OutputExprLong(mode->pImmediate);
						return TRUE;
					}
					return FALSE;
				}
			}
		}
		case AM_WORD:
		{
			if(mode->Outer.pDisp)
			{
				sect_OutputExprWord(mode->Outer.pDisp);
				return TRUE;
			}
			internalerror("no word");
			break;
		}
		case AM_LONG:
		{
			if(mode->Outer.pDisp)
			{
				sect_OutputExprLong(mode->Outer.pDisp);
				return TRUE;
			}
			internalerror("no long");
			break;
		}
		case AM_PCDISP:
		{
			if(mode->Outer.pDisp)
				mode->Outer.pDisp = parse_CreatePCRelExpr(mode->Outer.pDisp, 0);
		}
		// fall through
		case AM_ADISP:
		{
			if(mode->Outer.pDisp)
			{
				if(mode->Outer.eDispSize == SIZE_WORD
				|| mode->Outer.eDispSize == SIZE_DEFAULT)
				{
					sect_OutputExprWord(mode->Outer.pDisp);
					return TRUE;
				}
				prj_Error(MERROR_DISP_SIZE);
				return FALSE;
			}
			internalerror("no displacement word");
			break;
		}
		case AM_PCXDISP:
		{
			if(mode->Outer.pDisp)
				mode->Outer.pDisp = parse_CreatePCRelExpr(mode->Outer.pDisp, 0);
		}
		// fall through
		case AM_AXDISP:
		{
			SExpression* expr;
			UWORD ins = (UWORD)(mode->Outer.nIndexReg << 12);
			if(mode->Outer.eIndexSize == SIZE_LONG)
				ins |= 0x0800;

			if(mode->Outer.pDisp != NULL)
				expr = parse_Check8bit(mode->Outer.pDisp);
			else
				expr = parse_CreateConstExpr(0);

			expr = parse_CreateANDExpr(expr, parse_CreateConstExpr(0xFF));
			if(expr != NULL)
			{
				expr = parse_CreateORExpr(expr, parse_CreateConstExpr(ins));
				if(mode->Outer.pIndexScale != NULL)
				{
					expr = parse_CreateORExpr(expr, parse_CreateSHLExpr(mode->Outer.pIndexScale, parse_CreateConstExpr(9)));
				}
				sect_OutputExprWord(expr);
				return TRUE;
			}
			return FALSE;
		}

		case AM_DREG:
		case AM_AREG:
		case AM_AIND:
		case AM_AINC:
		case AM_ADEC:
			return TRUE;

		case AM_PCXDISP020:
		{
			if(mode->Outer.pDisp)
				mode->Outer.pDisp = parse_CreatePCRelExpr(mode->Outer.pDisp, 2);
		}
		// fall through
		case AM_AXDISP020:
		{
			UWORD ins = 0x0100;
			SExpression* expr;

			if(mode->Outer.nBaseReg == -1)
			{
				ins |= 0x0080;
			}

			if(mode->Outer.nIndexReg == -1)
			{
				ins |= 0x0040;
			}
			else
			{
				ins |= mode->Outer.nIndexReg << 12;
				if(mode->Outer.nIndexReg == SIZE_LONG)
					ins |= 0x0800;
			}

			if(mode->Outer.pDisp != NULL)
			{
				parse_OptimizeDisp(&mode->Outer);
				switch(mode->Outer.eDispSize)
				{
					case SIZE_WORD:
						ins |= 0x0020;
						break;
					case SIZE_LONG:
						ins |= 0x0030;
						break;
					default:
						internalerror("unknown BD size");
						return FALSE;
				}
			}
			else
			{
				ins |= 0x0010;
			}

			expr = parse_CreateConstExpr(ins);
			if(expr != NULL)
			{
				if(mode->Outer.pIndexScale != NULL)
				{
					expr = parse_CreateORExpr(expr, parse_CreateSHLExpr(mode->Outer.pIndexScale, parse_CreateConstExpr(9)));
				}
				sect_OutputExprWord(expr);

				if(mode->Outer.pDisp != NULL)
				{
					switch(mode->Outer.eDispSize)
					{
						default:
						case SIZE_WORD:
							sect_OutputExprWord(mode->Outer.pDisp);
							break;
						case SIZE_LONG:
							sect_OutputExprLong(mode->Outer.pDisp);
							break;
					}
				}

				return TRUE;
			}
			return FALSE;
		}

		case AM_PREINDPCXD020:
		{
			if(mode->Inner.pDisp)
				mode->Inner.pDisp = parse_CreatePCRelExpr(mode->Inner.pDisp, 2);
		}
		// fall through
		case AM_PREINDAXD020:
		{
			UWORD ins = 0x0100;
			SExpression* expr;

			if(mode->Inner.nBaseReg == -1)
			{
				ins |= 0x0080;
			}

			if(mode->Inner.nIndexReg == -1)
			{
				ins |= 0x0040;
			}
			else
			{
				ins |= mode->Inner.nIndexReg << 12;
				if(mode->Inner.nIndexReg == SIZE_LONG)
					ins |= 0x0800;
			}

			if(mode->Inner.pDisp != NULL)
			{
				parse_OptimizeDisp(&mode->Inner);
				switch(mode->Inner.eDispSize)
				{
					case SIZE_WORD:
						ins |= 0x0020;
						break;
					case SIZE_LONG:
						ins |= 0x0030;
						break;
					default:
						internalerror("unknown BD size");
						return FALSE;
				}
			}
			else
			{
				ins |= 0x0010;
			}

			if(mode->Outer.pDisp != NULL)
			{
				parse_OptimizeDisp(&mode->Outer);
				switch(mode->Outer.eDispSize)
				{
					case SIZE_WORD:
						ins |= 0x0002;
						break;
					case SIZE_LONG:
						ins |= 0x0003;
						break;
					default:
						internalerror("unknown OD size");
						return FALSE;
				}
			}
			else
			{
				ins |= 0x0001;
			}

			expr = parse_CreateConstExpr(ins);
			if(expr != NULL)
			{
				if(mode->Inner.pIndexScale != NULL)
				{
					expr = parse_CreateORExpr(expr, parse_CreateSHLExpr(mode->Inner.pIndexScale, parse_CreateConstExpr(9)));
				}
				sect_OutputExprWord(expr);

				if(mode->Inner.pDisp != NULL)
				{
					switch(mode->Inner.eDispSize)
					{
						default:
						case SIZE_WORD:
							sect_OutputExprWord(mode->Inner.pDisp);
							break;
						case SIZE_LONG:
							sect_OutputExprLong(mode->Inner.pDisp);
							break;
					}
				}

				if(mode->Outer.pDisp != NULL)
				{
					switch(mode->Outer.eDispSize)
					{
						default:
						case SIZE_WORD:
							sect_OutputExprWord(mode->Outer.pDisp);
							break;
						case SIZE_LONG:
							sect_OutputExprLong(mode->Outer.pDisp);
							break;
					}
				}

				return TRUE;
			}
			return FALSE;
		}

		case AM_POSTINDPCXD020:
		{
			if(mode->Inner.pDisp)
				mode->Inner.pDisp = parse_CreatePCRelExpr(mode->Inner.pDisp, 2);
		}
		// fall through
		case AM_POSTINDAXD020:
		{
			UWORD ins = 0x0100;
			SExpression* expr;

			if(mode->Inner.nBaseReg == -1)
			{
				ins |= 0x0080;
			}

			if(mode->Outer.nIndexReg == -1)
			{
				ins |= 0x0040;
			}
			else
			{
				ins |= mode->Outer.nIndexReg << 12;
				if(mode->Outer.nIndexReg == SIZE_LONG)
					ins |= 0x0800;
			}

			if(mode->Inner.pDisp != NULL)
			{
				parse_OptimizeDisp(&mode->Inner);
				switch(mode->Inner.eDispSize)
				{
					case SIZE_WORD:
						ins |= 0x0020;
						break;
					case SIZE_LONG:
						ins |= 0x0030;
						break;
					default:
						internalerror("unknown BD size");
						return FALSE;
				}
			}
			else
			{
				ins |= 0x0010;
			}

			if(mode->Outer.pDisp != NULL)
			{
				parse_OptimizeDisp(&mode->Outer);
				switch(mode->Outer.eDispSize)
				{
					case SIZE_WORD:
						ins |= 0x0006;
						break;
					case SIZE_LONG:
						ins |= 0x0007;
						break;
					default:
						internalerror("unknown OD size");
						return FALSE;
				}
			}
			else
			{
				ins |= 0x0005;
			}

			expr = parse_CreateConstExpr(ins);
			if(expr != NULL)
			{
				if(mode->Outer.pIndexScale != NULL)
				{
					expr = parse_CreateORExpr(expr, parse_CreateSHLExpr(mode->Outer.pIndexScale, parse_CreateConstExpr(9)));
				}
				sect_OutputExprWord(expr);

				if(mode->Inner.pDisp != NULL)
				{
					switch(mode->Inner.eDispSize)
					{
						default:
						case SIZE_WORD:
							sect_OutputExprWord(mode->Inner.pDisp);
							break;
						case SIZE_LONG:
							sect_OutputExprLong(mode->Inner.pDisp);
							break;
					}
				}

				if(mode->Outer.pDisp != NULL)
				{
					switch(mode->Outer.eDispSize)
					{
						default:
						case SIZE_WORD:
							sect_OutputExprWord(mode->Outer.pDisp);
							break;
						case SIZE_LONG:
							sect_OutputExprLong(mode->Outer.pDisp);
							break;
					}
				}

				return TRUE;
			}
			return FALSE;
		}

		default:
			internalerror("unsupported adressing mode");
			return FALSE;

	}

	return FALSE;
}

static int parse_GetEAField(SAddrMode* mode)
{
	switch(mode->eMode)
	{
		case AM_DREG:
			return mode->nDirectReg;
		case AM_AREG:
			return 0x1 << 3 | mode->nDirectReg;
		case AM_AIND:
			return 0x2 << 3 | mode->Outer.nBaseReg;
		case AM_AINC:
			return 0x3 << 3 | mode->Outer.nBaseReg;
		case AM_ADEC:
			return 0x4 << 3 | mode->Outer.nBaseReg;
		case AM_ADISP:
			return 0x5 << 3 | mode->Outer.nBaseReg;
		case AM_AXDISP:
			return 0x6 << 3 | mode->Outer.nBaseReg;
		case AM_WORD:
			return 0x7 << 3 | 0x0;
		case AM_LONG:
			return 0x7 << 3 | 0x1;
		case AM_IMM:
			return 0x7 << 3 | 0x4;
		case AM_PCDISP:
			return 0x7 << 3 | 0x2;
		case AM_PCXDISP:
			return 0x7 << 3 | 0x3;
		case AM_AXDISP020:
			if(mode->Outer.nBaseReg != -1)
				return 0x6 << 3 | mode->Outer.nBaseReg;
			else
				return 0x6 << 3 | 0;
		case AM_PCXDISP020:
		case AM_PREINDPCXD020:
		case AM_POSTINDPCXD020:
			return 0x7 << 3 | 0x3;
		case AM_PREINDAXD020:
		case AM_POSTINDAXD020:
			if(mode->Inner.nBaseReg != -1)
				return 0x6 << 3 | mode->Inner.nBaseReg;
			else
				return 0x6 << 3 | 0;
		default:
			internalerror("Unknown addressing mode");
	}

	return -1;
}


static int parse_GetSizeField(ESize sz)
{
	switch(sz)
	{
		case SIZE_BYTE:
			return 0x0;
		case SIZE_WORD:
			return 0x1;
		case SIZE_LONG:
			return 0x2;
		default:
			internalerror("Unknown size");
	}

	return -1;
}


static BOOL parse_SingleOpIns(UWORD ins, ESize sz, SAddrMode* src)
{
	// CLR, TST
	sect_OutputAbsWord((UWORD)(ins | parse_GetSizeField(sz) << 6 | parse_GetEAField(src)));
	parse_OutputExtWords(src);
	return TRUE;
}



static BOOL parse_xBCD(UWORD ins, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(src->eMode != dest->eMode)
		prj_Error(ERROR_OPERAND);

	if(src->eMode == AM_ADEC)
	{
		ins |= 0x0008;
		ins |= src->Outer.nBaseReg;
		ins |= dest->Outer.nBaseReg << 9;
	}
	else
	{
		ins |= src->nDirectReg;
		ins |= dest->nDirectReg << 9;
	}

	ins |= parse_GetSizeField(sz) << 6;

	sect_OutputAbsWord(ins);

	return TRUE;
}

static BOOL parse_ABCD(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_xBCD(0xC100, sz, src, dest);
}

static BOOL parse_SBCD(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_xBCD(0x8100, sz, src, dest);
}

static BOOL parse_ADDX(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_xBCD(0xD100, sz, src, dest);
}

static BOOL parse_SUBX(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_xBCD(0x9100, sz, src, dest);
}

static BOOL parse_xxxQ(UWORD ins, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	SExpression* expr;

	src->pImmediate = parse_CheckRange(src->pImmediate, 1, 8);
	if(src->pImmediate == NULL)
	{
		prj_Error(ERROR_OPERAND_RANGE);
		return TRUE;
	}

	ins |= (UWORD)(parse_GetEAField(dest) | (parse_GetSizeField(sz) << 6));

	expr = parse_CreateConstExpr(ins);
	expr = parse_CreateORExpr(expr, parse_CreateSHLExpr(parse_CreateANDExpr(src->pImmediate, parse_CreateConstExpr(7)), parse_CreateConstExpr(9)));

	sect_OutputExprWord(expr);
	return parse_OutputExtWords(dest);
}

static BOOL parse_ADDQ(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_xxxQ(0x5000, sz, src, dest);
}

static BOOL parse_SUBQ(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_xxxQ(0x5100, sz, src, dest);
}

static BOOL parse_ADDA(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	UWORD ins;

	if(src->eMode == AM_IMM
	&& src->pImmediate->Flags & EXPRF_isCONSTANT
	&& src->pImmediate->Value.Value >= 1
	&& src->pImmediate->Value.Value <= 8)
	{
		return parse_IntegerOp(T_68K_ADDQ, sz, src, dest);
	}

	ins = (UWORD)(0xD000 | dest->nDirectReg << 9 | parse_GetEAField(src));
	if(sz == SIZE_WORD)
		ins |= 0x3 << 6;
	else
		ins |= 0x7 << 6;

	sect_OutputAbsWord(ins);
	return parse_OutputExtWords(src);
}

static BOOL parse_SUBA(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	UWORD ins;

	if(src->eMode == AM_IMM
	&& src->pImmediate->Flags & EXPRF_isCONSTANT
	&& src->pImmediate->Value.Value >= 1
	&& src->pImmediate->Value.Value <= 8)
	{
		return parse_IntegerOp(T_68K_SUBQ, sz, src, dest);
	}

	ins = (UWORD)(0x9000 | dest->nDirectReg << 9 | parse_GetEAField(src));
	if(sz == SIZE_WORD)
		ins |= 0x3 << 6;
	else
		ins |= 0x7 << 6;

	sect_OutputAbsWord(ins);
	return parse_OutputExtWords(src);
}

static BOOL parse_ArithmeticLogicalI(UWORD ins, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	ins |= parse_GetEAField(dest);
	if(sz == SIZE_BYTE)
	{
		ins |= 0x0 << 6;
		sect_OutputAbsWord(ins);
		sect_OutputExprWord(parse_CreateANDExpr(parse_Check8bit(src->pImmediate), parse_CreateConstExpr(0xFF)));
	}
	else if(sz == SIZE_WORD)
	{
		ins |= 0x1 << 6;
		sect_OutputAbsWord(ins);
		sect_OutputExprWord(parse_Check16bit(src->pImmediate));
	}
	else
	{
		ins |= 0x2 << 6;
		sect_OutputAbsWord(ins);
		sect_OutputExprLong(src->pImmediate);
	}

	return parse_OutputExtWords(dest);
}

static BOOL parse_ADDI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(src->eMode == AM_IMM
	&& src->pImmediate->Flags & EXPRF_isCONSTANT
	&& src->pImmediate->Value.Value >= 1
	&& src->pImmediate->Value.Value <= 8)
	{
		return parse_IntegerOp(T_68K_ADDQ, sz, src, dest);
	}

	return parse_ArithmeticLogicalI(0x0600, sz, src, dest);
}

static BOOL parse_SUBI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(src->eMode == AM_IMM
	&& src->pImmediate->Flags & EXPRF_isCONSTANT
	&& src->pImmediate->Value.Value >= 1
	&& src->pImmediate->Value.Value <= 8)
	{
		return parse_IntegerOp(T_68K_SUBQ, sz, src, dest);
	}

	return parse_ArithmeticLogicalI(0x0400, sz, src, dest);
}

static BOOL parse_ANDI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(dest->eMode == AM_SYSREG)
	{
		if(sz != SIZE_WORD)
		{
			prj_Error(MERROR_INSTRUCTION_SIZE);
			return TRUE;
		}

		if(dest->nDirectReg == T_68K_REG_CCR)
		{
			sect_OutputAbsWord(0x023C);
			sect_OutputExprWord(parse_CreateANDExpr(src->pImmediate, parse_CreateConstExpr(0xFF)));
			return TRUE;
		}
		else if(dest->nDirectReg == T_68K_REG_SR)
		{
			prj_Warn(MERROR_INSTRUCTION_PRIV);
			sect_OutputAbsWord(0x027C);
			sect_OutputExprWord(src->pImmediate);
			return TRUE;
		}
		prj_Error(ERROR_DEST_OPERAND);
		return TRUE;
	}

	return parse_ArithmeticLogicalI(0x0200, sz, src, dest);
}

static BOOL parse_ArithmeticLogical(UWORD ins, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(dest->eMode == AM_DREG)
	{
		if(src->eMode == AM_AREG
		&& sz != SIZE_WORD
		&& sz != SIZE_LONG)
		{
			prj_Error(MERROR_INSTRUCTION_SIZE);
			return TRUE;
		}

		ins |= (UWORD)(0x0000 | dest->nDirectReg << 9 | parse_GetSizeField(sz) << 6);
		ins |= parse_GetEAField(src);
		sect_OutputAbsWord(ins);
		return parse_OutputExtWords(src);
	}
	else if(src->eMode == AM_DREG)
	{
		ins |= (UWORD)(0x0100 | src->nDirectReg << 9 | parse_GetSizeField(sz) << 6);
		ins |= parse_GetEAField(dest);
		sect_OutputAbsWord(ins);
		return parse_OutputExtWords(dest);
	}

	prj_Error(ERROR_OPERAND);
	return TRUE;
}

static BOOL parse_ADD(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(dest->eMode == AM_AREG)
		return parse_IntegerOp(T_68K_ADDA, sz, src, dest);

	if(src->eMode == AM_IMM)
		return parse_IntegerOp(T_68K_ADDI, sz, src, dest);

	return parse_ArithmeticLogical(0xD000, sz, src, dest);
}

static BOOL parse_SUB(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(dest->eMode == AM_AREG)
		return parse_IntegerOp(T_68K_SUBA, sz, src, dest);

	if(src->eMode == AM_IMM)
		return parse_IntegerOp(T_68K_SUBI, sz, src, dest);

	return parse_ArithmeticLogical(0x9000, sz, src, dest);
}

static BOOL parse_CMPA(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	UWORD ins;

	ins = (UWORD)(0xB040 | dest->nDirectReg << 9 | parse_GetEAField(src));
	if(sz == SIZE_WORD)
		ins |= 0x3 << 6;
	else /*if(sz == SIZE_LONG)*/
		ins |= 0x7 << 6;

	sect_OutputAbsWord(ins);
	return parse_OutputExtWords(src);
}

static BOOL parse_CMPI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	UWORD ins;

	ins = (UWORD)(0x0C00 | parse_GetSizeField(sz) << 6 | parse_GetEAField(dest));
	sect_OutputAbsWord(ins);
	
	if(sz == SIZE_BYTE)
	{
		SExpression* expr = parse_Check8bit(src->pImmediate);
		if(expr == NULL)
		{
			prj_Error(ERROR_OPERAND_RANGE);
			return TRUE;
		}
		sect_OutputExprWord(parse_CreateANDExpr(expr, parse_CreateConstExpr(0xFF)));
	}
	else if(sz == SIZE_WORD)
	{
		SExpression* expr = parse_Check16bit(src->pImmediate);
		if(expr == NULL)
		{
			prj_Error(ERROR_OPERAND_RANGE);
			return TRUE;
		}
		sect_OutputExprWord(expr);
	}
	else if(sz == SIZE_WORD)
	{
		sect_OutputExprLong(src->pImmediate);
	}
	return parse_OutputExtWords(dest);
}

static BOOL parse_CMPM(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	UWORD ins;

	ins = (UWORD)(0xB108 | dest->Outer.nBaseReg << 9 | src->Outer.nBaseReg | parse_GetSizeField(sz) << 6);
	sect_OutputAbsWord(ins);
	return TRUE;
}

static BOOL parse_CMP(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(src->eMode == AM_AINC && dest->eMode == AM_AINC)
		return parse_IntegerOp(T_68K_CMPM, sz, src, dest);

	if(dest->eMode == AM_AREG)
		return parse_IntegerOp(T_68K_CMPA, sz, src, dest);

	if(src->eMode == AM_IMM)
		return parse_IntegerOp(T_68K_CMPI, sz, src, dest);

	if(dest->eMode == AM_DREG)
		return parse_ArithmeticLogical(0xB000, sz, src, dest);

	prj_Fail(ERROR_DEST_OPERAND);
	return FALSE;
}

static BOOL parse_AND(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(src->eMode == AM_IMM || dest->eMode == AM_SYSREG)
		return parse_IntegerOp(T_68K_ANDI, sz, src, dest);

	return parse_ArithmeticLogical(0xC000, sz, src, dest);
}

static BOOL parse_CLR(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_SingleOpIns(0x4200, sz, src);
}

static BOOL parse_TST(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(src->eMode == AM_AREG
	&& sz == SIZE_BYTE)
	{
		prj_Error(MERROR_INSTRUCTION_SIZE);
		return TRUE;
	}

	return parse_SingleOpIns(0x4A00, sz, src);
}

static BOOL parse_Shift(UWORD ins, UWORD memins, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(dest->eMode == AM_DREG)
	{
		ins |= 0x0000 | parse_GetSizeField(sz) << 6 | dest->nDirectReg;
		if(src->eMode == AM_IMM)
		{
			SExpression* expr;
			expr = parse_CheckRange(src->pImmediate, 1, 8);
			expr = parse_CreateANDExpr(expr, parse_CreateConstExpr(7));
			if(expr == NULL)
			{
				prj_Error(ERROR_OPERAND_RANGE);
				return TRUE;
			}
			expr = parse_CreateORExpr(parse_CreateConstExpr(ins), parse_CreateSHLExpr(expr, parse_CreateConstExpr(9)));
			sect_OutputExprWord(expr);
		}
		else if(src->eMode == AM_DREG)
		{
			ins |= 0x0020 | src->nDirectReg << 9;
			sect_OutputAbsWord(ins);
		}
		return TRUE;
	}
	if(dest->eMode != AM_EMPTY)
	{
		prj_Error(ERROR_DEST_OPERAND);
		return TRUE;
	}
	if(src->eMode & (AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_POSTINDAXD020 | AM_PREINDAXD020))
	{
		if(sz != SIZE_WORD)
		{
			prj_Error(MERROR_INSTRUCTION_SIZE);
			return TRUE;
		}

		memins |= parse_GetEAField(src);
		sect_OutputAbsWord(memins);
		return parse_OutputExtWords(src);
	}

	prj_Error(ERROR_SOURCE_OPERAND);
	return TRUE;
}

static BOOL parse_ASL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Shift(0xE100, 0xE1C0, sz, src, dest);
}

static BOOL parse_ASR(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Shift(0xE000, 0xE0C0, sz, src, dest);
}

static BOOL parse_LSL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Shift(0xE108, 0xE3C0, sz, src, dest);
}

static BOOL parse_LSR(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Shift(0xE008, 0xE2C0, sz, src, dest);
}

static BOOL parse_ROL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Shift(0xE118, 0xE7C0, sz, src, dest);
}

static BOOL parse_ROR(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Shift(0xE018, 0xE6C0, sz, src, dest);
}

static BOOL parse_ROXL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Shift(0xE110, 0xE5C0, sz, src, dest);
}

static BOOL parse_ROXR(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Shift(0xE010, 0xE4C0, sz, src, dest);
}

static BOOL parse_Bcc(UWORD ins, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	ins = 0x6000 | ins << 8;
	if(sz == SIZE_BYTE)
	{
		SExpression* expr = parse_CheckRange(parse_CreatePCRelExpr(src->Outer.pDisp, -2), -128, 127);
		if(expr != NULL)
		{
			expr = parse_CreateANDExpr(expr, parse_CreateConstExpr(0xFF));
			expr = parse_CreateORExpr(expr, parse_CreateConstExpr(ins));
			sect_OutputExprWord(expr);
			return TRUE;
		}

		prj_Error(ERROR_OPERAND_RANGE);
		return TRUE;
	}
	else if(sz == SIZE_WORD)
	{
		SExpression* expr = parse_CheckRange(parse_CreatePCRelExpr(src->Outer.pDisp, 0), -32768, 32767);
		if(expr != NULL)
		{
			sect_OutputAbsWord(ins);
			sect_OutputExprWord(expr);
			return TRUE;
		}

		prj_Error(ERROR_OPERAND_RANGE);
		return TRUE;
	}
	else if(sz == SIZE_LONG)
	{
		SExpression* expr;

		if(g_pOptions->pMachine->nCpu < CPUF_68020)
		{
			prj_Error(MERROR_INSTRUCTION_SIZE);
			return TRUE;
		}

		expr = parse_CreatePCRelExpr(src->Outer.pDisp, 0);
		sect_OutputAbsWord(ins | 0xFF);
		sect_OutputExprLong(expr);
		return TRUE;
	}


	return TRUE;
}

static BOOL parse_BRA(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0x0, sz, src, dest);
}

static BOOL parse_BSR(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0x1, sz, src, dest);
}

static BOOL parse_BHI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0x2, sz, src, dest);
}

static BOOL parse_BLS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0x3, sz, src, dest);
}

static BOOL parse_BCC(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0x4, sz, src, dest);
}

static BOOL parse_BCS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0x5, sz, src, dest);
}

static BOOL parse_BNE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0x6, sz, src, dest);
}

static BOOL parse_BEQ(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0x7, sz, src, dest);
}

static BOOL parse_BVC(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0x8, sz, src, dest);
}

static BOOL parse_BVS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0x9, sz, src, dest);
}

static BOOL parse_BPL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0xA, sz, src, dest);
}

static BOOL parse_BMI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0xB, sz, src, dest);
}

static BOOL parse_BGE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0xC, sz, src, dest);
}

static BOOL parse_BLT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0xD, sz, src, dest);
}

static BOOL parse_BGT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0xE, sz, src, dest);
}

static BOOL parse_BLE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0xF, sz, src, dest);
}

static BOOL parse_BitInstruction(UWORD dins, UWORD immins, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(src->eMode == AM_DREG)
	{
		dins |= src->nDirectReg << 9 | parse_GetEAField(dest);
		sect_OutputAbsWord(dins);
		return parse_OutputExtWords(dest);
	}
	else if(src->eMode == AM_IMM)
	{
		SExpression* expr;

		immins |= parse_GetEAField(dest);
		sect_OutputAbsWord(immins);

		if(dest->eMode == AM_DREG)
			expr = parse_CheckRange(src->pImmediate, 0, 31);
		else
			expr = parse_CheckRange(src->pImmediate, 0, 7);

		if(expr != NULL)
		{
			sect_OutputExprWord(expr);
			return parse_OutputExtWords(dest);
		}
		prj_Error(ERROR_OPERAND_RANGE);
	}
	return TRUE;
}

static BOOL parse_BCHG(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_BitInstruction(0x0140, 0x0840, sz, src, dest);
}

static BOOL parse_BCLR(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_BitInstruction(0x0180, 0x0880, sz, src, dest);
}

static BOOL parse_BSET(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_BitInstruction(0x01C0, 0x08C0, sz, src, dest);
}

static BOOL parse_BTST(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_BitInstruction(0x0100, 0x0800, sz, src, dest);
}

static BOOL parse_BitfieldInstruction(UWORD ins, UWORD ext, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	SExpression* expr = parse_CreateConstExpr(ext);

	ins |= parse_GetEAField(src);
	sect_OutputAbsWord(ins);

	if(src->nBFOffsetReg != -1)
		expr = parse_CreateORExpr(expr, parse_CreateConstExpr(0x0800 | src->nBFOffsetReg << 6));
	else
	{
		SExpression* bf = parse_CheckRange(src->pBFOffsetExpr, 0, 31);
		if(bf == NULL)
		{
			prj_Error(ERROR_OPERAND_RANGE);
			return TRUE;
		}
		expr = parse_CreateORExpr(expr, parse_CreateSHLExpr(bf, parse_CreateConstExpr(6)));
	}

	if(src->nBFWidthReg != -1)
		expr = parse_CreateORExpr(expr, parse_CreateConstExpr(0x0020 | src->nBFWidthReg));
	else
	{
		SExpression* bf = parse_CheckRange(src->pBFWidthExpr, 0, 31);
		if(bf == NULL)
		{
			prj_Error(ERROR_OPERAND_RANGE);
			return TRUE;
		}
		expr = parse_CreateORExpr(expr, bf);
	}

	sect_OutputExprWord(expr);
	return parse_OutputExtWords(src);
}

static BOOL parse_SingleOpBitfieldInstruction(UWORD ins, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_BitfieldInstruction(ins, 0, sz, src, dest);
}

static BOOL parse_BFCHG(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_SingleOpBitfieldInstruction(0xEAC0, sz, src, dest);
}

static BOOL parse_BFCLR(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_SingleOpBitfieldInstruction(0xECC0, sz, src, dest);
}

static BOOL parse_BFSET(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_SingleOpBitfieldInstruction(0xEEC0, sz, src, dest);
}

static BOOL parse_BFTST(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_SingleOpBitfieldInstruction(0xE8C0, sz, src, dest);
}

static BOOL parse_BFEXTS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_BitfieldInstruction(0xEBC0, (UWORD)(dest->nDirectReg << 12), sz, src, dest);
}

static BOOL parse_BFEXTU(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_BitfieldInstruction(0xE9C0, (UWORD)(dest->nDirectReg << 12), sz, src, dest);
}

static BOOL parse_BFFFO(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_BitfieldInstruction(0xEDC0, (UWORD)(dest->nDirectReg << 12), sz, src, dest);
}

static BOOL parse_BFINS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_BitfieldInstruction(0xEFC0, (UWORD)(src->nDirectReg << 12), sz, dest, src);
}

static BOOL parse_BKPT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	SExpression* expr = parse_CheckRange(src->pImmediate, 0, 7);
	if(expr == NULL)
	{
		prj_Error(ERROR_OPERAND_RANGE);
		return TRUE;
	}

	expr = parse_CreateORExpr(expr, parse_CreateConstExpr(0x4848));
	sect_OutputExprWord(expr);
	return TRUE;
}

static BOOL parse_CALLM(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	SExpression* expr = parse_CheckRange(src->pImmediate, 0, 255);
	if(expr == NULL)
	{
		prj_Error(ERROR_OPERAND_RANGE);
		return TRUE;
	}

	sect_OutputAbsWord(0x06C0 | (UWORD)parse_GetEAField(dest));
	sect_OutputExprWord(expr);
	return parse_OutputExtWords(dest);
}

static BOOL parse_CAS(ESize sz, SAddrMode* dc, SAddrMode* du)
{
	UWORD ins;
	SAddrMode ea;

	if(g_pOptions->pMachine->nCpu == CPUF_68060
	&& sz != SIZE_BYTE)
	{
		prj_Error(MERROR_INSTRUCTION_CPU);
		return TRUE;
	}

	if(!parse_ExpectComma())
		return FALSE;
	if(!parse_GetAddrMode(&ea))
		return FALSE;

	if((ea.eMode & (AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020)) == 0)
	{
		prj_Error(ERROR_OPERAND);
		return TRUE;
	}

	ins = (UWORD)(0x08C0 | (parse_GetSizeField(sz) + 1) << 9 | parse_GetEAField(&ea));
	sect_OutputAbsWord(ins);

	ins = (UWORD)(0x0000 | du->nDirectReg << 6 | dc->nDirectReg);
	sect_OutputAbsWord(ins);

	return parse_OutputExtWords(&ea);
}

static BOOL parse_GetDataRegister(UWORD* pReg)
{
	if(g_CurrentToken.ID.TargetToken >= T_68K_REG_D0
	&& g_CurrentToken.ID.TargetToken <= T_68K_REG_D7)
	{
		*pReg = (UWORD)(g_CurrentToken.ID.TargetToken - T_68K_REG_D0);
		parse_GetToken();
		return TRUE;
	}

	return FALSE;
}

static BOOL parse_GetAddressRegister(UWORD* pReg)
{
	if(g_CurrentToken.ID.TargetToken >= T_68K_REG_A0
	&& g_CurrentToken.ID.TargetToken <= T_68K_REG_A7)
	{
		*pReg = (UWORD)(g_CurrentToken.ID.TargetToken - T_68K_REG_A0);
		parse_GetToken();
		return TRUE;
	}

	return FALSE;
}

static BOOL parse_GetRegister(UWORD* pReg)
{
	if(parse_GetDataRegister(pReg))
		return TRUE;

	if(parse_GetAddressRegister(pReg))
	{
		*pReg += 8;
		return TRUE;
	}

	return FALSE;
}

static BOOL parse_ExpectDataRegister(UWORD* pReg)
{
	if(g_CurrentToken.ID.TargetToken >= T_68K_REG_D0
	&& g_CurrentToken.ID.TargetToken <= T_68K_REG_D7)
	{
		*pReg = (UWORD)(g_CurrentToken.ID.TargetToken - T_68K_REG_D0);
		parse_GetToken();
		return TRUE;
	}

	prj_Error(ERROR_OPERAND);
	return FALSE;
}

static BOOL parse_ExpectIndirectRegister(UWORD* pReg)
{
	if(g_CurrentToken.ID.TargetToken >= T_68K_REG_A0_IND
	&& g_CurrentToken.ID.TargetToken <= T_68K_REG_A7_IND)
	{
		*pReg = (UWORD)(g_CurrentToken.ID.TargetToken - T_68K_REG_A0_IND + 8);
		parse_GetToken();
		return TRUE;
	}

	if(!parse_ExpectChar('('))
		return FALSE;

	if(!parse_ExpectDataRegister(pReg))
		return FALSE;

	if(!parse_ExpectChar(')'))
		return FALSE;

	return TRUE;
}

static BOOL parse_CAS2(ESize sz, SAddrMode* unused1, SAddrMode* unused2)
{
	UWORD dc1, dc2, du1, du2, rn1, rn2;

	if(!parse_ExpectDataRegister(&dc1))
		return FALSE;

	if(!parse_ExpectChar(':'))
		return FALSE;

	if(!parse_ExpectDataRegister(&dc2))
		return FALSE;

	if(!parse_ExpectComma())
		return FALSE;

	if(!parse_ExpectDataRegister(&du1))
		return FALSE;

	if(!parse_ExpectChar(':'))
		return FALSE;

	if(!parse_ExpectDataRegister(&du2))
		return FALSE;

	if(!parse_ExpectComma())
		return FALSE;

	if(!parse_ExpectIndirectRegister(&rn1))
		return FALSE;

	if(!parse_ExpectChar(':'))
		return FALSE;

	if(!parse_ExpectIndirectRegister(&rn2))
		return FALSE;

	sect_OutputAbsWord(0x08FC | (UWORD)(parse_GetSizeField(sz) + 1) << 9);
	sect_OutputAbsWord(rn1 << 12 | du1 << 6 | dc1);
	sect_OutputAbsWord(rn2 << 12 | du2 << 6 | dc2);

	return TRUE;
}

static BOOL parse_CHK(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	UWORD ins;

	if(sz == SIZE_LONG
	&& g_pOptions->pMachine->nCpu < CPUF_68020)
	{
		prj_Error(MERROR_INSTRUCTION_SIZE);
		return TRUE;
	}

	ins = (UWORD)(0x4000 | dest->nDirectReg << 9 | parse_GetEAField(src));
	if(sz == SIZE_WORD)
		ins |= 0x3 << 7;
	else /*if(sz == SIZE_LONG)*/
		ins |= 0x2 << 7;

	sect_OutputAbsWord(ins);
	return parse_OutputExtWords(src);
}

static BOOL parse_CHK2(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	UWORD ins;

	ins = (UWORD)(0x00C0 | parse_GetSizeField(sz) << 9 | parse_GetEAField(src));
	sect_OutputAbsWord(ins);

	ins = (UWORD)(0x0800 | dest->nDirectReg << 12);
	sect_OutputAbsWord(ins);

	return parse_OutputExtWords(src);
}

static BOOL parse_CMP2(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	UWORD ins;
	
	ins = (UWORD)(0x00C0 | parse_GetSizeField(sz) << 9 | parse_GetEAField(src));
	sect_OutputAbsWord(ins);

	ins = (UWORD)(dest->nDirectReg << 12);
	if(dest->eMode == AM_AREG)
		ins |= 0x8000;
	sect_OutputAbsWord(ins);
	return TRUE;
}

static BOOL parse_DBcc(UWORD code, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	code = (UWORD)(0x50C8 | code << 8 | src->nDirectReg);
	sect_OutputAbsWord(code);
	sect_OutputExprWord(parse_CreatePCRelExpr(dest->Outer.pDisp, 0));
	return TRUE;
}

static BOOL parse_DBT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0x0, sz, src, dest);
}

static BOOL parse_DBF(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0x1, sz, src, dest);
}

static BOOL parse_DBHI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0x2, sz, src, dest);
}

static BOOL parse_DBLS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0x3, sz, src, dest);
}

static BOOL parse_DBCC(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0x4, sz, src, dest);
}

static BOOL parse_DBCS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0x5, sz, src, dest);
}

static BOOL parse_DBNE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0x6, sz, src, dest);
}

static BOOL parse_DBEQ(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0x7, sz, src, dest);
}

static BOOL parse_DBVC(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0x8, sz, src, dest);
}

static BOOL parse_DBVS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0x9, sz, src, dest);
}

static BOOL parse_DBPL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0xA, sz, src, dest);
}

static BOOL parse_DBMI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0xB, sz, src, dest);
}

static BOOL parse_DBGE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0xC, sz, src, dest);
}

static BOOL parse_DBLT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0xD, sz, src, dest);
}

static BOOL parse_DBGT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0xE, sz, src, dest);
}

static BOOL parse_DBLE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0xF, sz, src, dest);
}

static BOOL parse_DIVxx(BOOL sign, BOOL l, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(l || sz == SIZE_LONG)
	{
		BOOL div64;
		int dq, dr;
		if(g_CurrentToken.ID.TargetToken == ':')
		{
			UWORD reg;
			parse_GetToken();
			if(!parse_ExpectDataRegister(&reg))
				return FALSE;

			dq = reg;
			dr = dest->nDirectReg;
			div64 = !l;
		}
		else
		{
			dq = dest->nDirectReg;
			dr = dq;
			div64 = FALSE;
		}

		if(g_pOptions->pMachine->nCpu >= CPUF_68060
		&& div64)
		{
			prj_Error(MERROR_INSTRUCTION_CPU);
			return TRUE;
		}

		sect_OutputAbsWord((UWORD)(0x4C40 | parse_GetEAField(src)));
		sect_OutputAbsWord((UWORD)(sign << 11 | div64 << 10 | dq << 12 | dr));
		return parse_OutputExtWords(src);
	}
	else
	{
		UWORD ins = (UWORD)(0x80C0 | sign << 8 | dest->nDirectReg << 9 | parse_GetEAField(src));
		sect_OutputAbsWord(ins);
		return parse_OutputExtWords(src);
	}
}

static BOOL parse_DIVS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DIVxx(TRUE, FALSE, sz, src, dest);
}

static BOOL parse_DIVSL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DIVxx(TRUE, TRUE, sz, src, dest);
}

static BOOL parse_DIVU(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DIVxx(FALSE, FALSE, sz, src, dest);
}

static BOOL parse_DIVUL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DIVxx(FALSE, TRUE, sz, src, dest);
}

static BOOL parse_EOR(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	UWORD ins;

	if(src->eMode == AM_IMM || dest->eMode == AM_SYSREG)
		return parse_IntegerOp(T_68K_EORI, sz, src, dest);

	ins = (UWORD)(0xB100 | src->nDirectReg << 9 | parse_GetEAField(dest) | parse_GetSizeField(sz) << 6);
	sect_OutputAbsWord(ins);
	return parse_OutputExtWords(dest);
}

static BOOL parse_EORI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(dest->eMode == AM_SYSREG)
	{
		if(sz != SIZE_WORD)
		{
			prj_Error(MERROR_INSTRUCTION_SIZE);
			return TRUE;
		}

		if(dest->nDirectReg == T_68K_REG_CCR)
		{
			sect_OutputAbsWord(0x0A3C);
			sect_OutputExprWord(parse_CreateANDExpr(src->pImmediate, parse_CreateConstExpr(0xFF)));
			return TRUE;
		}
		else if(dest->nDirectReg == T_68K_REG_SR)
		{
			prj_Warn(MERROR_INSTRUCTION_PRIV);
			sect_OutputAbsWord(0x0A7C);
			sect_OutputExprWord(src->pImmediate);
			return TRUE;
		}

		prj_Error(ERROR_DEST_OPERAND);
		return TRUE;
	}

	return parse_ArithmeticLogicalI(0x0A00, sz, src, dest);
}

static BOOL parse_EXG(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	UWORD ins;
	UWORD rx, ry;

	if(src->eMode != dest->eMode)
	{
		ins = 0x11 << 3;
		if(src->eMode == AM_AREG)
		{
			rx = (UWORD)dest->nDirectReg;
			ry = (UWORD)src->nDirectReg;
		}
		else
		{
			rx = (UWORD)src->nDirectReg;
			ry = (UWORD)dest->nDirectReg;
		}
	}
	else
	{
		rx = (UWORD)src->nDirectReg;
		ry = (UWORD)dest->nDirectReg;
		if(src->eMode == AM_DREG)
			ins = 0x08 << 3;
		else
			ins = 0x09 << 3;
	}

	sect_OutputAbsWord(ins | 0xC100 | rx << 9 | ry);
	return TRUE;
}

static BOOL parse_EXT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	UWORD ins = (UWORD)(0x4800 | src->nDirectReg);
	if(sz == SIZE_WORD)
		ins |= 0x2 << 6;
	else
		ins |= 0x3 << 6;

	sect_OutputAbsWord(ins);
	return TRUE;
}

static BOOL parse_EXTB(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputAbsWord((UWORD)(0x4800 | src->nDirectReg | 0x7 << 6));
	return TRUE;
}

static BOOL parse_ILLEGAL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputAbsWord(0x4AFC);
	return TRUE;
}

static BOOL parse_Jxx(UWORD ins, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	ins |= parse_GetEAField(src);
	sect_OutputAbsWord(ins);
	return parse_OutputExtWords(src);
}

static BOOL parse_JMP(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Jxx(0x4EC0, sz, src, dest);
}

static BOOL parse_JSR(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Jxx(0x4E80, sz, src, dest);
}

static BOOL parse_LEA(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputAbsWord((UWORD)(0x41C0 | dest->nDirectReg << 9 | parse_GetEAField(src)));
	return parse_OutputExtWords(src);
}

static BOOL parse_LINK(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(sz == SIZE_LONG
	&& g_pOptions->pMachine->nCpu < CPUF_68020)
	{
		prj_Error(MERROR_INSTRUCTION_SIZE);
		return TRUE;
	}

	if(sz == SIZE_WORD)
	{
		sect_OutputAbsWord((UWORD)(0x4E50 | src->nDirectReg));
		sect_OutputExprWord(dest->pImmediate);
		return TRUE;
	}
	else /*if(sz == SIZE_LONG)*/
	{
		sect_OutputAbsWord((UWORD)(0x4808 | src->nDirectReg));
		sect_OutputExprLong(dest->pImmediate);
		return TRUE;
	}
}

static BOOL parse_MOVEfromSYSREG(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(src->nDirectReg == T_68K_REG_USP)
	{
		prj_Warn(MERROR_INSTRUCTION_PRIV);

		if(sz != SIZE_LONG)
		{
			prj_Error(MERROR_INSTRUCTION_SIZE);
			return TRUE;
		}

		if(dest->eMode != AM_AREG)
		{
			prj_Error(ERROR_DEST_OPERAND);
			return TRUE;
		}

		sect_OutputAbsWord((UWORD)(0x4E68 | dest->nDirectReg));
		return TRUE;
	}
	else
	{
		EAddrMode allow;

		allow = AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG;
		if(g_pOptions->pMachine->nCpu >= CPUF_68020)
			allow |= AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020;

		if((dest->eMode & allow) == 0)
		{
			prj_Error(ERROR_DEST_OPERAND);
			return TRUE;
		}

		if(src->nDirectReg == T_68K_REG_CCR)
		{
			if(g_pOptions->pMachine->nCpu < CPUF_68010)
			{
				prj_Error(MERROR_INSTRUCTION_CPU);
				return TRUE;
			}

			if(sz != SIZE_WORD)
			{
				prj_Error(MERROR_INSTRUCTION_SIZE);
				return TRUE;
			}

			sect_OutputAbsWord((UWORD)(0x42C0 | parse_GetEAField(dest)));
			return TRUE;
		}
		else if(src->nDirectReg == T_68K_REG_SR)
		{
			if(g_pOptions->pMachine->nCpu >= CPUF_68010)
				prj_Warn(MERROR_INSTRUCTION_PRIV);

			if(sz != SIZE_WORD)
			{
				prj_Error(MERROR_INSTRUCTION_SIZE);
				return TRUE;
			}

			sect_OutputAbsWord((UWORD)(0x40C0 | parse_GetEAField(dest)));
			return TRUE;
		}
	}

	return TRUE;
}

static BOOL parse_MOVEtoSYSREG(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(dest->nDirectReg == T_68K_REG_USP)
	{
		prj_Warn(MERROR_INSTRUCTION_PRIV);

		if(sz != SIZE_LONG)
		{
			prj_Error(MERROR_INSTRUCTION_SIZE);
			return TRUE;
		}

		if(src->eMode != AM_AREG)
		{
			prj_Error(ERROR_SOURCE_OPERAND);
			return TRUE;
		}

		sect_OutputAbsWord((UWORD)(0x4E60 | src->nDirectReg));
		return TRUE;
	}
	else
	{
		EAddrMode allow;

		allow = AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP;
		if(g_pOptions->pMachine->nCpu >= CPUF_68020)
			allow |= AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020;

		if((src->eMode & allow) == 0)
		{
			prj_Error(ERROR_SOURCE_OPERAND);
			return TRUE;
		}

		if(dest->nDirectReg == T_68K_REG_CCR)
		{
			if(sz != SIZE_WORD)
			{
				prj_Error(MERROR_INSTRUCTION_SIZE);
				return TRUE;
			}

			sect_OutputAbsWord((UWORD)(0x44C0 | parse_GetEAField(src)));
			return TRUE;
		}
		else if(dest->nDirectReg == T_68K_REG_SR)
		{
			if(sz != SIZE_WORD)
			{
				prj_Error(MERROR_INSTRUCTION_SIZE);
				return TRUE;
			}

			prj_Warn(MERROR_INSTRUCTION_PRIV);

			sect_OutputAbsWord((UWORD)(0x46C0 | parse_GetEAField(src)));
			return TRUE;
		}
	}

	return TRUE;
}

		

static BOOL parse_MOVE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	UWORD destea;
	UWORD ins;

	if(src->eMode == AM_IMM
	&& dest->eMode == AM_DREG && sz == SIZE_LONG
	&& (src->pImmediate->Flags & EXPRF_isCONSTANT)
	&& src->pImmediate->Value.Value >= -128
	&& src->pImmediate->Value.Value < 127)
	{
		return parse_IntegerOp(T_68K_MOVEQ, sz, src, dest);
	}

	if(src->eMode == AM_SYSREG)
		return parse_MOVEfromSYSREG(sz, src, dest);

	if(dest->eMode == AM_SYSREG)
		return parse_MOVEtoSYSREG(sz, src, dest);

	if(dest->eMode == AM_AREG)
		return parse_IntegerOp(T_68K_MOVEA, sz, src, dest);

	destea = (UWORD)parse_GetEAField(dest);
	ins = (UWORD)parse_GetEAField(src);

	destea = (destea >> 3 | destea << 3) & 0x3F;

	ins |= destea << 6;
	if(sz == SIZE_BYTE)
		ins |= 0x1 << 12;
	else if(sz == SIZE_WORD)
		ins |= 0x3 << 12;
	else /*if(sz == SIZE_LONG)*/
		ins |= 0x2 << 12;

	sect_OutputAbsWord(ins);
	if(!parse_OutputExtWords(src))
		return FALSE;
	return parse_OutputExtWords(dest);
}

static BOOL parse_MOVEA(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	UWORD ins = (UWORD)(0x0040 | parse_GetEAField(src) | dest->nDirectReg << 9);

	if(sz == SIZE_WORD)
		ins |= 0x3 << 12;
	else /*if(sz == SIZE_LONG)*/
		ins |= 0x2 << 12;

	sect_OutputAbsWord(ins);
	return parse_OutputExtWords(src);
}

static BOOL parse_MOVE16(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	UWORD opmode;
	SExpression *line;
	UWORD reg;

	if(src->eMode == AM_AINC && dest->eMode == AM_AINC)
	{
		sect_OutputAbsWord((UWORD)(0xF620 | src->Outer.nBaseReg));
		sect_OutputAbsWord((UWORD)(0x8000 | dest->Outer.nBaseReg << 12));
		return TRUE;
	}

	if(src->eMode == AM_AINC && dest->eMode == AM_LONG)
	{
		opmode = 0x0;
		line = dest->Outer.pDisp;
		reg = (UWORD)src->Outer.nBaseReg;
	}
	else if(src->eMode == AM_LONG && dest->eMode == AM_AINC)
	{
		opmode = 0x1;
		line = src->Outer.pDisp;
		reg = (UWORD)dest->Outer.nBaseReg;
	}
	else if(src->eMode == AM_AIND && dest->eMode == AM_LONG)
	{
		opmode = 0x2;
		line = dest->Outer.pDisp;
		reg = (UWORD)src->Outer.nBaseReg;
	}
	else if(src->eMode == AM_LONG && dest->eMode == AM_AIND)
	{
		opmode = 0x3;
		line = src->Outer.pDisp;
		reg = (UWORD)dest->Outer.nBaseReg;
	}
	else
	{
		prj_Error(ERROR_OPERAND);
		return TRUE;
	}

	sect_OutputAbsWord(0xF600 | opmode << 3 | reg);
	sect_OutputExprLong(line);
	return TRUE;
}

static BOOL parse_GetRegisterRange(UWORD* pStart, UWORD* pEnd)
{
	if(parse_GetRegister(pStart))
	{
		if(g_CurrentToken.ID.TargetToken == T_OP_SUB)
		{
			parse_GetToken();
			if(!parse_GetRegister(pEnd))
				return 0;
			return TRUE;
		}
		*pEnd = *pStart;
		return TRUE;
	}
	return FALSE;
}

#define REGLIST_FAIL 65536

static ULONG parse_RegisterList(void)
{
	UWORD r;
	UWORD start;
	UWORD end;


	if(g_CurrentToken.ID.TargetToken == '#')
	{
		SLONG expr;
		parse_GetToken();
		expr = parse_ConstantExpression();
		if(expr >= 0 && expr <= 65535)
			return (UWORD)expr;

		return REGLIST_FAIL;
	}

	r = 0;

	while(parse_GetRegisterRange(&start, &end))
	{
		if(start > end)
		{
			prj_Error(ERROR_OPERAND);
			return REGLIST_FAIL;
		}

		while(start <= end)
			r |= 1 << start++;

		if(g_CurrentToken.ID.TargetToken != T_OP_DIV)
			return r;

		parse_GetToken();
	}

	return REGLIST_FAIL;
}

static UWORD parse_SwapBits(UWORD bits)
{
	UWORD r = 0;
	int i;

	for(i = 0; i < 16; ++i)
		r |= (bits & 1 << i) ? 1 << (15 - i) : 0;

	return r;
}

static BOOL parse_MOVEM(ESize sz, SAddrMode* unused1, SAddrMode* unused2)
{
	UWORD ins;
	UWORD dr;
	ULONG reglist;
	SAddrMode mode;

	reglist = parse_RegisterList();
	if(reglist != REGLIST_FAIL)
	{
		EAddrMode allowdest = AM_AIND | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG;

		if(!parse_ExpectComma())
			return FALSE;

		if(!parse_GetAddrMode(&mode))
			return FALSE;

		if(g_pOptions->pMachine->nCpu >= CPUF_68020)
			allowdest |= AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020;

		if((mode.eMode & allowdest) == 0)
		{
			prj_Error(ERROR_DEST_OPERAND);
			return TRUE;
		}
		dr = 0;
	}
	else
	{
		EAddrMode allowsrc = AM_AIND | AM_AINC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_PCDISP | AM_PCXDISP;

		if(!parse_GetAddrMode(&mode))
			return FALSE;

		if(g_pOptions->pMachine->nCpu >= CPUF_68020)
			allowsrc |= AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020;

		if((mode.eMode & allowsrc) == 0)
		{
			prj_Error(ERROR_SOURCE_OPERAND);
			return TRUE;
		}

		if(!parse_ExpectComma())
			return FALSE;

		reglist = parse_RegisterList();
		if(reglist == REGLIST_FAIL)
			return FALSE;

		dr = 1;
	}

	if(reglist == 0)
	{
		prj_Warn(MERROR_MOVEM_SKIPPED);
		return TRUE;
	}

	ins = (UWORD)(0x4880 | dr << 10 | parse_GetEAField(&mode));
	if(sz == SIZE_LONG)
		ins |= 1 << 6;

	sect_OutputAbsWord(ins);
	if(mode.eMode == AM_ADEC)
		reglist = parse_SwapBits((UWORD)reglist);
	sect_OutputAbsWord((UWORD)reglist);
	return parse_OutputExtWords(&mode);
}

static BOOL parse_MOVEP(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	UWORD dr;
	UWORD ar;
	UWORD opmode;
	SExpression* disp;

	if(src->eMode == AM_AIND)
	{
		src->eMode = AM_ADISP;
		src->Outer.pDisp = NULL;
	}

	if(dest->eMode == AM_AIND)
	{
		dest->eMode = AM_ADISP;
		dest->Outer.pDisp = NULL;
	}

	if(src->eMode == AM_ADISP && dest->eMode == AM_DREG)
	{
		if(sz == SIZE_WORD)
			opmode = 0x4;
		else
			opmode = 0x5;

		dr = (UWORD)dest->nDirectReg;
		ar = (UWORD)src->Outer.nBaseReg;
		disp = src->Outer.pDisp;
	}
	else if(src->eMode == AM_DREG && dest->eMode == AM_ADISP)
	{
		if(sz == SIZE_WORD)
			opmode = 0x6;
		else
			opmode = 0x7;

		dr = (UWORD)src->nDirectReg;
		ar = (UWORD)dest->Outer.nBaseReg;
		disp = dest->Outer.pDisp;
	}
	else
	{
		prj_Error(ERROR_OPERAND);
		return TRUE;
	}

	sect_OutputAbsWord(0x0008 | dr << 9 | opmode << 6 | ar);
	if(disp != NULL)
		sect_OutputExprWord(disp);
	else
		sect_OutputAbsWord(0);

	return TRUE;
}

static BOOL parse_MOVEQ(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	SExpression* expr = parse_CheckRange(src->pImmediate, -128, 127);
	if(expr == NULL)
	{
		prj_Error(ERROR_OPERAND_RANGE);
		return TRUE;
	}

	expr = parse_CreateANDExpr(expr, parse_CreateConstExpr(0xFF));
	expr = parse_CreateORExpr(expr, parse_CreateConstExpr(0x7000 | dest->nDirectReg << 9));
	sect_OutputExprWord(expr);
	return TRUE;
}

static BOOL parse_MULx(UWORD sign, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(sz == SIZE_LONG
	&& g_pOptions->pMachine->nCpu < CPUF_68020)
	{
		prj_Error(MERROR_INSTRUCTION_CPU);
		return TRUE;
	}

	if(sz == SIZE_LONG)
	{
		UWORD dh, dl, mul64;
		if(g_CurrentToken.ID.TargetToken == ':')
		{
			parse_GetToken();
			if(!parse_ExpectDataRegister(&dl))
				return FALSE;
			dh = (UWORD)dest->nDirectReg;
			mul64 = 1;
			if(dh == dl)
				prj_Warn(MERROR_UNDEFINED_RESULT);

			if(g_pOptions->pMachine->nCpu == CPUF_68060)
			{
				prj_Error(MERROR_INSTRUCTION_CPU);
				return TRUE;
			}
		}
		else
		{
			dl = (UWORD)dest->nDirectReg;
			dh = 0;
			mul64 = 0;
		}

		sect_OutputAbsWord((UWORD)(0x4C00 | parse_GetEAField(src)));
		sect_OutputAbsWord(0x0000 | sign << 11 | mul64 << 10 | dl << 12 | dh);
		return parse_OutputExtWords(src);
	}
	else
	{
		sect_OutputAbsWord((UWORD)(0xC0C0 | sign << 8 | dest->nDirectReg << 9 | parse_GetEAField(src)));
		return parse_OutputExtWords(src);
	}
}


static BOOL parse_MULS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_MULx(TRUE, sz, src, dest);
}

static BOOL parse_MULU(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_MULx(FALSE, sz, src, dest);
}

static BOOL parse_NBCD(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputAbsWord((UWORD)(0x4800 | parse_GetEAField(src)));
	return parse_OutputExtWords(src);
}

static BOOL parse_NEG(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputAbsWord((UWORD)(0x4400 | parse_GetEAField(src) | parse_GetSizeField(sz) << 6));
	return parse_OutputExtWords(src);
}

static BOOL parse_NEGX(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputAbsWord((UWORD)(0x4000 | parse_GetEAField(src) | parse_GetSizeField(sz) << 6));
	return parse_OutputExtWords(src);
}

static BOOL parse_NOP(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputAbsWord(0x4E71);
	return TRUE;
}

static BOOL parse_NOT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputAbsWord((UWORD)(0x4600 | parse_GetEAField(src) | parse_GetSizeField(sz) << 6));
	return parse_OutputExtWords(src);
}

static BOOL parse_ORI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(dest->eMode == AM_SYSREG)
	{
		if(sz != SIZE_WORD)
		{
			prj_Error(MERROR_INSTRUCTION_SIZE);
			return TRUE;
		}
		if(dest->nDirectReg == T_68K_REG_CCR)
		{
			sect_OutputAbsWord(0x003C);
			sect_OutputExprWord(parse_CreateANDExpr(src->pImmediate, parse_CreateConstExpr(0xFF)));
			return TRUE;
		}
		else if(dest->nDirectReg == T_68K_REG_SR)
		{
			prj_Warn(MERROR_INSTRUCTION_PRIV);
			sect_OutputAbsWord(0x007C);
			sect_OutputExprWord(src->pImmediate);
			return TRUE;
		}
		prj_Error(ERROR_DEST_OPERAND);
		return TRUE;
	}

	return parse_ArithmeticLogicalI(0x0000, sz, src, dest);
}

static BOOL parse_OR(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(src->eMode == AM_IMM || dest->eMode == AM_SYSREG)
		return parse_IntegerOp(T_68K_ORI, sz, src, dest);

	return parse_ArithmeticLogical(0x8000, sz, src, dest);
}

static BOOL parse_PackUnpack(UWORD ins, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	UWORD dy, dx;
	UWORD rm;
	SAddrMode adj;

	if(!parse_ExpectComma())
		return FALSE;

	if(!parse_GetAddrMode(&adj))
		return FALSE;

	if(src->eMode != dest->eMode || adj.eMode != AM_IMM)
	{
		prj_Error(ERROR_OPERAND);
		return TRUE;
	}

	if(src->eMode == AM_DREG)
	{
		dx = (UWORD)src->nDirectReg;
		dy = (UWORD)dest->nDirectReg;
		rm = 0;
	}
	else
	{
		dx = (UWORD)src->Outer.nBaseReg;
		dy = (UWORD)dest->Outer.nBaseReg;
		rm = 1;
	}

	sect_OutputAbsWord(ins | dy << 9 | rm << 3 | dx);
	sect_OutputExprWord(adj.pImmediate);
	return TRUE;
}

static BOOL parse_PACK(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_PackUnpack(0x8140, sz, src, dest);
}

static BOOL parse_UNPACK(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_PackUnpack(0x8180, sz, src, dest);
}

static BOOL parse_PEA(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputAbsWord((UWORD)(0x4840 | parse_GetEAField(src)));
	return parse_OutputExtWords(src);
}

static BOOL parse_RTD(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputAbsWord(0x4E74);
	sect_OutputExprWord(src->pImmediate);
	return TRUE;
}

static BOOL parse_RTM(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	UWORD reg;

	if(src->eMode == AM_DREG)
		reg = (UWORD)src->nDirectReg;
	else /* if(src->eMode == AM_AREG) */
		reg = (UWORD)src->nDirectReg + 8;

	sect_OutputAbsWord(0x06C0 | reg);
	return TRUE;
}

static BOOL parse_RTR(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputAbsWord(0x4E77);
	return TRUE;
}

static BOOL parse_RTS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputAbsWord(0x4E75);
	return TRUE;
}

static BOOL parse_Scc(UWORD code, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputAbsWord((UWORD)(0x50C0 | code << 8 | parse_GetEAField(src)));
	return parse_OutputExtWords(src);
}

static BOOL parse_ST(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0x0, sz, src, dest);
}

static BOOL parse_SF(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0x1, sz, src, dest);
}

static BOOL parse_SHI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0x2, sz, src, dest);
}

static BOOL parse_SLS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0x3, sz, src, dest);
}

static BOOL parse_SCC(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0x4, sz, src, dest);
}

static BOOL parse_SCS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0x5, sz, src, dest);
}

static BOOL parse_SNE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0x6, sz, src, dest);
}

static BOOL parse_SEQ(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0x7, sz, src, dest);
}

static BOOL parse_SVC(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0x8, sz, src, dest);
}

static BOOL parse_SVS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0x9, sz, src, dest);
}

static BOOL parse_SPL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0xA, sz, src, dest);
}

static BOOL parse_SMI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0xB, sz, src, dest);
}

static BOOL parse_SGE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0xC, sz, src, dest);
}

static BOOL parse_SLT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0xD, sz, src, dest);
}

static BOOL parse_SGT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0xE, sz, src, dest);
}

static BOOL parse_SLE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0xF, sz, src, dest);
}

static BOOL parse_SWAP(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputAbsWord((UWORD)(0x4840 | src->nDirectReg));
	return TRUE;
}

static BOOL parse_TAS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputAbsWord((UWORD)(0x4AC0 | parse_GetEAField(src)));
	return parse_OutputExtWords(src);
}

static BOOL parse_TRAP(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	SExpression* expr;

	expr = parse_CheckRange(src->pImmediate, 0, 15);
	if(expr == NULL)
	{
		prj_Error(ERROR_OPERAND_RANGE);
		return TRUE;
	}
	expr = parse_CreateORExpr(expr, parse_CreateConstExpr(0x4E40));
	sect_OutputExprWord(expr);
	return TRUE;
}

static BOOL parse_TRAPcc(UWORD code, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	UWORD opmode;

	if(sz == SIZE_DEFAULT && src->eMode == AM_EMPTY)
		opmode = 0x4;
	else if(sz == SIZE_WORD && src->eMode == AM_IMM)
		opmode = 0x2;
	else if(sz == SIZE_LONG && src->eMode == AM_IMM)
		opmode = 0x3;
	else
	{
		prj_Error(ERROR_OPERAND);
		return TRUE;
	}

	sect_OutputAbsWord(0x50F8 | opmode | code << 8);
	if(sz == SIZE_WORD)
		sect_OutputExprWord(src->pImmediate);
	else if(sz == SIZE_LONG)
		sect_OutputExprLong(src->pImmediate);

	return TRUE;
}

static BOOL parse_TRAPT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0x0, sz, src, dest);
}

static BOOL parse_TRAPF(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0x1, sz, src, dest);
}

static BOOL parse_TRAPHI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0x2, sz, src, dest);
}

static BOOL parse_TRAPLS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0x3, sz, src, dest);
}

static BOOL parse_TRAPCC(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0x4, sz, src, dest);
}

static BOOL parse_TRAPCS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0x5, sz, src, dest);
}

static BOOL parse_TRAPNE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0x6, sz, src, dest);
}

static BOOL parse_TRAPEQ(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0x7, sz, src, dest);
}

static BOOL parse_TRAPVC(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0x8, sz, src, dest);
}

static BOOL parse_TRAPVS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0x9, sz, src, dest);
}

static BOOL parse_TRAPPL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0xA, sz, src, dest);
}

static BOOL parse_TRAPMI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0xB, sz, src, dest);
}

static BOOL parse_TRAPGE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0xC, sz, src, dest);
}

static BOOL parse_TRAPLT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0xD, sz, src, dest);
}

static BOOL parse_TRAPGT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0xE, sz, src, dest);
}

static BOOL parse_TRAPLE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0xF, sz, src, dest);
}

static BOOL parse_TRAPV(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputAbsWord(0x4E76);
	return TRUE;
}

static BOOL parse_UNLK(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputAbsWord((UWORD)(0x4E58 | src->nDirectReg));
	return TRUE;
}

static BOOL parse_RESET(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	prj_Warn(MERROR_INSTRUCTION_PRIV);
	sect_OutputAbsWord(0x4E70);
	return TRUE;
}

static BOOL parse_RTE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	prj_Warn(MERROR_INSTRUCTION_PRIV);
	sect_OutputAbsWord(0x4E73);
	return TRUE;
}

static BOOL parse_STOP(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	prj_Warn(MERROR_INSTRUCTION_PRIV);
	sect_OutputAbsWord(0x4E72);
	sect_OutputExprWord(src->pImmediate);
	return TRUE;
}

static BOOL parse_Cache040(UWORD ins, UWORD scope, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	UWORD cache = 0;
	UWORD reg;
	
	prj_Warn(MERROR_INSTRUCTION_PRIV);

	if(src->nDirectReg == T_68K_REG_DC)
		cache = 0x1;
	else if(src->nDirectReg == T_68K_REG_IC)
		cache = 0x2;
	else if(src->nDirectReg == T_68K_REG_BC)
		cache = 0x3;
	else
	{
		prj_Error(ERROR_DEST_OPERAND);
		return TRUE;
	}

	if(scope == 3)
		reg = 0;
	else
		reg = (UWORD)dest->Outer.nBaseReg;

	sect_OutputAbsWord(ins | scope << 3 | cache << 6 | reg);
	return TRUE;
}

static BOOL parse_CINVA(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Cache040(0xF400, 0x3, sz, src, dest);
}

static BOOL parse_CINVL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Cache040(0xF400, 0x1, sz, src, dest);
}

static BOOL parse_CINVP(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Cache040(0xF400, 0x2, sz, src, dest);
}

static BOOL parse_CPUSHA(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Cache040(0xF420, 0x3, sz, src, dest);
}

static BOOL parse_CPUSHL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Cache040(0xF420, 0x1, sz, src, dest);
}

static BOOL parse_CPUSHP(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Cache040(0xF420, 0x2, sz, src, dest);
}

typedef struct
{
	UWORD	nCpu;
	UWORD	nValue;
} SControlRegister;

SControlRegister g_ControlRegister[]=
{
	{// SFC
		CPUF_68010 | CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		0x000 },
	{// DFC
		CPUF_68010 | CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		0x001 },
	{// USP
		CPUF_68010 | CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		0x800 },
	{// VBR
		CPUF_68010 | CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		0x801 },

	{// CACR
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		0x002 },
	{// CAAR
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		0x802 },
	{// MSP
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		0x803 },
	{// ISP
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		0x804 },

	{// TC
		CPUF_68040 | CPUF_68060,
		0x003 },
	{// ITT0
		CPUF_68040 | CPUF_68060,
		0x004 },
	{// ITT1
		CPUF_68040 | CPUF_68060,
		0x005 },
	{// DTT0
		CPUF_68040 | CPUF_68060,
		0x006 },
	{// DTT1
		CPUF_68040 | CPUF_68060,
		0x007 },
	{// MMUSR
		CPUF_68040 | CPUF_68060,
		0x805 },
	{// URP
		CPUF_68040 | CPUF_68060,
		0x806 },
	{// SRP
		CPUF_68040 | CPUF_68060,
		0x807 },

	{// IACR0
		CPUF_68040,
		0x004 },
	{// IACR1
		CPUF_68040,
		0x005 },
	{// DACR0
		CPUF_68040,
		0x006 },
	{// DACR1
		CPUF_68040,
		0x007 },
};

static BOOL parse_MOVEC(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	UWORD dr;
	int control;
	UWORD reg;
	SControlRegister* pReg;

	prj_Warn(MERROR_INSTRUCTION_PRIV);

	if(src->eMode == AM_SYSREG && (dest->eMode == AM_DREG || dest->eMode == AM_AREG))
	{
		dr = 0;
		control = src->nDirectReg;
		if(dest->eMode == AM_DREG)
			reg = (UWORD)dest->nDirectReg;
		else
			reg = (UWORD)dest->nDirectReg + 8;
	}
	else if(dest->eMode == AM_SYSREG && (src->eMode == AM_DREG || src->eMode == AM_AREG))
	{
		dr = 1;
		control = dest->nDirectReg;
		if(src->eMode == AM_DREG)
			reg = (UWORD)src->nDirectReg;
		else
			reg = (UWORD)src->nDirectReg + 8;
	}
	else
	{
		prj_Error(ERROR_OPERAND);
		return TRUE;
	}

	if(control < T_68K_REG_SFC || control > T_68K_REG_DACR1)
	{
		prj_Error(ERROR_OPERAND);
		return TRUE;
	}

	pReg = &g_ControlRegister[control - T_68K_REG_SFC];
	if((pReg->nCpu & g_pOptions->pMachine->nCpu) == 0)
	{
		prj_Error(MERROR_INSTRUCTION_CPU);
		return TRUE;
	}

	sect_OutputAbsWord(0x4E7A | dr);
	sect_OutputAbsWord(reg << 12 | pReg->nValue);
	return TRUE;
}

static BOOL parse_MOVES(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	int allow = AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_PCXDISP | AM_WORD | AM_LONG | AM_PCXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020;
	UWORD dr;
	UWORD reg;
	SAddrMode* ea;

	prj_Warn(MERROR_INSTRUCTION_PRIV);

	if((src->eMode == AM_DREG || src->eMode == AM_AREG)
	&& (dest->eMode & allow))
	{
		ea = dest;
		dr = 1;
		if(src->eMode == AM_DREG)
			reg = (UWORD)src->nDirectReg;
		else
			reg = (UWORD)src->nDirectReg + 8;
	}
	else if((dest->eMode == AM_DREG || dest->eMode == AM_AREG)
	&& (src->eMode & allow))
	{
		ea = src;
		dr = 0;
		if(dest->eMode == AM_DREG)
			reg = (UWORD)src->nDirectReg;
		else
			reg = (UWORD)src->nDirectReg + 8;
	}
	else
	{
		prj_Error(ERROR_OPERAND);
		return TRUE;
	}

	sect_OutputAbsWord((UWORD)(0x0E00 | parse_GetSizeField(sz) << 6 | parse_GetEAField(ea)));
	sect_OutputAbsWord(reg << 12 | dr << 11);
	return parse_OutputExtWords(ea);
}


typedef struct
{
	int	nCPU;
	int	nAllowSize;
	int	nDefaultSize;
	ULONG nAllowSrc;
	ULONG nAllowDest;
	ULONG nAllowSrc020;
	ULONG nAllowDest020;
	BOOL (*pHandler)(ESize eSize, SAddrMode* pSrc, SAddrMode* pDest);
} SInstruction;

static SInstruction sIntegerInstructions[] =
{
	{	// ABCD
		CPUF_ALL,
		SIZE_BYTE, SIZE_BYTE,
		AM_DREG | AM_ADEC, AM_DREG | AM_ADEC,
		0, 0,
		parse_ABCD
	},
	{	// ADD
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /*dest*/ AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG /* for adda */ | AM_AREG,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /*dest*/ AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
		parse_ADD
	},
	{	// ADDA
		CPUF_ALL,
		SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /*dest*/ AM_AREG,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /*dest*/ 0,
		parse_ADDA
	},
	{	// ADDI
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_IMM, AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG,
		0, AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
		parse_ADDI
	},
	{	// ADDQ
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_IMM, AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG,
		0, AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
		parse_ADDQ
	},
	{	// ADDX
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_DREG | AM_ADEC, AM_DREG | AM_ADEC,
		0, 0,
		parse_ADDX
	},
	{	// AND
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /*dest*/ AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG /* for ANDI */ | AM_SYSREG,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /*dest*/ AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
		parse_AND
	},
	{	// ANDI
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_IMM, AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_SYSREG,
		0, AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
		parse_ANDI
	},
	{	// ASL
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_IMM | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_DREG | (ULONG)AM_EMPTY,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		parse_ASL
	},
	{	// ASR
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_IMM | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_DREG | (ULONG)AM_EMPTY,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		parse_ASR
	},
	{	// BCC
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_LONG, 0,
		0, 0,
		parse_BCC
	},
	{	// BCS
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_LONG, 0,
		0, 0,
		parse_BCS
	},
	{	// BEQ
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_LONG, 0,
		0, 0,
		parse_BEQ
	},
	{	// BGE
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_LONG, 0,
		0, 0,
		parse_BGE
	},
	{	// BGT
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_LONG, 0,
		0, 0,
		parse_BGT
	},
	{	// BHI
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_LONG, 0,
		0, 0,
		parse_BHI
	},
	{	// BLE
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_LONG, 0,
		0, 0,
		parse_BLE
	},
	{	// BLS
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_LONG, 0,
		0, 0,
		parse_BLS
	},
	{	// BLT
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_LONG, 0,
		0, 0,
		parse_BLT
	},
	{	// BMI
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_LONG, 0,
		0, 0,
		parse_BMI
	},
	{	// BNE
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_LONG, 0,
		0, 0,
		parse_BNE
	},
	{	// BPL
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_LONG, 0,
		0, 0,
		parse_BPL
	},
	{	// BVC
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_LONG, 0,
		0, 0,
		parse_BVC
	},
	{	// BVS
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_LONG, 0,
		0, 0,
		parse_BVS
	},
	{	// BCHG
		CPUF_ALL,
		SIZE_DEFAULT, SIZE_DEFAULT,
		AM_DREG | AM_IMM, /* dest */ AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG,
		0, AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
		parse_BCHG
	},
	{	// BCLR
		CPUF_ALL,
		SIZE_DEFAULT, SIZE_DEFAULT,
		AM_DREG | AM_IMM, /* dest */ AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG,
		0, AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
		parse_BCLR
	},
	{	// BFCHG
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_DEFAULT, SIZE_DEFAULT,
		AM_DREG | AM_AIND | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_BITFIELD, 0,
		0, 0,
		parse_BFCHG
	},
	{	// BFCLR
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_DEFAULT, SIZE_DEFAULT,
		AM_DREG | AM_AIND | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_BITFIELD, 0,
		0, 0,
		parse_BFCLR
	},
	{	// BFEXTS
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_DEFAULT, SIZE_DEFAULT,
		AM_DREG | AM_AIND | AM_ADISP | AM_AXDISP | AM_PCDISP | AM_PCXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020 | AM_BITFIELD, AM_DREG,
		0, 0,
		parse_BFEXTS
	},
	{	// BFEXTU
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_DEFAULT, SIZE_DEFAULT,
		AM_DREG | AM_AIND | AM_ADISP | AM_AXDISP | AM_PCDISP | AM_PCXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020 | AM_BITFIELD, AM_DREG,
		0, 0,
		parse_BFEXTU
	},
	{	// BFFFO
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_DEFAULT, SIZE_DEFAULT,
		AM_DREG | AM_AIND | AM_ADISP | AM_AXDISP | AM_PCDISP | AM_PCXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020 | AM_BITFIELD, AM_DREG,
		0, 0,
		parse_BFFFO
	},
	{	// BFINS
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_DEFAULT, SIZE_DEFAULT,
		AM_DREG, AM_DREG | AM_AIND | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_BITFIELD,
		0, 0,
		parse_BFINS
	},
	{	// BFSET
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_DEFAULT, SIZE_DEFAULT,
		AM_DREG | AM_AIND | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_BITFIELD, 0,
		0, 0,
		parse_BFSET
	},
	{	// BFTST
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_DEFAULT, SIZE_DEFAULT,
		AM_DREG | AM_AIND | AM_ADISP | AM_AXDISP | AM_PCDISP | AM_PCXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020 | AM_BITFIELD, 0,
		0, 0,
		parse_BFTST
	},
	{	// BKPT
		CPUF_ALL,
		SIZE_DEFAULT, SIZE_DEFAULT,
		AM_IMM, 0,
		0, 0,
		parse_BKPT
	},
	{	// BRA
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_LONG, 0,
		0, 0,
		parse_BRA
	},
	{	// BSET
		CPUF_ALL,
		SIZE_DEFAULT, SIZE_DEFAULT,
		AM_DREG | AM_IMM, /* dest */ AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG,
		0, AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
		parse_BSET
	},
	{	// BSR
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_LONG, 0,
		0, 0,
		parse_BSR
	},
	{	// BTST
		CPUF_ALL,
		SIZE_DEFAULT, SIZE_DEFAULT,
		AM_DREG | AM_IMM, /* dest */ AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG,
		0, AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
		parse_BTST
	},
	{	// CALLM
		CPUF_68020,
		SIZE_DEFAULT, SIZE_DEFAULT,
		AM_IMM, AM_AIND | AM_ADISP | AM_AXDISP | AM_PCDISP | AM_PCXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020,
		0, 0,
		parse_CALLM
	},
	{	// CAS
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_DREG, AM_DREG,
		0, 0,
		parse_CAS
	},
	{	// CAS2
		CPUF_68020 | CPUF_68030 | CPUF_68040,
		SIZE_WORD | SIZE_LONG, SIZE_WORD,
		0, 0,
		0, 0,
		parse_CAS2
	},
	{	// CHK
		CPUF_ALL,
		SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /*dest*/ AM_DREG,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /*dest*/ 0,
		parse_CHK
	},
	{	// CHK2
		CPUF_68020 | CPUF_68030 | CPUF_68040,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_AIND | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_PCDISP | AM_PCXDISP | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /*dest*/ AM_DREG | AM_AREG,
		0, 0,
		parse_CHK2
	},
	{	// CINVA
		CPUF_68040 | CPUF_68060,
		0, 0,
		AM_SYSREG, 0,
		0, 0,
		parse_CINVA
	},
	{	// CINVL
		CPUF_68040 | CPUF_68060,
		0, 0,
		AM_SYSREG, AM_AIND,
		0, 0,
		parse_CINVL
	},
	{	// CINVP
		CPUF_68040 | CPUF_68060,
		0, 0,
		AM_SYSREG, AM_AIND,
		0, 0,
		parse_CINVP
	},
	{	// CLR
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, 0,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		parse_CLR
	},
	{	// CMP
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /*dest*/ AM_DREG /* for CMPA */ | AM_AREG /* for CMPM */ | AM_AINC /* for CMPI */ | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /*dest*/ 0,
		parse_CMP
	},
	{	// CMPA
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /*dest*/ AM_AREG,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /*dest*/ 0,
		parse_CMPA
	},
	{	// CMPI
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_IMM, AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG,
		0, AM_PCDISP | AM_PCXDISP | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020,
		parse_CMPI
	},
	{	// CMPM
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_AINC, AM_AINC,
		0, 0,
		parse_CMPM
	},
	{	// CMP2
		CPUF_68020 | CPUF_68030 | CPUF_68040,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_AIND | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_PCDISP | AM_PCXDISP | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, AM_DREG | AM_AREG,
		0, 0,
		parse_CMP2
	},
	{	// CPUSHA
		CPUF_68040 | CPUF_68060,
		0, 0,
		AM_SYSREG, 0,
		0, 0,
		parse_CPUSHA
	},
	{	// CPUSHL
		CPUF_68040 | CPUF_68060,
		0, 0,
		AM_SYSREG, AM_AIND,
		0, 0,
		parse_CPUSHL
	},
	{	// CPUSHP
		CPUF_68040 | CPUF_68060,
		0, 0,
		AM_SYSREG, AM_AIND,
		0, 0,
		parse_CPUSHP
	},
	{	// DBCC
		CPUF_ALL,
		0, 0,
		AM_DREG, AM_LONG,
		0, 0,
		parse_DBCC
	},
	{	// DBCS
		CPUF_ALL,
		0, 0,
		AM_DREG, AM_LONG,
		0, 0,
		parse_DBCS
	},
	{	// DBEQ
		CPUF_ALL,
		0, 0,
		AM_DREG, AM_LONG,
		0, 0,
		parse_DBEQ
	},
	{	// DBF
		CPUF_ALL,
		0, 0,
		AM_DREG, AM_LONG,
		0, 0,
		parse_DBF
	},
	{	// DBGE
		CPUF_ALL,
		0, 0,
		AM_DREG, AM_LONG,
		0, 0,
		parse_DBGE
	},
	{	// DBGT
		CPUF_ALL,
		0, 0,
		AM_DREG, AM_LONG,
		0, 0,
		parse_DBGT
	},
	{	// DBHI
		CPUF_ALL,
		0, 0,
		AM_DREG, AM_LONG,
		0, 0,
		parse_DBHI
	},
	{	// DBLE
		CPUF_ALL,
		0, 0,
		AM_DREG, AM_LONG,
		0, 0,
		parse_DBLE
	},
	{	// DBLS
		CPUF_ALL,
		0, 0,
		AM_DREG, AM_LONG,
		0, 0,
		parse_DBLS
	},
	{	// DBLT
		CPUF_ALL,
		0, 0,
		AM_DREG, AM_LONG,
		0, 0,
		parse_DBLT
	},
	{	// DBMI
		CPUF_ALL,
		0, 0,
		AM_DREG, AM_LONG,
		0, 0,
		parse_DBMI
	},
	{	// DBNE
		CPUF_ALL,
		0, 0,
		AM_DREG, AM_LONG,
		0, 0,
		parse_DBNE
	},
	{	// DBPL
		CPUF_ALL,
		0, 0,
		AM_DREG, AM_LONG,
		0, 0,
		parse_DBPL
	},
	{	// DBT
		CPUF_ALL,
		0, 0,
		AM_DREG, AM_LONG,
		0, 0,
		parse_DBT
	},
	{	// DBVC
		CPUF_ALL,
		0, 0,
		AM_DREG, AM_LONG,
		0, 0,
		parse_DBVC
	},
	{	// DBVS
		CPUF_ALL,
		0, 0,
		AM_DREG, AM_LONG,
		0, 0,
		parse_DBVS
	},

	{	// DIVS
		CPUF_ALL,
		SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, AM_DREG,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, 0,
		parse_DIVS
	},
	{	// DIVSL
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_LONG, SIZE_LONG,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, AM_DREG,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, 0,
		parse_DIVSL
	},
	{	// DIVU
		CPUF_ALL,
		SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, AM_DREG,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, 0,
		parse_DIVU
	},
	{	// DIVUL
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_LONG, SIZE_LONG,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, AM_DREG,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, 0,
		parse_DIVUL
	},
	{	// EOR
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_IMM | AM_DREG, AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_SYSREG,
		0, AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
		parse_EOR
	},
	{	// EORI
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_IMM, AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_SYSREG,
		0, AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
		parse_EORI
	},
	{	// EXG
		CPUF_ALL,
		SIZE_LONG, SIZE_LONG,
		AM_DREG | AM_AREG, AM_DREG | AM_AREG,
		0, 0,
		parse_EXG
	},
	{	// EXT
		CPUF_ALL,
		SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_DREG, 0,
		0, 0,
		parse_EXT
	},
	{	// EXTB
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_LONG, SIZE_LONG,
		AM_DREG, 0,
		0, 0,
		parse_EXTB
	},
	{	// ILLEGAL
		CPUF_ALL,
		0, 0,
		0, 0,
		0, 0,
		parse_ILLEGAL
	},
	{	// JMP
		CPUF_ALL,
		0, 0,
		AM_AIND | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_PCDISP | AM_PCXDISP, 0,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, 0,
		parse_JMP
	},
	{	// JSR
		CPUF_ALL,
		0, 0,
		AM_AIND | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_PCDISP | AM_PCXDISP, 0,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, 0,
		parse_JSR
	},
	{	// LEA
		CPUF_ALL,
		0, 0,
		AM_AIND | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_PCDISP | AM_PCXDISP, AM_AREG,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, 0,
		parse_LEA
	},
	{	// LINK
		CPUF_ALL,
		SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_AREG, AM_IMM,
		0, 0,
		parse_LINK
	},
	{	// LSL
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_IMM | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_DREG | (ULONG)AM_EMPTY,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		parse_LSL
	},
	{	// LSR
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_IMM | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_DREG | (ULONG)AM_EMPTY,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		parse_LSR
	},
	{	// MOVE
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_SYSREG | AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /* dest */ AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG /* for movea */ | AM_AREG /* for move to ccr */ | AM_SYSREG,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /* dest */ AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
		parse_MOVE
	},
	{	// MOVEA
		CPUF_ALL,
		SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /* dest */ AM_AREG,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /* dest */ 0,
		parse_MOVEA
	},
	{	// MOVEC
		CPUF_68010 | CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_LONG, SIZE_LONG,
		AM_DREG | AM_AREG | AM_SYSREG, AM_DREG | AM_AREG | AM_SYSREG,
		0, 0,
		parse_MOVEC
	},
	{	// MOVE16
		CPUF_68040 | CPUF_68060,
		0, 0,
		AM_AIND | AM_AINC | AM_LONG, AM_AIND | AM_AINC | AM_LONG,
		0, 0,
		parse_MOVE16
	},
	{	// MOVEM
		CPUF_ALL,
		SIZE_WORD | SIZE_LONG, SIZE_WORD,
		0, 0,
		0, 0,
		parse_MOVEM
	},
	{	// MOVEP
		CPUF_68000 | CPUF_68010 | CPUF_68020 | CPUF_68030 | CPUF_68040,
		SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_DREG | AM_AIND | AM_ADISP, AM_DREG | AM_AIND | AM_ADISP,
		0, 0,
		parse_MOVEP
	},
	{	// MOVEQ
		CPUF_ALL,
		SIZE_LONG, SIZE_LONG,
		AM_IMM, AM_DREG,
		0, 0,
		parse_MOVEQ
	},
	{	// MOVES
		CPUF_68010 | CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_LONG | SIZE_WORD | SIZE_BYTE, SIZE_WORD,
		AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_PCXDISP | AM_WORD | AM_LONG, AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_PCXDISP | AM_WORD | AM_LONG,
		AM_PCXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_PCXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
		parse_MOVES
	},
	{	// MULS
		CPUF_ALL,
		SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /* dest */ AM_DREG,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /* dest */ 0,
		parse_MULS
	},
	{	// MULU
		CPUF_ALL,
		SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /* dest */ AM_DREG,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /* dest */ 0,
		parse_MULU
	},
	{	// NBCD
		CPUF_ALL,
		SIZE_BYTE, SIZE_BYTE,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, 0,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		parse_NBCD
	},
	{	// NEG
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, 0,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		parse_NEG
	},
	{	// NEGX
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, 0,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		parse_NEGX
	},
	{	// NOP
		CPUF_ALL,
		0, 0,
		0, 0,
		0, 0,
		parse_NOP
	},
	{	// NOT
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, 0,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		parse_NOT
	},
	{	// OR
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /*dest*/ AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG /* for ORI */ | AM_SYSREG,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /*dest*/ AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
		parse_OR
	},
	{	// ORI
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_IMM, AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_SYSREG,
		0, AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
		parse_ORI
	},
	{	// PACK
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		0, 0,
		AM_DREG | AM_ADEC, AM_DREG | AM_ADEC,
		0, 0,
		parse_PACK
	},
	{	// PEA
		CPUF_ALL,
		SIZE_LONG, SIZE_LONG,
		AM_AIND | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_PCDISP | AM_PCXDISP, 0,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, 0,
		parse_PEA
	},
	{	// RESET
		CPUF_ALL,
		0, 0,
		0, 0,
		0, 0,
		parse_RESET
	},
	{	// ROL
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_IMM | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_DREG | (ULONG)AM_EMPTY,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		parse_ROL
	},
	{	// ROR
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_IMM | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_DREG | (ULONG)AM_EMPTY,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		parse_ROR
	},
	{	// ROXL
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_IMM | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_DREG | (ULONG)AM_EMPTY,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		parse_ROXL
	},
	{	// ROXR
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_IMM | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_DREG | (ULONG)AM_EMPTY,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		parse_ROXR
	},
	{	// RTD
		CPUF_68010 | CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		0, 0,
		AM_IMM, 0,
		0, 0,
		parse_RTD
	},
	{	// RTE
		CPUF_ALL,
		0, 0,
		0, 0,
		0, 0,
		parse_RTE
	},
	{	// RTM
		CPUF_68020,
		0, 0,
		AM_DREG | AM_AREG, 0,
		0, 0,
		parse_RTM
	},
	{	// RTR
		CPUF_ALL,
		0, 0,
		0, 0,
		0, 0,
		parse_RTR
	},
	{	// RTS
		CPUF_ALL,
		0, 0,
		0, 0,
		0, 0,
		parse_RTS
	},
	{	// SBCD
		CPUF_ALL,
		SIZE_BYTE, SIZE_BYTE,
		AM_DREG | AM_ADEC, AM_DREG | AM_ADEC,
		0, 0,
		parse_SBCD
	},

	{	// SCC
		CPUF_ALL,
		SIZE_BYTE, SIZE_BYTE,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		0, 0,
		parse_SCC
	},
	{	// SCS
		CPUF_ALL,
		SIZE_BYTE, SIZE_BYTE,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		0, 0,
		parse_SCS
	},
	{	// SEQ
		CPUF_ALL,
		SIZE_BYTE, SIZE_BYTE,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		0, 0,
		parse_SEQ
	},
	{	// SF
		CPUF_ALL,
		SIZE_BYTE, SIZE_BYTE,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		0, 0,
		parse_SF
	},
	{	// SGE
		CPUF_ALL,
		SIZE_BYTE, SIZE_BYTE,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		0, 0,
		parse_SGE
	},
	{	// SGT
		CPUF_ALL,
		SIZE_BYTE, SIZE_BYTE,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		0, 0,
		parse_SGT
	},
	{	// SHI
		CPUF_ALL,
		SIZE_BYTE, SIZE_BYTE,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		0, 0,
		parse_SHI
	},
	{	// SLE
		CPUF_ALL,
		SIZE_BYTE, SIZE_BYTE,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		0, 0,
		parse_SLE
	},
	{	// SLS
		CPUF_ALL,
		SIZE_BYTE, SIZE_BYTE,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		0, 0,
		parse_SLS
	},
	{	// SLT
		CPUF_ALL,
		SIZE_BYTE, SIZE_BYTE,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		0, 0,
		parse_SLT
	},
	{	// SMI
		CPUF_ALL,
		SIZE_BYTE, SIZE_BYTE,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		0, 0,
		parse_SMI
	},
	{	// SNE
		CPUF_ALL,
		SIZE_BYTE, SIZE_BYTE,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		0, 0,
		parse_SNE
	},
	{	// SPL
		CPUF_ALL,
		SIZE_BYTE, SIZE_BYTE,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		0, 0,
		parse_SPL
	},
	{	// ST
		CPUF_ALL,
		SIZE_BYTE, SIZE_BYTE,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		0, 0,
		parse_ST
	},
	{	// SVC
		CPUF_ALL,
		SIZE_BYTE, SIZE_BYTE,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		0, 0,
		parse_SVC
	},
	{	// SVS
		CPUF_ALL,
		SIZE_BYTE, SIZE_BYTE,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		0, 0,
		parse_SVS
	},
		
	{	// STOP
		CPUF_ALL,
		0, 0,
		AM_IMM, 0,
		0, 0,
		parse_STOP
	},
	{	// SUB
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /*dest*/ AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG /* for adda */ | AM_AREG,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /*dest*/ AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
		parse_SUB
	},
	{	// SUBA
		CPUF_ALL,
		SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /*dest*/ AM_AREG,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /*dest*/ 0,
		parse_SUBA
	},
	{	// SUBI
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_IMM, AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG,
		0, AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
		parse_SUBI
	},
	{	// SUBQ
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_IMM, AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG,
		0, AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
		parse_SUBQ
	},
	{	// SUBX
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_DREG | AM_ADEC, AM_DREG | AM_ADEC,
		0, 0,
		parse_SUBX
	},
	{	// SWAP
		CPUF_ALL,
		SIZE_WORD, SIZE_WORD,
		AM_DREG, 0,
		0, 0,
		parse_SWAP
	},
	{	// TAS
		CPUF_ALL,
		SIZE_BYTE, SIZE_BYTE,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		0, 0,
		parse_TAS
	},
	{	// TRAP
		CPUF_ALL,
		0, 0,
		AM_IMM, 0,
		0, 0,
		parse_TRAP
	},

	{	// TRAPCC
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(ULONG)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPCC
	},
	{	// TRAPCS
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(ULONG)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPCS
	},
	{	// TRAPEQ
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(ULONG)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPEQ
	},
	{	// TRAPF
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(ULONG)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPF
	},
	{	// TRAPGE
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(ULONG)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPGE
	},
	{	// TRAPGT
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(ULONG)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPGT
	},
	{	// TRAPHI
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(ULONG)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPHI
	},
	{	// TRAPLE
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(ULONG)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPLE
	},
	{	// TRAPLS
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(ULONG)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPLS
	},
	{	// TRAPLT
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(ULONG)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPLT
	},
	{	// TRAPMI
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(ULONG)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPMI
	},
	{	// TRAPNE
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(ULONG)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPNE
	},
	{	// TRAPPL
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(ULONG)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPPL
	},
	{	// TRAPT
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(ULONG)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPT
	},
	{	// TRAPVC
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(ULONG)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPVC
	},
	{	// TRAPVS
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(ULONG)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPVS
	},
	{	// TRAPV
		CPUF_ALL,
		0, 0,
		0, 0,
		0, 0,
		parse_TRAPV
	},

	{	// TST
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, 0,
		AM_IMM | AM_PCDISP | AM_PCXDISP | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, 0,
		parse_TST
	},
	{	// UNLK
		CPUF_ALL,
		0, 0,
		AM_AREG, 0,
		0, 0,
		parse_UNLK
	},
	{	// UNPACK
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		0, 0,
		AM_DREG | AM_ADEC, AM_DREG | AM_ADEC,
		0, 0,
		parse_UNPACK
	},

};

static BOOL parse_IntegerOp(int op, ESize inssz, SAddrMode* src, SAddrMode* dest)
{
	SInstruction* pIns;
	EAddrMode allowsrc;
	EAddrMode allowdest;

	if(op >= T_68K_INTEGER_FIRST)
		op -= T_68K_INTEGER_FIRST;

	pIns = &sIntegerInstructions[op];

	allowsrc = pIns->nAllowSrc;
	if(g_pOptions->pMachine->nCpu >= CPUF_68020)
		allowsrc |= pIns->nAllowSrc020;

	if((allowsrc & src->eMode) == 0
	&& !(allowsrc == 0 && src->eMode == AM_EMPTY))
	{
		prj_Error(ERROR_SOURCE_OPERAND);
		return TRUE;
	}

	allowdest = pIns->nAllowDest;
	if(g_pOptions->pMachine->nCpu >= CPUF_68020)
		allowdest |= pIns->nAllowDest020;

	if((allowdest & dest->eMode) == 0
	&& !(allowdest == 0 && dest->eMode == AM_EMPTY))
	{
		prj_Error(ERROR_DEST_OPERAND);
		return TRUE;
	}

	return pIns->pHandler(inssz, src, dest);
}

BOOL parse_GetBitfield(SAddrMode* pMode)
{
	if(parse_ExpectChar('{'))
	{
		pMode->bBitfield = TRUE;

		if(g_CurrentToken.ID.TargetToken >= T_68K_REG_D0
		&& g_CurrentToken.ID.TargetToken <= T_68K_REG_D7)
		{
			pMode->nBFOffsetReg = g_CurrentToken.ID.TargetToken - T_68K_REG_D0;
			pMode->pBFOffsetExpr = NULL;
			parse_GetToken();
		}
		else
		{
			pMode->pBFOffsetExpr = parse_Expression();
			if(pMode->pBFOffsetExpr == NULL)
			{
				prj_Error(ERROR_OPERAND);
				return FALSE;
			}
			pMode->nBFOffsetReg = -1;
		}

		if(!parse_ExpectChar(':'))
			return FALSE;

		if(g_CurrentToken.ID.TargetToken >= T_68K_REG_D0
		&& g_CurrentToken.ID.TargetToken <= T_68K_REG_D7)
		{
			pMode->nBFWidthReg = g_CurrentToken.ID.TargetToken - T_68K_REG_D0;
			pMode->pBFWidthExpr = NULL;
			parse_GetToken();
		}
		else
		{
			pMode->pBFWidthExpr = parse_Expression();
			if(pMode->pBFWidthExpr == NULL)
			{
				prj_Error(ERROR_OPERAND);
				return TRUE;
			}
			pMode->nBFWidthReg = -1;
		}

		return parse_ExpectChar('}');
	}

	return FALSE;
}

BOOL parse_IntegerInstruction(void)
{
	int op;
	SInstruction* pIns;
	ESize inssz;
	SAddrMode src;
	SAddrMode dest;

	if(g_CurrentToken.ID.TargetToken < T_68K_INTEGER_FIRST
	|| g_CurrentToken.ID.TargetToken > T_68K_INTEGER_LAST)
	{
		return FALSE;
	}

	op = g_CurrentToken.ID.TargetToken - T_68K_INTEGER_FIRST;
	parse_GetToken();

	pIns = &sIntegerInstructions[op];
	if((pIns->nCPU & g_pOptions->pMachine->nCpu) == 0)
	{
		prj_Error(MERROR_INSTRUCTION_CPU);
		return TRUE;
	}

	if(pIns->nAllowSize == SIZE_DEFAULT)
	{
		if(parse_GetSizeSpec(SIZE_DEFAULT) != SIZE_DEFAULT)
		{
			prj_Warn(MERROR_IGNORING_SIZE);
			parse_GetToken();
		}
		inssz = SIZE_DEFAULT;
	}
	else
		inssz = parse_GetSizeSpec(pIns->nDefaultSize);

	src.eMode = AM_EMPTY;
	dest.eMode = AM_EMPTY;

	if(pIns->nAllowSrc != 0 && pIns->nAllowSrc != AM_EMPTY)
	{
		if(parse_GetAddrMode(&src))
		{
			if(pIns->nAllowSrc & AM_BITFIELD)
			{
				if(!parse_GetBitfield(&src))
				{
					prj_Error(MERROR_EXPECT_BITFIELD);
					return FALSE;
				}
			}

			if(src.eMode == AM_IMM)
				src.eImmSize = inssz;
		}
		else
		{
			if((pIns->nAllowSrc & AM_EMPTY) == 0)
				return TRUE;
		}
	}

	if(pIns->nAllowDest != 0)
	{
		if(g_CurrentToken.ID.TargetToken == ',')
		{
			parse_GetToken();
			if(!parse_GetAddrMode(&dest))
				return FALSE;

			if(pIns->nAllowDest & AM_BITFIELD)
			{
				if(!parse_GetBitfield(&dest))
				{
					prj_Error(MERROR_EXPECT_BITFIELD);
					return FALSE;
				}
			}
		}
	}

	if((pIns->nAllowSize & inssz) == 0
	&&	pIns->nAllowSize != 0
	&&	pIns->nDefaultSize != 0)
	{
		prj_Error(MERROR_INSTRUCTION_SIZE);
	}

	return parse_IntegerOp(op, inssz, &src, &dest);
}

#endif
