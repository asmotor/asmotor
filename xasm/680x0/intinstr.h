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

static bool_t parse_IntegerOp(int op, ESize inssz, SAddrMode* src, SAddrMode* dest);

static bool_t parse_OutputExtWords(SAddrMode* mode)
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
						sect_OutputExpr16(mode->pImmediate);
						return true;
					}
					return false;
				}
				default:
				case SIZE_WORD:
				{
					mode->pImmediate = parse_Check16bit(mode->pImmediate);
					if(mode->pImmediate)
					{
						sect_OutputExpr16(mode->pImmediate);
						return true;
					}
					return false;
				}
				case SIZE_LONG:
				{
					if(mode->pImmediate)
					{
						sect_OutputExprLong(mode->pImmediate);
						return true;
					}
					return false;
				}
			}
		}
		case AM_WORD:
		{
			if(mode->Outer.pDisp)
			{
				sect_OutputExpr16(mode->Outer.pDisp);
				return true;
			}
			internalerror("no word");
			break;
		}
		case AM_LONG:
		{
			if(mode->Outer.pDisp)
			{
				sect_OutputExprLong(mode->Outer.pDisp);
				return true;
			}
			internalerror("no long");
			break;
		}
		case AM_PCDISP:
		{
			if(mode->Outer.pDisp)
				mode->Outer.pDisp = expr_PcRelative(mode->Outer.pDisp, 0);
		}
		// fall through
		case AM_ADISP:
		{
			if(mode->Outer.pDisp)
			{
				if(mode->Outer.eDispSize == SIZE_WORD
				|| mode->Outer.eDispSize == SIZE_DEFAULT)
				{
					sect_OutputExpr16(mode->Outer.pDisp);
					return true;
				}
				prj_Error(MERROR_DISP_SIZE);
				return false;
			}
			internalerror("no displacement word");
			break;
		}
		case AM_PCXDISP:
		{
			if(mode->Outer.pDisp)
				mode->Outer.pDisp = expr_PcRelative(mode->Outer.pDisp, 0);
		}
		// fall through
		case AM_AXDISP:
		{
			SExpression* expr;
			uint16_t ins = (uint16_t)(mode->Outer.nIndexReg << 12);
			if(mode->Outer.eIndexSize == SIZE_LONG)
				ins |= 0x0800;

			if(mode->Outer.pDisp != NULL)
				expr = parse_Check8bit(mode->Outer.pDisp);
			else
				expr = expr_Const(0);

			expr = expr_And(expr, expr_Const(0xFF));
			if(expr != NULL)
			{
				expr = expr_Or(expr, expr_Const(ins));
				if(mode->Outer.pIndexScale != NULL)
				{
					expr = expr_Or(expr, expr_Shl(mode->Outer.pIndexScale, expr_Const(9)));
				}
				sect_OutputExpr16(expr);
				return true;
			}
			return false;
		}

		case AM_DREG:
		case AM_AREG:
		case AM_AIND:
		case AM_AINC:
		case AM_ADEC:
			return true;

		case AM_PCXDISP020:
		{
			if(mode->Outer.pDisp)
				mode->Outer.pDisp = expr_PcRelative(mode->Outer.pDisp, 2);
		}
		// fall through
		case AM_AXDISP020:
		{
			uint16_t ins = 0x0100;
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
						return false;
				}
			}
			else
			{
				ins |= 0x0010;
			}

			expr = expr_Const(ins);
			if(expr != NULL)
			{
				if(mode->Outer.pIndexScale != NULL)
				{
					expr = expr_Or(expr, expr_Shl(mode->Outer.pIndexScale, expr_Const(9)));
				}
				sect_OutputExpr16(expr);

				if(mode->Outer.pDisp != NULL)
				{
					switch(mode->Outer.eDispSize)
					{
						default:
						case SIZE_WORD:
							sect_OutputExpr16(mode->Outer.pDisp);
							break;
						case SIZE_LONG:
							sect_OutputExprLong(mode->Outer.pDisp);
							break;
					}
				}

				return true;
			}
			return false;
		}

		case AM_PREINDPCXD020:
		{
			if(mode->Inner.pDisp)
				mode->Inner.pDisp = expr_PcRelative(mode->Inner.pDisp, 2);
		}
		// fall through
		case AM_PREINDAXD020:
		{
			uint16_t ins = 0x0100;
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
						return false;
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
						return false;
				}
			}
			else
			{
				ins |= 0x0001;
			}

			expr = expr_Const(ins);
			if(expr != NULL)
			{
				if(mode->Inner.pIndexScale != NULL)
				{
					expr = expr_Or(expr, expr_Shl(mode->Inner.pIndexScale, expr_Const(9)));
				}
				sect_OutputExpr16(expr);

				if(mode->Inner.pDisp != NULL)
				{
					switch(mode->Inner.eDispSize)
					{
						default:
						case SIZE_WORD:
							sect_OutputExpr16(mode->Inner.pDisp);
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
							sect_OutputExpr16(mode->Outer.pDisp);
							break;
						case SIZE_LONG:
							sect_OutputExprLong(mode->Outer.pDisp);
							break;
					}
				}

				return true;
			}
			return false;
		}

		case AM_POSTINDPCXD020:
		{
			if(mode->Inner.pDisp)
				mode->Inner.pDisp = expr_PcRelative(mode->Inner.pDisp, 2);
		}
		// fall through
		case AM_POSTINDAXD020:
		{
			uint16_t ins = 0x0100;
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
						return false;
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
						return false;
				}
			}
			else
			{
				ins |= 0x0005;
			}

			expr = expr_Const(ins);
			if(expr != NULL)
			{
				if(mode->Outer.pIndexScale != NULL)
				{
					expr = expr_Or(expr, expr_Shl(mode->Outer.pIndexScale, expr_Const(9)));
				}
				sect_OutputExpr16(expr);

				if(mode->Inner.pDisp != NULL)
				{
					switch(mode->Inner.eDispSize)
					{
						default:
						case SIZE_WORD:
							sect_OutputExpr16(mode->Inner.pDisp);
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
							sect_OutputExpr16(mode->Outer.pDisp);
							break;
						case SIZE_LONG:
							sect_OutputExprLong(mode->Outer.pDisp);
							break;
					}
				}

				return true;
			}
			return false;
		}

		default:
			internalerror("unsupported adressing mode");
			return false;

	}

	return false;
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


static bool_t parse_SingleOpIns(uint16_t ins, ESize sz, SAddrMode* src)
{
	// CLR, TST
	sect_OutputConst16((uint16_t)(ins | parse_GetSizeField(sz) << 6 | parse_GetEAField(src)));
	parse_OutputExtWords(src);
	return true;
}



static bool_t parse_xBCD(uint16_t ins, ESize sz, SAddrMode* src, SAddrMode* dest)
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

	sect_OutputConst16(ins);

	return true;
}

static bool_t parse_ABCD(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_xBCD(0xC100, sz, src, dest);
}

static bool_t parse_SBCD(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_xBCD(0x8100, sz, src, dest);
}

static bool_t parse_ADDX(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_xBCD(0xD100, sz, src, dest);
}

static bool_t parse_SUBX(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_xBCD(0x9100, sz, src, dest);
}

static bool_t parse_xxxQ(uint16_t ins, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	SExpression* expr;

	src->pImmediate = expr_CheckRange(src->pImmediate, 1, 8);
	if(src->pImmediate == NULL)
	{
		prj_Error(ERROR_OPERAND_RANGE);
		return true;
	}

	ins |= (uint16_t)(parse_GetEAField(dest) | (parse_GetSizeField(sz) << 6));

	expr = expr_Const(ins);
	expr = expr_Or(expr, expr_Shl(expr_And(src->pImmediate, expr_Const(7)), expr_Const(9)));

	sect_OutputExpr16(expr);
	return parse_OutputExtWords(dest);
}

static bool_t parse_ADDQ(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_xxxQ(0x5000, sz, src, dest);
}

static bool_t parse_SUBQ(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_xxxQ(0x5100, sz, src, dest);
}

