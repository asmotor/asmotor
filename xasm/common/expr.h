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

#ifndef	INCLUDE_EXPR_H
#define	INCLUDE_EXPR_H

#include "asmotor.h"
#include "tokens.h"

struct Symbol;

typedef	enum
{
	EXPR_OPERATOR,
	EXPR_PCREL,
	EXPR_CONSTANT,
	EXPR_SYMBOL
} EExprType;

#define EXPRF_CONSTANT	0x01
#define EXPRF_RELOC		0x02

typedef struct Expression
{
	struct Expression*	pLeft;
	struct Expression*	pRight;
	EExprType	eType;
	uint32_t	nFlags;
	EToken		eOperator;
	union
	{
		int32_t	Value;
		struct Symbol*	pSymbol;
	} Value;
} SExpression;


INLINE EExprType expr_GetType(SExpression* pExpr)
{
	return pExpr->eType;
}

INLINE bool_t expr_IsOperator(SExpression* pExpr, EToken eOperator)
{
	return pExpr->eType == EXPR_OPERATOR && pExpr->eOperator == eOperator;
}

INLINE bool_t expr_IsConstant(SExpression* pExpr)
{
	return pExpr->nFlags & EXPRF_CONSTANT;
}

INLINE bool_t expr_IsRelocatable(SExpression* pExpr)
{
	return pExpr->nFlags & EXPRF_RELOC;
}

extern SExpression* expr_CheckRange(SExpression* expr, int32_t low, int32_t high);

extern SExpression* expr_Equal(SExpression* left, SExpression* right);
extern SExpression* expr_NotEqual(SExpression* left, SExpression* right);
extern SExpression* expr_GreaterThan(SExpression* left, SExpression* right);
extern SExpression* expr_LessThan(SExpression* left, SExpression* right);
extern SExpression* expr_GreaterEqual(SExpression* left, SExpression* right);
extern SExpression* expr_LessEqual(SExpression* left, SExpression* right);

extern SExpression* expr_BooleanNot(SExpression* expr);
extern SExpression* expr_BooleanOr(SExpression* left, SExpression* right);
extern SExpression* expr_BooleanAnd(SExpression* left, SExpression* right);
extern SExpression* expr_BooleanXor(SExpression* left, SExpression* right);

extern SExpression* expr_Abs(SExpression* expr);

extern SExpression* expr_Or(SExpression* left, SExpression* right);
extern SExpression* expr_And(SExpression* left, SExpression* right);
extern SExpression* expr_Xor(SExpression* left, SExpression* right);

extern SExpression* expr_Add(SExpression* left, SExpression* right);
extern SExpression* expr_Sub(SExpression* left, SExpression* right);
extern SExpression* expr_Mul(SExpression* left, SExpression* right);
extern SExpression* expr_Div(SExpression* left, SExpression* right);
extern SExpression* expr_Mod(SExpression* left, SExpression* right);

extern SExpression* expr_Bit(SExpression* right);
extern SExpression* expr_Shl(SExpression* left, SExpression* right);
extern SExpression* expr_Shr(SExpression* left, SExpression* right);

extern SExpression* expr_Sin(SExpression* right);
extern SExpression* expr_Cos(SExpression* right);
extern SExpression* expr_Tan(SExpression* right);
extern SExpression* expr_Asin(SExpression* right);
extern SExpression* expr_Acos(SExpression* right);
extern SExpression* expr_Atan(SExpression* right);
extern SExpression* expr_Atan2(SExpression* left, SExpression* right);

extern SExpression* expr_Fmul(SExpression* left, SExpression* right);
extern SExpression* expr_Fdiv(SExpression* left, SExpression* right);

extern SExpression* expr_PcRelative(SExpression* expr, int nAdjust);

extern SExpression* expr_Pc();
extern SExpression* expr_Const(int32_t value);
extern SExpression* expr_Symbol(char* s);
extern SExpression* expr_Bank(char* s);

extern SExpression* expr_Clone(SExpression* expr);
extern void expr_Free(SExpression* expr);
extern void expr_Clear(SExpression* expr);


#endif	/*INCLUDE_EXPR_H*/
