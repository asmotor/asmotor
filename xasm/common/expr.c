/*  Copyright 2008 Carsten S�rensen

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

#include <stdio.h>

#include "asmotor.h"
#include "xasm.h"
#include "expr.h"
#include "symbol.h"
#include "section.h"
#include "tokens.h"
#include "project.h"

#define T_OP_Sub T_OP_SUB
#define T_OP_Add T_OP_ADD
#define T_OP_Xor T_OP_XOR
#define T_OP_Or T_OP_OR
#define T_OP_And T_OP_AND
#define T_OP_Shl T_OP_SHL
#define T_OP_Shr T_OP_SHR
#define T_OP_Mul T_OP_MUL
#define T_OP_Div T_OP_DIV
#define T_OP_Mod T_OP_MOD
#define T_OP_BooleanOr T_OP_LOGICOR
#define T_OP_BooleanAnd T_OP_LOGICAND
#define T_OP_GreaterEqual T_OP_LOGICGE
#define T_OP_GreaterThan T_OP_LOGICGT
#define T_OP_LessEqual T_OP_LOGICLE
#define T_OP_LessThan T_OP_LOGICLT
#define T_OP_Equal T_OP_LOGICEQU
#define T_OP_NotEqual T_OP_LOGICNE

#define T_FUNC_Sin T_FUNC_SIN
#define T_FUNC_Cos T_FUNC_COS
#define T_FUNC_Tan T_FUNC_TAN
#define T_FUNC_ASin T_FUNC_ASIN
#define T_FUNC_ACos T_FUNC_ACOS
#define T_FUNC_ATan T_FUNC_ATAN
#define T_FUNC_ATan2 T_FUNC_ATAN2

#define T_FUNC_LowLimit T_FUNC_LOWLIMIT
#define T_FUNC_HighLimit T_FUNC_HIGHLIMIT

static void parse_VerifyPointers(SExpression* left, SExpression* right)
{
	if(left != NULL && right != NULL )
		return;

	prj_Fail(ERROR_INVALID_EXPRESSION);
}

SExpression* expr_CreateAbsExpr(SExpression* right)
{
	SExpression* pSign = expr_CreateShrExpr(expr_DuplicateExpr(right), expr_CreateConstExpr(31));
	return expr_CreateSubExpr(expr_CreateXorExpr(right, expr_DuplicateExpr(pSign)), pSign);
}


SExpression* expr_CreateBitExpr(SExpression* right)
{
	SExpression* expr;

	parse_VerifyPointers(right, right);

	if((expr = (SExpression*)malloc(sizeof(SExpression))) != NULL)
	{
		int b = 0;
		uint32_t v = right->Value.Value;

		if((right->Flags & EXPRF_isCONSTANT) != 0
		&& ((v & -(int32_t)v) != v))
		{
			prj_Error(ERROR_EXPR_TWO_POWER);
			return NULL;
		}

		if(v != 0)
		{
			while(v != 1)
			{
				v >>= 1;
				++b;
			}
		}

		expr->pRight = right;
		expr->pLeft = NULL;
		expr->Value.Value = b;
		expr->Flags = right->Flags;
		expr->Type = EXPR_OPERATOR;
		expr->Operator = T_OP_BIT;
		return expr;
	}
	else
	{
		internalerror("Out of memory!");
		return NULL;
	}
}

SExpression* expr_CreatePcRelativeExpr(SExpression* in, int nAdjust)
{
	SExpression* expr = (SExpression*)malloc(sizeof(SExpression));

	if(expr == NULL)
		internalerror("Out of memory!");

	expr->Value.Value = 0;
	expr->Type = EXPR_PCREL;
	expr->Flags = EXPRF_isRELOC;
	expr->pLeft = expr_CreateConstExpr(nAdjust);
	expr->pRight = in;
	return expr;
}


SExpression* expr_CreatePcExpr()
{
	SExpression* expr;

	if((expr = (SExpression*)malloc(sizeof(SExpression))) != NULL)
	{
		char sym[MAXSYMNAMELENGTH + 20];
		SSymbol* pSym;

		sprintf(sym, "$%s%lu", pCurrentSection->Name, pCurrentSection->PC);
		pSym = sym_AddLabel(sym);

		if(pSym->Flags & SYMF_CONSTANT)
		{
			expr->Value.Value = pSym->Value.Value;
			expr->Type = EXPR_CONSTANT;
			expr->Flags = EXPRF_isCONSTANT | EXPRF_isRELOC;
			expr->pLeft = NULL;
			expr->pRight = NULL;
		}
		else
		{
			expr->pRight = NULL;
			expr->pLeft = NULL;
			expr->Value.pSymbol = pSym;
			expr->Flags = EXPRF_isRELOC;
			expr->Type = EXPR_SYMBOL;
		}

		return expr;
	}

	internalerror("Out of memory!");
	return NULL;
}

SExpression* expr_CreateConstExpr(int32_t value)
{
	SExpression* expr;

	if((expr = (SExpression*)malloc(sizeof(SExpression))) != NULL)
	{
		expr->Value.Value = value;
		expr->Type = EXPR_CONSTANT;
		expr->Flags = EXPRF_isCONSTANT | EXPRF_isRELOC;
		expr->pLeft = NULL;
		expr->pRight = NULL;
		return expr;
	}
	else
	{
		internalerror("Out of memory!");
	}
	return NULL;
}

static SExpression* parse_MergeExpressions(SExpression* left, SExpression* right)
{
	SExpression* expr;

	parse_VerifyPointers(left, right);

	if((expr=(SExpression*)malloc(sizeof(SExpression)))!=NULL)
	{
		expr->Value.Value=0;
		expr->Type=0;
		expr->Flags=(left->Flags)&(right->Flags);
		expr->pLeft=left;
		expr->pRight=right;
		return expr;
	}
	else
	{
		internalerror("Out of memory!");
		return NULL;
	}
}

#define CREATEEXPRDIV(NAME,OP)															\
SExpression* expr_Create ## NAME ## Expr(SExpression* left, SExpression* right)	\
{																						\
	int32_t val;																			\
	parse_VerifyPointers(left, right);													\
	if(right->Value.Value != 0)															\
	{																					\
		val = left->Value.Value OP right->Value.Value;									\
		left = parse_MergeExpressions(left, right);										\
		left->Type = EXPR_OPERATOR;														\
		left->Operator = T_OP_ ## NAME;													\
		left->Value.Value = val;														\
		return left;																	\
	}																					\
	else																				\
	{																					\
		prj_Fail(ERROR_ZERO_DIVIDE);													\
		return NULL;																	\
	}																					\
}

CREATEEXPRDIV(Div, /)
CREATEEXPRDIV(Mod, %)

SExpression* expr_CreateBooleanNotExpr(SExpression* right)
{
	SExpression* expr;

	parse_VerifyPointers(right, right);

	if((expr = (SExpression*)malloc(sizeof(SExpression))) != NULL)
	{
		expr->pRight = right;
		expr->pLeft = NULL;
		expr->Value.Value = !right->Value.Value;
		expr->Flags = right->Flags;
		expr->Type = EXPR_OPERATOR;
		expr->Operator = T_OP_LOGICNOT;
		return expr;
	}
	else
	{
		internalerror("Out of memory!");
		return NULL;
	}
}

#define CREATEEXPR(NAME,OP) \
SExpression* expr_Create ## NAME ## Expr(SExpression* left, SExpression* right)	\
{																					\
	int32_t val;																		\
	parse_VerifyPointers(left, right);												\
	val = left->Value.Value OP right->Value.Value;									\
	left = parse_MergeExpressions(left, right);										\
	left->Type = EXPR_OPERATOR;														\
	left->Operator = T_OP_ ## NAME;													\
	left->Value.Value = val;														\
	return left;																	\
}

CREATEEXPR(Sub, -)
CREATEEXPR(Add, +)
CREATEEXPR(Xor, ^)
CREATEEXPR(Or,  |)
CREATEEXPR(And, &)
CREATEEXPR(Shl, <<)
CREATEEXPR(Shr, >>)
CREATEEXPR(Mul, *)
CREATEEXPR(BooleanOr, ||)
CREATEEXPR(BooleanAnd, &&)
CREATEEXPR(GreaterEqual, >=)
CREATEEXPR(GreaterThan, >)
CREATEEXPR(LessEqual, <=)
CREATEEXPR(LessThan, <)
CREATEEXPR(Equal, ==)
CREATEEXPR(NotEqual, !=)

#define CREATELIMIT(NAME,OP)												\
SExpression* expr_Create ## NAME ## Expr(SExpression* expr, SExpression* bound)	\
{																			\
	int32_t val;															\
	parse_VerifyPointers(expr, bound);										\
	val = expr->Value.Value;													\
	if((expr->Flags & EXPRF_isCONSTANT) && (bound->Flags & EXPRF_isCONSTANT))	\
	{																		\
		if(expr->Value.Value OP bound->Value.Value)							\
		{																	\
			expr_FreeExpression(expr);										\
			expr_FreeExpression(bound);									\
			return NULL;													\
		}																	\
	}																		\
	expr = parse_MergeExpressions(expr, bound);								\
	expr->Type = EXPR_OPERATOR;												\
	expr->Operator = T_FUNC_ ## NAME;										\
	expr->Value.Value = val;												\
	return expr;															\
}

CREATELIMIT(LowLimit,<)
CREATELIMIT(HighLimit,>)

SExpression* expr_CheckRange(SExpression* expr, int32_t low, int32_t high)
{
	SExpression* low_expr;
	SExpression* high_expr;

	low_expr = expr_CreateConstExpr(low);
	high_expr = expr_CreateConstExpr(high);

	expr = expr_CreateLowLimitExpr(expr, low_expr);
	if(expr != NULL)
		return expr_CreateHighLimitExpr(expr, high_expr);

	expr_FreeExpression(high_expr);
	return NULL;
}

SExpression* expr_CreateFDivExpr(SExpression* left, SExpression* right)
{
	int32_t val;

	parse_VerifyPointers(left, right);

	if(right->Value.Value != 0)
	{
		val = fdiv(left->Value.Value, right->Value.Value);

		left = parse_MergeExpressions(left, right);
		left->Type = EXPR_OPERATOR;
		left->Operator = T_FUNC_FDIV;
		left->Value.Value = val;
		left->Flags &= ~EXPRF_isRELOC;
		return left;
	}
	else
	{
		prj_Fail(ERROR_ZERO_DIVIDE);
		return NULL;
	}
}

SExpression* expr_CreateFMulExpr(SExpression* left, SExpression* right)
{
	int32_t val;

	parse_VerifyPointers(left, right);

	val = fmul(left->Value.Value, right->Value.Value);

	left = parse_MergeExpressions(left, right);
	left->Type = EXPR_OPERATOR;
	left->Operator = T_FUNC_FMUL;
	left->Value.Value = val;
	left->Flags &= ~EXPRF_isRELOC;
	return left;
}

SExpression* expr_CreateATan2Expr(SExpression* left, SExpression* right)
{
	int32_t val;

	parse_VerifyPointers(left, right);

	val = fatan2(left->Value.Value, right->Value.Value);

	left = parse_MergeExpressions(left, right);
	left->Type = EXPR_OPERATOR;
	left->Operator = T_FUNC_ATAN2;
	left->Value.Value = val;
	left->Flags &= ~EXPRF_isRELOC;
	return left;
}

#define CREATETRANSEXPR(NAME,FUNC)										\
SExpression* expr_Create ## NAME ## Expr(SExpression* right)	\
{																		\
	SExpression* expr;													\
	parse_VerifyPointers(right, right);									\
	if((expr = (SExpression*)malloc(sizeof(SExpression))) != NULL)		\
	{																	\
		expr->pRight = right;											\
		expr->pLeft = NULL;												\
		expr->Value.Value = FUNC(right->Value.Value);					\
		expr->Flags = right->Flags & ~EXPRF_isRELOC;					\
		expr->Type = EXPR_OPERATOR;										\
		expr->Operator = T_FUNC_ ## NAME;								\
		return expr;													\
	}																	\
	else																\
	{																	\
		internalerror("Out of memory!");								\
		return NULL;													\
	}																	\
}

CREATETRANSEXPR(Sin,fsin)
CREATETRANSEXPR(Cos,fcos)
CREATETRANSEXPR(Tan,ftan)
CREATETRANSEXPR(ASin,fasin)
CREATETRANSEXPR(ACos,facos)
CREATETRANSEXPR(ATan,fatan)

SExpression* expr_CreateBankExpr(char* s)
{
	SExpression* expr;

	if(!g_pConfiguration->bSupportBanks)
		internalerror("Banks not supported");

	if((expr = (SExpression*)malloc(sizeof(SExpression))) != NULL)
	{
		expr->pRight = NULL;
		expr->pLeft = NULL;
		expr->Value.pSymbol = sym_FindSymbol(s);
		expr->Value.pSymbol->Flags |= SYMF_REFERENCED;
		expr->Flags = EXPRF_isRELOC;
		expr->Type = EXPR_OPERATOR;
		expr->Operator = T_FUNC_BANK;
		return expr;
	}
	else
	{
		internalerror("Out of memory!");
		return NULL;
	}
}

SExpression* expr_CreateSymbolExpr(char* s)
{
	SSymbol* sym;
	SExpression* expr;

	sym = sym_FindSymbol(s);

	if(sym->Flags & SYMF_EXPR)
	{
		sym->Flags |= SYMF_REFERENCED;
		if(sym->Flags & SYMF_CONSTANT)
		{
			return expr_CreateConstExpr(sym_GetConstant(s));
		}
		else if((expr = (SExpression*)malloc(sizeof(SExpression))) != NULL)
		{
			expr->pRight = NULL;
			expr->pLeft = NULL;
			expr->Value.pSymbol = sym;
			expr->Flags = EXPRF_isRELOC;
			expr->Type = EXPR_SYMBOL;
			return expr;
		}
		else
		{
			internalerror("Out of memory!");
			return NULL;
		}
	}
	else
	{
		prj_Fail(ERROR_SYMBOL_IN_EXPR);
		return NULL;
	}
}
