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

struct Symbol;

typedef	enum
{
	EXPR_OPERATOR,
	EXPR_PCREL,
	EXPR_CONSTANT,
	EXPR_SYMBOL
} EExprType;

#define EXPRF_isCONSTANT	0x01
#define EXPRF_isRELOC		0x02

typedef struct Expression
{
	struct Expression*	pLeft;
	struct Expression*	pRight;
	EExprType	eType;
	uint32_t	Flags;
	uint32_t	Operator;
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


extern SExpression* expr_CheckRange(SExpression* expr, int32_t low, int32_t high);

extern SExpression* expr_CreateEqualExpr(SExpression* left, SExpression* right);
extern SExpression* expr_CreateNotEqualExpr(SExpression* left, SExpression* right);
extern SExpression* expr_CreateGreaterThanExpr(SExpression* left, SExpression* right);
extern SExpression* expr_CreateLessThanExpr(SExpression* left, SExpression* right);
extern SExpression* expr_CreateGreaterEqualExpr(SExpression* left, SExpression* right);
extern SExpression* expr_CreateLessEqualExpr(SExpression* left, SExpression* right);

extern SExpression* expr_CreateBooleanNotExpr(SExpression* expr);
extern SExpression* expr_CreateBooleanOrExpr(SExpression* left, SExpression* right);
extern SExpression* expr_CreateBooleanAndExpr(SExpression* left, SExpression* right);
extern SExpression* expr_CreateBooleanXorExpr(SExpression* left, SExpression* right);

extern SExpression* expr_CreateAbsExpr(SExpression* expr);

extern SExpression* expr_CreateOrExpr(SExpression* left, SExpression* right);
extern SExpression* expr_CreateAndExpr(SExpression* left, SExpression* right);
extern SExpression* expr_CreateXorExpr(SExpression* left, SExpression* right);

extern SExpression* expr_CreateAddExpr(SExpression* left, SExpression* right);
extern SExpression* expr_CreateSubExpr(SExpression* left, SExpression* right);
extern SExpression* expr_CreateMulExpr(SExpression* left, SExpression* right);
extern SExpression* expr_CreateDivExpr(SExpression* left, SExpression* right);
extern SExpression* expr_CreateModExpr(SExpression* left, SExpression* right);

extern SExpression* expr_CreatePcRelativeExpr(SExpression* expr, int nAdjust);

extern SExpression* expr_CreateBitExpr(SExpression* right);
extern SExpression* expr_CreateShlExpr(SExpression* left, SExpression* right);
extern SExpression* expr_CreateShrExpr(SExpression* left, SExpression* right);

extern SExpression* expr_CreateSinExpr(SExpression* right);
extern SExpression* expr_CreateCosExpr(SExpression* right);
extern SExpression* expr_CreateTanExpr(SExpression* right);
extern SExpression* expr_CreateASinExpr(SExpression* right);
extern SExpression* expr_CreateACosExpr(SExpression* right);
extern SExpression* expr_CreateATanExpr(SExpression* right);
extern SExpression* expr_CreateATan2Expr(SExpression* left, SExpression* right);

extern SExpression* expr_CreateFMulExpr(SExpression* left, SExpression* right);
extern SExpression* expr_CreateFDivExpr(SExpression* left, SExpression* right);

extern SExpression* expr_CreatePcRelativeExpr(SExpression* in, int nAdjust);

extern SExpression* expr_CreatePcExpr();
extern SExpression* expr_CreateConstExpr(int32_t value);
extern SExpression* expr_CreateSymbolExpr(char* s);
extern SExpression* expr_CreateBankExpr(char* s);

extern SExpression* expr_DuplicateExpr(SExpression* expr);
extern void expr_Free(SExpression* expr);


#endif	/*INCLUDE_EXPR_H*/