static bool_t parse_ADDA(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	uint16_t ins;

	if(src->eMode == AM_IMM
	&& expr_IsConstant(src->pImmediate)
	&& src->pImmediate->Value.Value >= 1
	&& src->pImmediate->Value.Value <= 8)
	{
		return parse_IntegerOp(T_68K_ADDQ, sz, src, dest);
	}

	ins = (uint16_t)(0xD000 | dest->nDirectReg << 9 | parse_GetEAField(src));
	if(sz == SIZE_WORD)
		ins |= 0x3 << 6;
	else
		ins |= 0x7 << 6;

	sect_OutputConst16(ins);
	return parse_OutputExtWords(src);
}

static bool_t parse_SUBA(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	uint16_t ins;

	if(src->eMode == AM_IMM
	&& expr_IsConstant(src->pImmediate)
	&& src->pImmediate->Value.Value >= 1
	&& src->pImmediate->Value.Value <= 8)
	{
		return parse_IntegerOp(T_68K_SUBQ, sz, src, dest);
	}

	ins = (uint16_t)(0x9000 | dest->nDirectReg << 9 | parse_GetEAField(src));
	if(sz == SIZE_WORD)
		ins |= 0x3 << 6;
	else
		ins |= 0x7 << 6;

	sect_OutputConst16(ins);
	return parse_OutputExtWords(src);
}

static bool_t parse_ArithmeticLogicalI(uint16_t ins, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	ins |= parse_GetEAField(dest);
	if(sz == SIZE_BYTE)
	{
		ins |= 0x0 << 6;
		sect_OutputConst16(ins);
		sect_OutputExpr16(expr_And(parse_Check8bit(src->pImmediate), expr_Const(0xFF)));
	}
	else if(sz == SIZE_WORD)
	{
		ins |= 0x1 << 6;
		sect_OutputConst16(ins);
		sect_OutputExpr16(parse_Check16bit(src->pImmediate));
	}
	else
	{
		ins |= 0x2 << 6;
		sect_OutputConst16(ins);
		sect_OutputExprLong(src->pImmediate);
	}

	return parse_OutputExtWords(dest);
}

static bool_t parse_ADDI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(src->eMode == AM_IMM
	&& expr_IsConstant(src->pImmediate)
	&& src->pImmediate->Value.Value >= 1
	&& src->pImmediate->Value.Value <= 8)
	{
		return parse_IntegerOp(T_68K_ADDQ, sz, src, dest);
	}

	return parse_ArithmeticLogicalI(0x0600, sz, src, dest);
}

static bool_t parse_SUBI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(src->eMode == AM_IMM
	&& expr_IsConstant(src->pImmediate)
	&& src->pImmediate->Value.Value >= 1
	&& src->pImmediate->Value.Value <= 8)
	{
		return parse_IntegerOp(T_68K_SUBQ, sz, src, dest);
	}

	return parse_ArithmeticLogicalI(0x0400, sz, src, dest);
}

static bool_t parse_ANDI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(dest->eMode == AM_SYSREG)
	{
		if(sz != SIZE_WORD)
		{
			prj_Error(MERROR_INSTRUCTION_SIZE);
			return true;
		}

		if(dest->nDirectReg == T_68K_REG_CCR)
		{
			sect_OutputConst16(0x023C);
			sect_OutputExpr16(expr_And(src->pImmediate, expr_Const(0xFF)));
			return true;
		}
		else if(dest->nDirectReg == T_68K_REG_SR)
		{
			prj_Warn(MERROR_INSTRUCTION_PRIV);
			sect_OutputConst16(0x027C);
			sect_OutputExpr16(src->pImmediate);
			return true;
		}
		prj_Error(ERROR_DEST_OPERAND);
		return true;
	}

	return parse_ArithmeticLogicalI(0x0200, sz, src, dest);
}

static bool_t parse_ArithmeticLogical(uint16_t ins, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(dest->eMode == AM_DREG)
	{
		if(src->eMode == AM_AREG
		&& sz != SIZE_WORD
		&& sz != SIZE_LONG)
		{
			prj_Error(MERROR_INSTRUCTION_SIZE);
			return true;
		}

		ins |= (uint16_t)(0x0000 | dest->nDirectReg << 9 | parse_GetSizeField(sz) << 6);
		ins |= parse_GetEAField(src);
		sect_OutputConst16(ins);
		return parse_OutputExtWords(src);
	}
	else if(src->eMode == AM_DREG)
	{
		ins |= (uint16_t)(0x0100 | src->nDirectReg << 9 | parse_GetSizeField(sz) << 6);
		ins |= parse_GetEAField(dest);
		sect_OutputConst16(ins);
		return parse_OutputExtWords(dest);
	}

	prj_Error(ERROR_OPERAND);
	return true;
}

static bool_t parse_ADD(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(dest->eMode == AM_AREG)
		return parse_IntegerOp(T_68K_ADDA, sz, src, dest);

	if(src->eMode == AM_IMM)
		return parse_IntegerOp(T_68K_ADDI, sz, src, dest);

	return parse_ArithmeticLogical(0xD000, sz, src, dest);
}

static bool_t parse_SUB(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(dest->eMode == AM_AREG)
		return parse_IntegerOp(T_68K_SUBA, sz, src, dest);

	if(src->eMode == AM_IMM)
		return parse_IntegerOp(T_68K_SUBI, sz, src, dest);

	return parse_ArithmeticLogical(0x9000, sz, src, dest);
}

static bool_t parse_CMPA(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	uint16_t ins;

	ins = (uint16_t)(0xB040 | dest->nDirectReg << 9 | parse_GetEAField(src));
	if(sz == SIZE_WORD)
		ins |= 0x3 << 6;
	else /*if(sz == SIZE_LONG)*/
		ins |= 0x7 << 6;

	sect_OutputConst16(ins);
	return parse_OutputExtWords(src);
}

static bool_t parse_CMPI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	uint16_t ins;

	ins = (uint16_t)(0x0C00 | parse_GetSizeField(sz) << 6 | parse_GetEAField(dest));
	sect_OutputConst16(ins);
	
	if(sz == SIZE_BYTE)
	{
		SExpression* expr = parse_Check8bit(src->pImmediate);
		if(expr == NULL)
		{
			prj_Error(ERROR_OPERAND_RANGE);
			return true;
		}
		sect_OutputExpr16(expr_And(expr, expr_Const(0xFF)));
	}
	else if(sz == SIZE_WORD)
	{
		SExpression* expr = parse_Check16bit(src->pImmediate);
		if(expr == NULL)
		{
			prj_Error(ERROR_OPERAND_RANGE);
			return true;
		}
		sect_OutputExpr16(expr);
	}
	else if(sz == SIZE_WORD)
	{
		sect_OutputExprLong(src->pImmediate);
	}
	return parse_OutputExtWords(dest);
}

static bool_t parse_CMPM(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	uint16_t ins;

	ins = (uint16_t)(0xB108 | dest->Outer.nBaseReg << 9 | src->Outer.nBaseReg | parse_GetSizeField(sz) << 6);
	sect_OutputConst16(ins);
	return true;
}

static bool_t parse_CMP(ESize sz, SAddrMode* src, SAddrMode* dest)
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
	return false;
}

static bool_t parse_AND(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(src->eMode == AM_IMM || dest->eMode == AM_SYSREG)
		return parse_IntegerOp(T_68K_ANDI, sz, src, dest);

	return parse_ArithmeticLogical(0xC000, sz, src, dest);
}

static bool_t parse_CLR(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_SingleOpIns(0x4200, sz, src);
}

static bool_t parse_TST(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(src->eMode == AM_AREG
	&& sz == SIZE_BYTE)
	{
		prj_Error(MERROR_INSTRUCTION_SIZE);
		return true;
	}

	return parse_SingleOpIns(0x4A00, sz, src);
}

static bool_t parse_Shift(uint16_t ins, uint16_t memins, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(dest->eMode == AM_DREG)
	{
		ins |= 0x0000 | parse_GetSizeField(sz) << 6 | dest->nDirectReg;
		if(src->eMode == AM_IMM)
		{
			SExpression* expr;
			expr = expr_CheckRange(src->pImmediate, 1, 8);
			expr = expr_And(expr, expr_Const(7));
			if(expr == NULL)
			{
				prj_Error(ERROR_OPERAND_RANGE);
				return true;
			}
			expr = expr_Or(expr_Const(ins), expr_Shl(expr, expr_Const(9)));
			sect_OutputExpr16(expr);
		}
		else if(src->eMode == AM_DREG)
		{
			ins |= 0x0020 | src->nDirectReg << 9;
			sect_OutputConst16(ins);
		}
		return true;
	}
	if(dest->eMode != AM_EMPTY)
	{
		prj_Error(ERROR_DEST_OPERAND);
		return true;
	}
	if(src->eMode & (AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_POSTINDAXD020 | AM_PREINDAXD020))
	{
		if(sz != SIZE_WORD)
		{
			prj_Error(MERROR_INSTRUCTION_SIZE);
			return true;
		}

		memins |= parse_GetEAField(src);
		sect_OutputConst16(memins);
		return parse_OutputExtWords(src);
	}

	prj_Error(ERROR_SOURCE_OPERAND);
	return true;
}

