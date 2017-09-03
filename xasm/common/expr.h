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

#ifndef	XASM_COMMON_INCLUDE_EXPR_H_INCLUDED_
#define	XASM_COMMON_INCLUDE_EXPR_H_INCLUDED_

#include "asmotor.h"
#include "tokens.h"

struct Symbol;

typedef	enum
{
	EXPR_OPERATOR,
	EXPR_PCREL,
	EXPR_CONSTANT,
	EXPR_SYMBOL,
	EXPR_PARENS
} EExprType;

#define EXPRF_CONSTANT	0x01
#define EXPRF_RELOC		0x02

typedef struct Expression
{
	struct Expression* pLeft;
	struct Expression* pRight;
	EExprType	eType;
	uint32_t	nFlags;
	EToken		eOperator;
	union
	{
		int32_t	Value;
		struct Symbol* pSymbol;
	} Value;
} SExpression;


INLINE EExprType expr_GetType(SExpression* pExpr)
{
	return pExpr->eType;
}

INLINE bool_t expr_IsOperator(SExpression* pExpr, EToken eOperator)
{
	return pExpr != NULL && pExpr->eType == EXPR_OPERATOR && pExpr->eOperator == eOperator;
}

INLINE bool_t expr_IsConstant(SExpression* pExpr)
{
	return pExpr != NULL && pExpr->nFlags & EXPRF_CONSTANT;
}

INLINE bool_t expr_IsRelocatable(SExpression* pExpr)
{
	return pExpr != NULL && pExpr->nFlags & EXPRF_RELOC;
}

extern SExpression* expr_CheckRange(SExpression* pExpr, int32_t nLow, int32_t nHigh);

extern SExpression* expr_Equal(SExpression* pLeft, SExpression* pRight);
extern SExpression* expr_NotEqual(SExpression* pLeft, SExpression* pRight);
extern SExpression* expr_GreaterThan(SExpression* pLeft, SExpression* pRight);
extern SExpression* expr_LessThan(SExpression* pLeft, SExpression* pRight);
extern SExpression* expr_GreaterEqual(SExpression* pLeft, SExpression* pRight);
extern SExpression* expr_LessEqual(SExpression* pLeft, SExpression* pRight);

extern SExpression* expr_BooleanNot(SExpression* pExpr);
extern SExpression* expr_BooleanOr(SExpression* pLeft, SExpression* pRight);
extern SExpression* expr_BooleanAnd(SExpression* pLeft, SExpression* pRight);
extern SExpression* expr_BooleanXor(SExpression* pLeft, SExpression* pRight);

extern SExpression* expr_Abs(SExpression* pExpr);

extern SExpression* expr_Or(SExpression* pLeft, SExpression* pRight);
extern SExpression* expr_And(SExpression* pLeft, SExpression* pRight);
extern SExpression* expr_Xor(SExpression* pLeft, SExpression* pRight);

extern SExpression* expr_Add(SExpression* pLeft, SExpression* pRight);
extern SExpression* expr_Sub(SExpression* pLeft, SExpression* pRight);
extern SExpression* expr_Mul(SExpression* pLeft, SExpression* pRight);
extern SExpression* expr_Div(SExpression* pLeft, SExpression* pRight);
extern SExpression* expr_Mod(SExpression* pLeft, SExpression* pRight);

extern SExpression* expr_Bit(SExpression* pExpr);
extern SExpression* expr_Shl(SExpression* pLeft, SExpression* pRight);
extern SExpression* expr_Shr(SExpression* pLeft, SExpression* pRight);

extern SExpression* expr_Sin(SExpression* pExpr);
extern SExpression* expr_Cos(SExpression* pExpr);
extern SExpression* expr_Tan(SExpression* pExpr);
extern SExpression* expr_Asin(SExpression* pExpr);
extern SExpression* expr_Acos(SExpression* pExpr);
extern SExpression* expr_Atan(SExpression* pExpr);
extern SExpression* expr_Atan2(SExpression* pLeft, SExpression* pRight);

extern SExpression* expr_Fmul(SExpression* pLeft, SExpression* pRight);
extern SExpression* expr_Fdiv(SExpression* pLeft, SExpression* pRight);

extern SExpression* expr_Parens(SExpression* pExpr);

extern SExpression* expr_PcRelative(SExpression* pExpr, int nAdjust);

extern SExpression* expr_Pc();
extern SExpression* expr_Const(int32_t nValue);
extern void expr_SetConst(SExpression* pExpr, int32_t nValue);
extern SExpression* expr_Symbol(char* pszSymbol);
extern SExpression* expr_Bank(char* pszSymbol);

extern SExpression* expr_Clone(SExpression* pExpr);
extern void expr_Free(SExpression* pExpr);
extern void expr_Clear(SExpression* pExpr);


#endif	/*INCLUDE_EXPR_H*/