static bool_t parse_ASL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Shift(0xE100, 0xE1C0, sz, src, dest);
}

static bool_t parse_ASR(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Shift(0xE000, 0xE0C0, sz, src, dest);
}

static bool_t parse_LSL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Shift(0xE108, 0xE3C0, sz, src, dest);
}

static bool_t parse_LSR(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Shift(0xE008, 0xE2C0, sz, src, dest);
}

static bool_t parse_ROL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Shift(0xE118, 0xE7C0, sz, src, dest);
}

static bool_t parse_ROR(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Shift(0xE018, 0xE6C0, sz, src, dest);
}

static bool_t parse_ROXL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Shift(0xE110, 0xE5C0, sz, src, dest);
}

static bool_t parse_ROXR(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Shift(0xE010, 0xE4C0, sz, src, dest);
}

static bool_t parse_Bcc(uint16_t ins, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	ins = 0x6000 | ins << 8;
	if(sz == SIZE_BYTE)
	{
		SExpression* expr = expr_CheckRange(expr_PcRelative(src->Outer.pDisp, -2), -128, 127);
		if(expr != NULL)
		{
			expr = expr_And(expr, expr_Const(0xFF));
			expr = expr_Or(expr, expr_Const(ins));
			sect_OutputExpr16(expr);
			return true;
		}

		prj_Error(ERROR_OPERAND_RANGE);
		return true;
	}
	else if(sz == SIZE_WORD)
	{
		SExpression* expr = expr_CheckRange(expr_PcRelative(src->Outer.pDisp, 0), -32768, 32767);
		if(expr != NULL)
		{
			sect_OutputConst16(ins);
			sect_OutputExpr16(expr);
			return true;
		}

		prj_Error(ERROR_OPERAND_RANGE);
		return true;
	}
	else if(sz == SIZE_LONG)
	{
		SExpression* expr;

		if(g_pOptions->pMachine->nCpu < CPUF_68020)
		{
			prj_Error(MERROR_INSTRUCTION_SIZE);
			return true;
		}

		expr = expr_PcRelative(src->Outer.pDisp, 0);
		sect_OutputConst16(ins | 0xFF);
		sect_OutputExprLong(expr);
		return true;
	}


	return true;
}

static bool_t parse_BRA(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0x0, sz, src, dest);
}

static bool_t parse_BSR(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0x1, sz, src, dest);
}

static bool_t parse_BHI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0x2, sz, src, dest);
}

static bool_t parse_BLS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0x3, sz, src, dest);
}

static bool_t parse_BCC(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0x4, sz, src, dest);
}

static bool_t parse_BCS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0x5, sz, src, dest);
}

static bool_t parse_BNE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0x6, sz, src, dest);
}

static bool_t parse_BEQ(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0x7, sz, src, dest);
}

static bool_t parse_BVC(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0x8, sz, src, dest);
}

static bool_t parse_BVS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0x9, sz, src, dest);
}

static bool_t parse_BPL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0xA, sz, src, dest);
}

static bool_t parse_BMI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0xB, sz, src, dest);
}

static bool_t parse_BGE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0xC, sz, src, dest);
}

static bool_t parse_BLT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0xD, sz, src, dest);
}

static bool_t parse_BGT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0xE, sz, src, dest);
}

static bool_t parse_BLE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Bcc(0xF, sz, src, dest);
}

static bool_t parse_BitInstruction(uint16_t dins, uint16_t immins, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(src->eMode == AM_DREG)
	{
		dins |= src->nDirectReg << 9 | parse_GetEAField(dest);
		sect_OutputConst16(dins);
		return parse_OutputExtWords(dest);
	}
	else if(src->eMode == AM_IMM)
	{
		SExpression* expr;

		immins |= parse_GetEAField(dest);
		sect_OutputConst16(immins);

		if(dest->eMode == AM_DREG)
			expr = expr_CheckRange(src->pImmediate, 0, 31);
		else
			expr = expr_CheckRange(src->pImmediate, 0, 7);

		if(expr != NULL)
		{
			sect_OutputExpr16(expr);
			return parse_OutputExtWords(dest);
		}
		prj_Error(ERROR_OPERAND_RANGE);
	}
	return true;
}

static bool_t parse_BCHG(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_BitInstruction(0x0140, 0x0840, sz, src, dest);
}

static bool_t parse_BCLR(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_BitInstruction(0x0180, 0x0880, sz, src, dest);
}

static bool_t parse_BSET(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_BitInstruction(0x01C0, 0x08C0, sz, src, dest);
}

static bool_t parse_BTST(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_BitInstruction(0x0100, 0x0800, sz, src, dest);
}

static bool_t parse_BitfieldInstruction(uint16_t ins, uint16_t ext, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	SExpression* expr = expr_Const(ext);

	ins |= parse_GetEAField(src);
	sect_OutputConst16(ins);

	if(src->nBFOffsetReg != -1)
		expr = expr_Or(expr, expr_Const(0x0800 | src->nBFOffsetReg << 6));
	else
	{
		SExpression* bf = expr_CheckRange(src->pBFOffsetExpr, 0, 31);
		if(bf == NULL)
		{
			prj_Error(ERROR_OPERAND_RANGE);
			return true;
		}
		expr = expr_Or(expr, expr_Shl(bf, expr_Const(6)));
	}

	if(src->nBFWidthReg != -1)
		expr = expr_Or(expr, expr_Const(0x0020 | src->nBFWidthReg));
	else
	{
		SExpression* bf = expr_CheckRange(src->pBFWidthExpr, 0, 31);
		if(bf == NULL)
		{
			prj_Error(ERROR_OPERAND_RANGE);
			return true;
		}
		expr = expr_Or(expr, bf);
	}

	sect_OutputExpr16(expr);
	return parse_OutputExtWords(src);
}

static bool_t parse_SingleOpBitfieldInstruction(uint16_t ins, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_BitfieldInstruction(ins, 0, sz, src, dest);
}

static bool_t parse_BFCHG(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_SingleOpBitfieldInstruction(0xEAC0, sz, src, dest);
}

static bool_t parse_BFCLR(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_SingleOpBitfieldInstruction(0xECC0, sz, src, dest);
}

static bool_t parse_BFSET(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_SingleOpBitfieldInstruction(0xEEC0, sz, src, dest);
}

static bool_t parse_BFTST(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_SingleOpBitfieldInstruction(0xE8C0, sz, src, dest);
}

static bool_t parse_BFEXTS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_BitfieldInstruction(0xEBC0, (uint16_t)(dest->nDirectReg << 12), sz, src, dest);
}

static bool_t parse_BFEXTU(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_BitfieldInstruction(0xE9C0, (uint16_t)(dest->nDirectReg << 12), sz, src, dest);
}

static bool_t parse_BFFFO(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_BitfieldInstruction(0xEDC0, (uint16_t)(dest->nDirectReg << 12), sz, src, dest);
}

static bool_t parse_BFINS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_BitfieldInstruction(0xEFC0, (uint16_t)(src->nDirectReg << 12), sz, dest, src);
}

static bool_t parse_BKPT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	SExpression* expr = expr_CheckRange(src->pImmediate, 0, 7);
	if(expr == NULL)
	{
		prj_Error(ERROR_OPERAND_RANGE);
		return true;
	}

	expr = expr_Or(expr, expr_Const(0x4848));
	sect_OutputExpr16(expr);
	return true;
}

static bool_t parse_CALLM(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	SExpression* expr = expr_CheckRange(src->pImmediate, 0, 255);
	if(expr == NULL)
	{
		prj_Error(ERROR_OPERAND_RANGE);
		return true;
	}

	sect_OutputConst16(0x06C0 | (uint16_t)parse_GetEAField(dest));
	sect_OutputExpr16(expr);
	return parse_OutputExtWords(dest);
}

static bool_t parse_CAS(ESize sz, SAddrMode* dc, SAddrMode* du)
{
	uint16_t ins;
	SAddrMode ea;

	if(g_pOptions->pMachine->nCpu == CPUF_68060
	&& sz != SIZE_BYTE)
	{
		prj_Error(MERROR_INSTRUCTION_CPU);
		return true;
	}

	if(!parse_ExpectComma())
		return false;
	if(!parse_GetAddrMode(&ea))
		return false;

	if((ea.eMode & (AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020)) == 0)
	{
		prj_Error(ERROR_OPERAND);
		return true;
	}

	ins = (uint16_t)(0x08C0 | (parse_GetSizeField(sz) + 1) << 9 | parse_GetEAField(&ea));
	sect_OutputConst16(ins);

	ins = (uint16_t)(0x0000 | du->nDirectReg << 6 | dc->nDirectReg);
	sect_OutputConst16(ins);

	return parse_OutputExtWords(&ea);
}

static bool_t parse_GetDataRegister(uint16_t* pReg)
{
	if(g_CurrentToken.ID.TargetToken >= T_68K_REG_D0
	&& g_CurrentToken.ID.TargetToken <= T_68K_REG_D7)
	{
		*pReg = (uint16_t)(g_CurrentToken.ID.TargetToken - T_68K_REG_D0);
		parse_GetToken();
		return true;
	}

	return false;
}

static bool_t parse_GetAddressRegister(uint16_t* pReg)
{
	if(g_CurrentToken.ID.TargetToken >= T_68K_REG_A0
	&& g_CurrentToken.ID.TargetToken <= T_68K_REG_A7)
	{
		*pReg = (uint16_t)(g_CurrentToken.ID.TargetToken - T_68K_REG_A0);
		parse_GetToken();
		return true;
	}

	return false;
}

static bool_t parse_GetRegister(uint16_t* pReg)
{
	if(parse_GetDataRegister(pReg))
		return true;

	if(parse_GetAddressRegister(pReg))
	{
		*pReg += 8;
		return true;
	}

	return false;
}

static bool_t parse_ExpectDataRegister(uint16_t* pReg)
{
	if(g_CurrentToken.ID.TargetToken >= T_68K_REG_D0
	&& g_CurrentToken.ID.TargetToken <= T_68K_REG_D7)
	{
		*pReg = (uint16_t)(g_CurrentToken.ID.TargetToken - T_68K_REG_D0);
		parse_GetToken();
		return true;
	}

	prj_Error(ERROR_OPERAND);
	return false;
}

static bool_t parse_ExpectIndirectRegister(uint16_t* pReg)
{
	if(g_CurrentToken.ID.TargetToken >= T_68K_REG_A0_IND
	&& g_CurrentToken.ID.TargetToken <= T_68K_REG_A7_IND)
	{
		*pReg = (uint16_t)(g_CurrentToken.ID.TargetToken - T_68K_REG_A0_IND + 8);
		parse_GetToken();
		return true;
	}

	if(!parse_ExpectChar('('))
		return false;

	if(!parse_ExpectDataRegister(pReg))
		return false;

	if(!parse_ExpectChar(')'))
		return false;

	return true;
}

static bool_t parse_CAS2(ESize sz, SAddrMode* unused1, SAddrMode* unused2)
{
	uint16_t dc1, dc2, du1, du2, rn1, rn2;

	if(!parse_ExpectDataRegister(&dc1))
		return false;

	if(!parse_ExpectChar(':'))
		return false;

	if(!parse_ExpectDataRegister(&dc2))
		return false;

	if(!parse_ExpectComma())
		return false;

	if(!parse_ExpectDataRegister(&du1))
		return false;

	if(!parse_ExpectChar(':'))
		return false;

	if(!parse_ExpectDataRegister(&du2))
		return false;

	if(!parse_ExpectComma())
		return false;

	if(!parse_ExpectIndirectRegister(&rn1))
		return false;

	if(!parse_ExpectChar(':'))
		return false;

	if(!parse_ExpectIndirectRegister(&rn2))
		return false;

	sect_OutputConst16(0x08FC | (uint16_t)(parse_GetSizeField(sz) + 1) << 9);
	sect_OutputConst16(rn1 << 12 | du1 << 6 | dc1);
	sect_OutputConst16(rn2 << 12 | du2 << 6 | dc2);

	return true;
}

static bool_t parse_CHK(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	uint16_t ins;

	if(sz == SIZE_LONG
	&& g_pOptions->pMachine->nCpu < CPUF_68020)
	{
		prj_Error(MERROR_INSTRUCTION_SIZE);
		return true;
	}

	ins = (uint16_t)(0x4000 | dest->nDirectReg << 9 | parse_GetEAField(src));
	if(sz == SIZE_WORD)
		ins |= 0x3 << 7;
	else /*if(sz == SIZE_LONG)*/
		ins |= 0x2 << 7;

	sect_OutputConst16(ins);
	return parse_OutputExtWords(src);
}

static bool_t parse_CHK2(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	uint16_t ins;

	ins = (uint16_t)(0x00C0 | parse_GetSizeField(sz) << 9 | parse_GetEAField(src));
	sect_OutputConst16(ins);

	ins = (uint16_t)(0x0800 | dest->nDirectReg << 12);
	sect_OutputConst16(ins);

	return parse_OutputExtWords(src);
}

static bool_t parse_CMP2(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	uint16_t ins;
	
	ins = (uint16_t)(0x00C0 | parse_GetSizeField(sz) << 9 | parse_GetEAField(src));
	sect_OutputConst16(ins);

	ins = (uint16_t)(dest->nDirectReg << 12);
	if(dest->eMode == AM_AREG)
		ins |= 0x8000;
	sect_OutputConst16(ins);
	return true;
}

static bool_t parse_DBcc(uint16_t code, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	code = (uint16_t)(0x50C8 | code << 8 | src->nDirectReg);
	sect_OutputConst16(code);
	sect_OutputExpr16(expr_PcRelative(dest->Outer.pDisp, 0));
	return true;
}

static bool_t parse_DBT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0x0, sz, src, dest);
}

static bool_t parse_DBF(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0x1, sz, src, dest);
}

static bool_t parse_DBHI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0x2, sz, src, dest);
}

static bool_t parse_DBLS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0x3, sz, src, dest);
}

static bool_t parse_DBCC(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0x4, sz, src, dest);
}

static bool_t parse_DBCS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0x5, sz, src, dest);
}

static bool_t parse_DBNE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0x6, sz, src, dest);
}

static bool_t parse_DBEQ(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0x7, sz, src, dest);
}

static bool_t parse_DBVC(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0x8, sz, src, dest);
}

static bool_t parse_DBVS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0x9, sz, src, dest);
}

static bool_t parse_DBPL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0xA, sz, src, dest);
}

static bool_t parse_DBMI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0xB, sz, src, dest);
}

static bool_t parse_DBGE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0xC, sz, src, dest);
}

static bool_t parse_DBLT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0xD, sz, src, dest);
}

static bool_t parse_DBGT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0xE, sz, src, dest);
}

static bool_t parse_DBLE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DBcc(0xF, sz, src, dest);
}

static bool_t parse_DIVxx(bool_t sign, bool_t l, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(l || sz == SIZE_LONG)
	{
		bool_t div64;
		int dq, dr;
		if(g_CurrentToken.ID.TargetToken == ':')
		{
			uint16_t reg;
			parse_GetToken();
			if(!parse_ExpectDataRegister(&reg))
				return false;

			dq = reg;
			dr = dest->nDirectReg;
			div64 = !l;
		}
		else
		{
			dq = dest->nDirectReg;
			dr = dq;
			div64 = false;
		}

		if(g_pOptions->pMachine->nCpu >= CPUF_68060
		&& div64)
		{
			prj_Error(MERROR_INSTRUCTION_CPU);
			return true;
		}

		sect_OutputConst16((uint16_t)(0x4C40 | parse_GetEAField(src)));
		sect_OutputConst16((uint16_t)(sign << 11 | div64 << 10 | dq << 12 | dr));
		return parse_OutputExtWords(src);
	}
	else
	{
		uint16_t ins = (uint16_t)(0x80C0 | sign << 8 | dest->nDirectReg << 9 | parse_GetEAField(src));
		sect_OutputConst16(ins);
		return parse_OutputExtWords(src);
	}
}

static bool_t parse_DIVS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DIVxx(true, false, sz, src, dest);
}

static bool_t parse_DIVSL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DIVxx(true, true, sz, src, dest);
}

static bool_t parse_DIVU(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DIVxx(false, false, sz, src, dest);
}

static bool_t parse_DIVUL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_DIVxx(false, true, sz, src, dest);
}

static bool_t parse_EOR(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	uint16_t ins;

	if(src->eMode == AM_IMM || dest->eMode == AM_SYSREG)
		return parse_IntegerOp(T_68K_EORI, sz, src, dest);

	ins = (uint16_t)(0xB100 | src->nDirectReg << 9 | parse_GetEAField(dest) | parse_GetSizeField(sz) << 6);
	sect_OutputConst16(ins);
	return parse_OutputExtWords(dest);
}

static bool_t parse_EORI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(dest->eMode == AM_SYSREG)
	{
		if(sz != SIZE_WORD)
		{
			prj_Error(MERROR_INSTRUCTION_SIZE);
			return true;
		}

		if(dest->nDirectReg == T_68K_REG_CCR)
		{
			sect_OutputConst16(0x0A3C);
			sect_OutputExpr16(expr_And(src->pImmediate, expr_Const(0xFF)));
			return true;
		}
		else if(dest->nDirectReg == T_68K_REG_SR)
		{
			prj_Warn(MERROR_INSTRUCTION_PRIV);
			sect_OutputConst16(0x0A7C);
			sect_OutputExpr16(src->pImmediate);
			return true;
		}

		prj_Error(ERROR_DEST_OPERAND);
		return true;
	}

	return parse_ArithmeticLogicalI(0x0A00, sz, src, dest);
}

static bool_t parse_EXG(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	uint16_t ins;
	uint16_t rx, ry;

	if(src->eMode != dest->eMode)
	{
		ins = 0x11 << 3;
		if(src->eMode == AM_AREG)
		{
			rx = (uint16_t)dest->nDirectReg;
			ry = (uint16_t)src->nDirectReg;
		}
		else
		{
			rx = (uint16_t)src->nDirectReg;
			ry = (uint16_t)dest->nDirectReg;
		}
	}
	else
	{
		rx = (uint16_t)src->nDirectReg;
		ry = (uint16_t)dest->nDirectReg;
		if(src->eMode == AM_DREG)
			ins = 0x08 << 3;
		else
			ins = 0x09 << 3;
	}

	sect_OutputConst16(ins | 0xC100 | rx << 9 | ry);
	return true;
}

static bool_t parse_EXT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	uint16_t ins = (uint16_t)(0x4800 | src->nDirectReg);
	if(sz == SIZE_WORD)
		ins |= 0x2 << 6;
	else
		ins |= 0x3 << 6;

	sect_OutputConst16(ins);
	return true;
}

static bool_t parse_EXTB(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputConst16((uint16_t)(0x4800 | src->nDirectReg | 0x7 << 6));
	return true;
}

static bool_t parse_ILLEGAL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputConst16(0x4AFC);
	return true;
}

static bool_t parse_Jxx(uint16_t ins, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	ins |= parse_GetEAField(src);
	sect_OutputConst16(ins);
	return parse_OutputExtWords(src);
}

static bool_t parse_JMP(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Jxx(0x4EC0, sz, src, dest);
}

static bool_t parse_JSR(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Jxx(0x4E80, sz, src, dest);
}

static bool_t parse_LEA(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputConst16((uint16_t)(0x41C0 | dest->nDirectReg << 9 | parse_GetEAField(src)));
	return parse_OutputExtWords(src);
}

static bool_t parse_LINK(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(sz == SIZE_LONG
	&& g_pOptions->pMachine->nCpu < CPUF_68020)
	{
		prj_Error(MERROR_INSTRUCTION_SIZE);
		return true;
	}

	if(sz == SIZE_WORD)
	{
		sect_OutputConst16((uint16_t)(0x4E50 | src->nDirectReg));
		sect_OutputExpr16(dest->pImmediate);
		return true;
	}
	else /*if(sz == SIZE_LONG)*/
	{
		sect_OutputConst16((uint16_t)(0x4808 | src->nDirectReg));
		sect_OutputExprLong(dest->pImmediate);
		return true;
	}
}

static bool_t parse_MOVEfromSYSREG(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(src->nDirectReg == T_68K_REG_USP)
	{
		prj_Warn(MERROR_INSTRUCTION_PRIV);

		if(sz != SIZE_LONG)
		{
			prj_Error(MERROR_INSTRUCTION_SIZE);
			return true;
		}

		if(dest->eMode != AM_AREG)
		{
			prj_Error(ERROR_DEST_OPERAND);
			return true;
		}

		sect_OutputConst16((uint16_t)(0x4E68 | dest->nDirectReg));
		return true;
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
			return true;
		}

		if(src->nDirectReg == T_68K_REG_CCR)
		{
			if(g_pOptions->pMachine->nCpu < CPUF_68010)
			{
				prj_Error(MERROR_INSTRUCTION_CPU);
				return true;
			}

			if(sz != SIZE_WORD)
			{
				prj_Error(MERROR_INSTRUCTION_SIZE);
				return true;
			}

			sect_OutputConst16((uint16_t)(0x42C0 | parse_GetEAField(dest)));
			return true;
		}
		else if(src->nDirectReg == T_68K_REG_SR)
		{
			if(g_pOptions->pMachine->nCpu >= CPUF_68010)
				prj_Warn(MERROR_INSTRUCTION_PRIV);

			if(sz != SIZE_WORD)
			{
				prj_Error(MERROR_INSTRUCTION_SIZE);
				return true;
			}

			sect_OutputConst16((uint16_t)(0x40C0 | parse_GetEAField(dest)));
			return true;
		}
	}

	return true;
}

static bool_t parse_MOVEtoSYSREG(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(dest->nDirectReg == T_68K_REG_USP)
	{
		prj_Warn(MERROR_INSTRUCTION_PRIV);

		if(sz != SIZE_LONG)
		{
			prj_Error(MERROR_INSTRUCTION_SIZE);
			return true;
		}

		if(src->eMode != AM_AREG)
		{
			prj_Error(ERROR_SOURCE_OPERAND);
			return true;
		}

		sect_OutputConst16((uint16_t)(0x4E60 | src->nDirectReg));
		return true;
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
			return true;
		}

		if(dest->nDirectReg == T_68K_REG_CCR)
		{
			if(sz != SIZE_WORD)
			{
				prj_Error(MERROR_INSTRUCTION_SIZE);
				return true;
			}

			sect_OutputConst16((uint16_t)(0x44C0 | parse_GetEAField(src)));
			return true;
		}
		else if(dest->nDirectReg == T_68K_REG_SR)
		{
			if(sz != SIZE_WORD)
			{
				prj_Error(MERROR_INSTRUCTION_SIZE);
				return true;
			}

			prj_Warn(MERROR_INSTRUCTION_PRIV);

			sect_OutputConst16((uint16_t)(0x46C0 | parse_GetEAField(src)));
			return true;
		}
	}

	return true;
}

		

static bool_t parse_MOVE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	uint16_t destea;
	uint16_t ins;

	if(src->eMode == AM_IMM
	&& dest->eMode == AM_DREG && sz == SIZE_LONG
	&& expr_IsConstant(src->pImmediate)
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

	destea = (uint16_t)parse_GetEAField(dest);
	ins = (uint16_t)parse_GetEAField(src);

	destea = (destea >> 3 | destea << 3) & 0x3F;

	ins |= destea << 6;
	if(sz == SIZE_BYTE)
		ins |= 0x1 << 12;
	else if(sz == SIZE_WORD)
		ins |= 0x3 << 12;
	else /*if(sz == SIZE_LONG)*/
		ins |= 0x2 << 12;

	sect_OutputConst16(ins);
	if(!parse_OutputExtWords(src))
		return false;
	return parse_OutputExtWords(dest);
}

static bool_t parse_MOVEA(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	uint16_t ins = (uint16_t)(0x0040 | parse_GetEAField(src) | dest->nDirectReg << 9);

	if(sz == SIZE_WORD)
		ins |= 0x3 << 12;
	else /*if(sz == SIZE_LONG)*/
		ins |= 0x2 << 12;

	sect_OutputConst16(ins);
	return parse_OutputExtWords(src);
}

static bool_t parse_MOVE16(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	uint16_t opmode;
	SExpression *line;
	uint16_t reg;

	if(src->eMode == AM_AINC && dest->eMode == AM_AINC)
	{
		sect_OutputConst16((uint16_t)(0xF620 | src->Outer.nBaseReg));
		sect_OutputConst16((uint16_t)(0x8000 | dest->Outer.nBaseReg << 12));
		return true;
	}

	if(src->eMode == AM_AINC && dest->eMode == AM_LONG)
	{
		opmode = 0x0;
		line = dest->Outer.pDisp;
		reg = (uint16_t)src->Outer.nBaseReg;
	}
	else if(src->eMode == AM_LONG && dest->eMode == AM_AINC)
	{
		opmode = 0x1;
		line = src->Outer.pDisp;
		reg = (uint16_t)dest->Outer.nBaseReg;
	}
	else if(src->eMode == AM_AIND && dest->eMode == AM_LONG)
	{
		opmode = 0x2;
		line = dest->Outer.pDisp;
		reg = (uint16_t)src->Outer.nBaseReg;
	}
	else if(src->eMode == AM_LONG && dest->eMode == AM_AIND)
	{
		opmode = 0x3;
		line = src->Outer.pDisp;
		reg = (uint16_t)dest->Outer.nBaseReg;
	}
	else
	{
		prj_Error(ERROR_OPERAND);
		return true;
	}

	sect_OutputConst16(0xF600 | opmode << 3 | reg);
	sect_OutputExprLong(line);
	return true;
}

static bool_t parse_GetRegisterRange(uint16_t* pStart, uint16_t* pEnd)
{
	if(parse_GetRegister(pStart))
	{
		if(g_CurrentToken.ID.TargetToken == T_OP_SUB)
		{
			parse_GetToken();
			if(!parse_GetRegister(pEnd))
				return 0;
			return true;
		}
		*pEnd = *pStart;
		return true;
	}
	return false;
}

#define REGLIST_FAIL 65536

static uint32_t parse_RegisterList(void)
{
	uint16_t r;
	uint16_t start;
	uint16_t end;


	if(g_CurrentToken.ID.TargetToken == '#')
	{
		int32_t expr;
		parse_GetToken();
		expr = parse_ConstantExpression();
		if(expr >= 0 && expr <= 65535)
			return (uint16_t)expr;

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

static uint16_t parse_SwapBits(uint16_t bits)
{
	uint16_t r = 0;
	int i;

	for(i = 0; i < 16; ++i)
		r |= (bits & 1 << i) ? 1 << (15 - i) : 0;

	return r;
}

static bool_t parse_MOVEM(ESize sz, SAddrMode* unused1, SAddrMode* unused2)
{
	uint16_t ins;
	uint16_t dr;
	uint32_t reglist;
	SAddrMode mode;

	reglist = parse_RegisterList();
	if(reglist != REGLIST_FAIL)
	{
		EAddrMode allowdest = AM_AIND | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG;

		if(!parse_ExpectComma())
			return false;

		if(!parse_GetAddrMode(&mode))
			return false;

		if(g_pOptions->pMachine->nCpu >= CPUF_68020)
			allowdest |= AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020;

		if((mode.eMode & allowdest) == 0)
		{
			prj_Error(ERROR_DEST_OPERAND);
			return true;
		}
		dr = 0;
	}
	else
	{
		EAddrMode allowsrc = AM_AIND | AM_AINC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_PCDISP | AM_PCXDISP;

		if(!parse_GetAddrMode(&mode))
			return false;

		if(g_pOptions->pMachine->nCpu >= CPUF_68020)
			allowsrc |= AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020;

		if((mode.eMode & allowsrc) == 0)
		{
			prj_Error(ERROR_SOURCE_OPERAND);
			return true;
		}

		if(!parse_ExpectComma())
			return false;

		reglist = parse_RegisterList();
		if(reglist == REGLIST_FAIL)
			return false;

		dr = 1;
	}

	if(reglist == 0)
	{
		prj_Warn(MERROR_MOVEM_SKIPPED);
		return true;
	}

	ins = (uint16_t)(0x4880 | dr << 10 | parse_GetEAField(&mode));
	if(sz == SIZE_LONG)
		ins |= 1 << 6;

	sect_OutputConst16(ins);
	if(mode.eMode == AM_ADEC)
		reglist = parse_SwapBits((uint16_t)reglist);
	sect_OutputConst16((uint16_t)reglist);
	return parse_OutputExtWords(&mode);
}

static bool_t parse_MOVEP(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	uint16_t dr;
	uint16_t ar;
	uint16_t opmode;
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

		dr = (uint16_t)dest->nDirectReg;
		ar = (uint16_t)src->Outer.nBaseReg;
		disp = src->Outer.pDisp;
	}
	else if(src->eMode == AM_DREG && dest->eMode == AM_ADISP)
	{
		if(sz == SIZE_WORD)
			opmode = 0x6;
		else
			opmode = 0x7;

		dr = (uint16_t)src->nDirectReg;
		ar = (uint16_t)dest->Outer.nBaseReg;
		disp = dest->Outer.pDisp;
	}
	else
	{
		prj_Error(ERROR_OPERAND);
		return true;
	}

	sect_OutputConst16(0x0008 | dr << 9 | opmode << 6 | ar);
	if(disp != NULL)
		sect_OutputExpr16(disp);
	else
		sect_OutputConst16(0);

	return true;
}

static bool_t parse_MOVEQ(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	SExpression* expr = expr_CheckRange(src->pImmediate, -128, 127);
	if(expr == NULL)
	{
		prj_Error(ERROR_OPERAND_RANGE);
		return true;
	}

	expr = expr_And(expr, expr_Const(0xFF));
	expr = expr_Or(expr, expr_Const(0x7000 | dest->nDirectReg << 9));
	sect_OutputExpr16(expr);
	return true;
}

static bool_t parse_MULx(uint16_t sign, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(sz == SIZE_LONG
	&& g_pOptions->pMachine->nCpu < CPUF_68020)
	{
		prj_Error(MERROR_INSTRUCTION_CPU);
		return true;
	}

	if(sz == SIZE_LONG)
	{
		uint16_t dh, dl, mul64;
		if(g_CurrentToken.ID.TargetToken == ':')
		{
			parse_GetToken();
			if(!parse_ExpectDataRegister(&dl))
				return false;
			dh = (uint16_t)dest->nDirectReg;
			mul64 = 1;
			if(dh == dl)
				prj_Warn(MERROR_UNDEFINED_RESULT);

			if(g_pOptions->pMachine->nCpu == CPUF_68060)
			{
				prj_Error(MERROR_INSTRUCTION_CPU);
				return true;
			}
		}
		else
		{
			dl = (uint16_t)dest->nDirectReg;
			dh = 0;
			mul64 = 0;
		}

		sect_OutputConst16((uint16_t)(0x4C00 | parse_GetEAField(src)));
		sect_OutputConst16(0x0000 | sign << 11 | mul64 << 10 | dl << 12 | dh);
		return parse_OutputExtWords(src);
	}
	else
	{
		sect_OutputConst16((uint16_t)(0xC0C0 | sign << 8 | dest->nDirectReg << 9 | parse_GetEAField(src)));
		return parse_OutputExtWords(src);
	}
}


static bool_t parse_MULS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_MULx(true, sz, src, dest);
}

static bool_t parse_MULU(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_MULx(false, sz, src, dest);
}

static bool_t parse_NBCD(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputConst16((uint16_t)(0x4800 | parse_GetEAField(src)));
	return parse_OutputExtWords(src);
}

static bool_t parse_NEG(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputConst16((uint16_t)(0x4400 | parse_GetEAField(src) | parse_GetSizeField(sz) << 6));
	return parse_OutputExtWords(src);
}

static bool_t parse_NEGX(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputConst16((uint16_t)(0x4000 | parse_GetEAField(src) | parse_GetSizeField(sz) << 6));
	return parse_OutputExtWords(src);
}

static bool_t parse_NOP(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputConst16(0x4E71);
	return true;
}

static bool_t parse_NOT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputConst16((uint16_t)(0x4600 | parse_GetEAField(src) | parse_GetSizeField(sz) << 6));
	return parse_OutputExtWords(src);
}

static bool_t parse_ORI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(dest->eMode == AM_SYSREG)
	{
		if(sz != SIZE_WORD)
		{
			prj_Error(MERROR_INSTRUCTION_SIZE);
			return true;
		}
		if(dest->nDirectReg == T_68K_REG_CCR)
		{
			sect_OutputConst16(0x003C);
			sect_OutputExpr16(expr_And(src->pImmediate, expr_Const(0xFF)));
			return true;
		}
		else if(dest->nDirectReg == T_68K_REG_SR)
		{
			prj_Warn(MERROR_INSTRUCTION_PRIV);
			sect_OutputConst16(0x007C);
			sect_OutputExpr16(src->pImmediate);
			return true;
		}
		prj_Error(ERROR_DEST_OPERAND);
		return true;
	}

	return parse_ArithmeticLogicalI(0x0000, sz, src, dest);
}

static bool_t parse_OR(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	if(src->eMode == AM_IMM || dest->eMode == AM_SYSREG)
		return parse_IntegerOp(T_68K_ORI, sz, src, dest);

	return parse_ArithmeticLogical(0x8000, sz, src, dest);
}

static bool_t parse_PackUnpack(uint16_t ins, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	uint16_t dy, dx;
	uint16_t rm;
	SAddrMode adj;

	if(!parse_ExpectComma())
		return false;

	if(!parse_GetAddrMode(&adj))
		return false;

	if(src->eMode != dest->eMode || adj.eMode != AM_IMM)
	{
		prj_Error(ERROR_OPERAND);
		return true;
	}

	if(src->eMode == AM_DREG)
	{
		dx = (uint16_t)src->nDirectReg;
		dy = (uint16_t)dest->nDirectReg;
		rm = 0;
	}
	else
	{
		dx = (uint16_t)src->Outer.nBaseReg;
		dy = (uint16_t)dest->Outer.nBaseReg;
		rm = 1;
	}

	sect_OutputConst16(ins | dy << 9 | rm << 3 | dx);
	sect_OutputExpr16(adj.pImmediate);
	return true;
}

static bool_t parse_PACK(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_PackUnpack(0x8140, sz, src, dest);
}

static bool_t parse_UNPACK(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_PackUnpack(0x8180, sz, src, dest);
}

static bool_t parse_PEA(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputConst16((uint16_t)(0x4840 | parse_GetEAField(src)));
	return parse_OutputExtWords(src);
}

static bool_t parse_RTD(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputConst16(0x4E74);
	sect_OutputExpr16(src->pImmediate);
	return true;
}

static bool_t parse_RTM(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	uint16_t reg;

	if(src->eMode == AM_DREG)
		reg = (uint16_t)src->nDirectReg;
	else /* if(src->eMode == AM_AREG) */
		reg = (uint16_t)src->nDirectReg + 8;

	sect_OutputConst16(0x06C0 | reg);
	return true;
}

static bool_t parse_RTR(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputConst16(0x4E77);
	return true;
}

static bool_t parse_RTS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputConst16(0x4E75);
	return true;
}

static bool_t parse_Scc(uint16_t code, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputConst16((uint16_t)(0x50C0 | code << 8 | parse_GetEAField(src)));
	return parse_OutputExtWords(src);
}

static bool_t parse_ST(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0x0, sz, src, dest);
}

static bool_t parse_SF(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0x1, sz, src, dest);
}

static bool_t parse_SHI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0x2, sz, src, dest);
}

static bool_t parse_SLS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0x3, sz, src, dest);
}

static bool_t parse_SCC(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0x4, sz, src, dest);
}

static bool_t parse_SCS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0x5, sz, src, dest);
}

static bool_t parse_SNE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0x6, sz, src, dest);
}

static bool_t parse_SEQ(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0x7, sz, src, dest);
}

static bool_t parse_SVC(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0x8, sz, src, dest);
}

static bool_t parse_SVS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0x9, sz, src, dest);
}

static bool_t parse_SPL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0xA, sz, src, dest);
}

static bool_t parse_SMI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0xB, sz, src, dest);
}

static bool_t parse_SGE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0xC, sz, src, dest);
}

static bool_t parse_SLT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0xD, sz, src, dest);
}

static bool_t parse_SGT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0xE, sz, src, dest);
}

static bool_t parse_SLE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Scc(0xF, sz, src, dest);
}

static bool_t parse_SWAP(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputConst16((uint16_t)(0x4840 | src->nDirectReg));
	return true;
}

static bool_t parse_TAS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputConst16((uint16_t)(0x4AC0 | parse_GetEAField(src)));
	return parse_OutputExtWords(src);
}

static bool_t parse_TRAP(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	SExpression* expr;

	expr = expr_CheckRange(src->pImmediate, 0, 15);
	if(expr == NULL)
	{
		prj_Error(ERROR_OPERAND_RANGE);
		return true;
	}
	expr = expr_Or(expr, expr_Const(0x4E40));
	sect_OutputExpr16(expr);
	return true;
}

static bool_t parse_TRAPcc(uint16_t code, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	uint16_t opmode;

	if(sz == SIZE_DEFAULT && src->eMode == AM_EMPTY)
		opmode = 0x4;
	else if(sz == SIZE_WORD && src->eMode == AM_IMM)
		opmode = 0x2;
	else if(sz == SIZE_LONG && src->eMode == AM_IMM)
		opmode = 0x3;
	else
	{
		prj_Error(ERROR_OPERAND);
		return true;
	}

	sect_OutputConst16(0x50F8 | opmode | code << 8);
	if(sz == SIZE_WORD)
		sect_OutputExpr16(src->pImmediate);
	else if(sz == SIZE_LONG)
		sect_OutputExprLong(src->pImmediate);

	return true;
}

static bool_t parse_TRAPT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0x0, sz, src, dest);
}

static bool_t parse_TRAPF(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0x1, sz, src, dest);
}

static bool_t parse_TRAPHI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0x2, sz, src, dest);
}

static bool_t parse_TRAPLS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0x3, sz, src, dest);
}

static bool_t parse_TRAPCC(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0x4, sz, src, dest);
}

static bool_t parse_TRAPCS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0x5, sz, src, dest);
}

static bool_t parse_TRAPNE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0x6, sz, src, dest);
}

static bool_t parse_TRAPEQ(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0x7, sz, src, dest);
}

static bool_t parse_TRAPVC(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0x8, sz, src, dest);
}

static bool_t parse_TRAPVS(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0x9, sz, src, dest);
}

static bool_t parse_TRAPPL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0xA, sz, src, dest);
}

static bool_t parse_TRAPMI(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0xB, sz, src, dest);
}

static bool_t parse_TRAPGE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0xC, sz, src, dest);
}

static bool_t parse_TRAPLT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0xD, sz, src, dest);
}

static bool_t parse_TRAPGT(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0xE, sz, src, dest);
}

static bool_t parse_TRAPLE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_TRAPcc(0xF, sz, src, dest);
}

static bool_t parse_TRAPV(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputConst16(0x4E76);
	return true;
}

static bool_t parse_UNLK(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	sect_OutputConst16((uint16_t)(0x4E58 | src->nDirectReg));
	return true;
}

static bool_t parse_RESET(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	prj_Warn(MERROR_INSTRUCTION_PRIV);
	sect_OutputConst16(0x4E70);
	return true;
}

static bool_t parse_RTE(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	prj_Warn(MERROR_INSTRUCTION_PRIV);
	sect_OutputConst16(0x4E73);
	return true;
}

static bool_t parse_STOP(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	prj_Warn(MERROR_INSTRUCTION_PRIV);
	sect_OutputConst16(0x4E72);
	sect_OutputExpr16(src->pImmediate);
	return true;
}

static bool_t parse_Cache040(uint16_t ins, uint16_t scope, ESize sz, SAddrMode* src, SAddrMode* dest)
{
	uint16_t cache = 0;
	uint16_t reg;
	
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
		return true;
	}

	if(scope == 3)
		reg = 0;
	else
		reg = (uint16_t)dest->Outer.nBaseReg;

	sect_OutputConst16(ins | scope << 3 | cache << 6 | reg);
	return true;
}

static bool_t parse_CINVA(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Cache040(0xF400, 0x3, sz, src, dest);
}

static bool_t parse_CINVL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Cache040(0xF400, 0x1, sz, src, dest);
}

static bool_t parse_CINVP(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Cache040(0xF400, 0x2, sz, src, dest);
}

static bool_t parse_CPUSHA(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Cache040(0xF420, 0x3, sz, src, dest);
}

static bool_t parse_CPUSHL(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Cache040(0xF420, 0x1, sz, src, dest);
}

static bool_t parse_CPUSHP(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	return parse_Cache040(0xF420, 0x2, sz, src, dest);
}

typedef struct
{
	uint16_t	nCpu;
	uint16_t	nValue;
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

static bool_t parse_MOVEC(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	uint16_t dr;
	int control;
	uint16_t reg;
	SControlRegister* pReg;

	prj_Warn(MERROR_INSTRUCTION_PRIV);

	if(src->eMode == AM_SYSREG && (dest->eMode == AM_DREG || dest->eMode == AM_AREG))
	{
		dr = 0;
		control = src->nDirectReg;
		if(dest->eMode == AM_DREG)
			reg = (uint16_t)dest->nDirectReg;
		else
			reg = (uint16_t)dest->nDirectReg + 8;
	}
	else if(dest->eMode == AM_SYSREG && (src->eMode == AM_DREG || src->eMode == AM_AREG))
	{
		dr = 1;
		control = dest->nDirectReg;
		if(src->eMode == AM_DREG)
			reg = (uint16_t)src->nDirectReg;
		else
			reg = (uint16_t)src->nDirectReg + 8;
	}
	else
	{
		prj_Error(ERROR_OPERAND);
		return true;
	}

	if(control < T_68K_REG_SFC || control > T_68K_REG_DACR1)
	{
		prj_Error(ERROR_OPERAND);
		return true;
	}

	pReg = &g_ControlRegister[control - T_68K_REG_SFC];
	if((pReg->nCpu & g_pOptions->pMachine->nCpu) == 0)
	{
		prj_Error(MERROR_INSTRUCTION_CPU);
		return true;
	}

	sect_OutputConst16(0x4E7A | dr);
	sect_OutputConst16(reg << 12 | pReg->nValue);
	return true;
}

static bool_t parse_MOVES(ESize sz, SAddrMode* src, SAddrMode* dest)
{
	int allow = AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_PCXDISP | AM_WORD | AM_LONG | AM_PCXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020;
	uint16_t dr;
	uint16_t reg;
	SAddrMode* ea;

	prj_Warn(MERROR_INSTRUCTION_PRIV);

	if((src->eMode == AM_DREG || src->eMode == AM_AREG)
	&& (dest->eMode & allow))
	{
		ea = dest;
		dr = 1;
		if(src->eMode == AM_DREG)
			reg = (uint16_t)src->nDirectReg;
		else
			reg = (uint16_t)src->nDirectReg + 8;
	}
	else if((dest->eMode == AM_DREG || dest->eMode == AM_AREG)
	&& (src->eMode & allow))
	{
		ea = src;
		dr = 0;
		if(dest->eMode == AM_DREG)
			reg = (uint16_t)src->nDirectReg;
		else
			reg = (uint16_t)src->nDirectReg + 8;
	}
	else
	{
		prj_Error(ERROR_OPERAND);
		return true;
	}

	sect_OutputConst16((uint16_t)(0x0E00 | parse_GetSizeField(sz) << 6 | parse_GetEAField(ea)));
	sect_OutputConst16(reg << 12 | dr << 11);
	return parse_OutputExtWords(ea);
}


typedef struct
{
	int	nCPU;
	int	nAllowSize;
	int	nDefaultSize;
	uint32_t nAllowSrc;
	uint32_t nAllowDest;
	uint32_t nAllowSrc020;
	uint32_t nAllowDest020;
	bool_t (*pHandler)(ESize eSize, SAddrMode* pSrc, SAddrMode* pDest);
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
		AM_IMM | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_DREG | (uint32_t)AM_EMPTY,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		parse_ASL
	},
	{	// ASR
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_IMM | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_DREG | (uint32_t)AM_EMPTY,
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
		AM_IMM | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_DREG | (uint32_t)AM_EMPTY,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		parse_LSL
	},
	{	// LSR
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_IMM | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_DREG | (uint32_t)AM_EMPTY,
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
		AM_IMM | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_DREG | (uint32_t)AM_EMPTY,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		parse_ROL
	},
	{	// ROR
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_IMM | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_DREG | (uint32_t)AM_EMPTY,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		parse_ROR
	},
	{	// ROXL
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_IMM | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_DREG | (uint32_t)AM_EMPTY,
		AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, 0,
		parse_ROXL
	},
	{	// ROXR
		CPUF_ALL,
		SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
		AM_IMM | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_DREG | (uint32_t)AM_EMPTY,
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
		(uint32_t)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPCC
	},
	{	// TRAPCS
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(uint32_t)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPCS
	},
	{	// TRAPEQ
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(uint32_t)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPEQ
	},
	{	// TRAPF
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(uint32_t)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPF
	},
	{	// TRAPGE
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(uint32_t)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPGE
	},
	{	// TRAPGT
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(uint32_t)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPGT
	},
	{	// TRAPHI
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(uint32_t)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPHI
	},
	{	// TRAPLE
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(uint32_t)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPLE
	},
	{	// TRAPLS
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(uint32_t)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPLS
	},
	{	// TRAPLT
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(uint32_t)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPLT
	},
	{	// TRAPMI
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(uint32_t)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPMI
	},
	{	// TRAPNE
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(uint32_t)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPNE
	},
	{	// TRAPPL
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(uint32_t)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPPL
	},
	{	// TRAPT
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(uint32_t)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPT
	},
	{	// TRAPVC
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(uint32_t)(AM_EMPTY | AM_IMM), 0,
		0, 0,
		parse_TRAPVC
	},
	{	// TRAPVS
		CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
		SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
		(uint32_t)(AM_EMPTY | AM_IMM), 0,
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

static bool_t parse_IntegerOp(int op, ESize inssz, SAddrMode* src, SAddrMode* dest)
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
		return true;
	}

	allowdest = pIns->nAllowDest;
	if(g_pOptions->pMachine->nCpu >= CPUF_68020)
		allowdest |= pIns->nAllowDest020;

	if((allowdest & dest->eMode) == 0
	&& !(allowdest == 0 && dest->eMode == AM_EMPTY))
	{
		prj_Error(ERROR_DEST_OPERAND);
		return true;
	}

	return pIns->pHandler(inssz, src, dest);
}

bool_t parse_GetBitfield(SAddrMode* pMode)
{
	if(parse_ExpectChar('{'))
	{
		pMode->bBitfield = true;

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
				return false;
			}
			pMode->nBFOffsetReg = -1;
		}

		if(!parse_ExpectChar(':'))
			return false;

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
				return true;
			}
			pMode->nBFWidthReg = -1;
		}

		return parse_ExpectChar('}');
	}

	return false;
}

bool_t parse_IntegerInstruction(void)
{
	int op;
	SInstruction* pIns;
	ESize inssz;
	SAddrMode src;
	SAddrMode dest;

	if(g_CurrentToken.ID.TargetToken < T_68K_INTEGER_FIRST
	|| g_CurrentToken.ID.TargetToken > T_68K_INTEGER_LAST)
	{
		return false;
	}

	op = g_CurrentToken.ID.TargetToken - T_68K_INTEGER_FIRST;
	parse_GetToken();

	pIns = &sIntegerInstructions[op];
	if((pIns->nCPU & g_pOptions->pMachine->nCpu) == 0)
	{
		prj_Error(MERROR_INSTRUCTION_CPU);
		return true;
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
					return false;
				}
			}

			if(src.eMode == AM_IMM)
				src.eImmSize = inssz;
		}
		else
		{
			if((pIns->nAllowSrc & AM_EMPTY) == 0)
				return true;
		}
	}

	if(pIns->nAllowDest != 0)
	{
		if(g_CurrentToken.ID.TargetToken == ',')
		{
			parse_GetToken();
			if(!parse_GetAddrMode(&dest))
				return false;

			if(pIns->nAllowDest & AM_BITFIELD)
			{
				if(!parse_GetBitfield(&dest))
				{
					prj_Error(MERROR_EXPECT_BITFIELD);
					return false;
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
