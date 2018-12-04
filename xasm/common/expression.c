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

#include <stdio.h>

// From util
#include "asmotor.h"
#include "fmath.h"
#include "mem.h"

// From xasm
#include "xasm.h"
#include "expression.h"
#include "symbol.h"
#include "section.h"
#include "tokens.h"
#include "project.h"

#define COMBINE(NAME, OP, TOKEN) \
SExpression* expr_ ## NAME(SExpression* left, SExpression* right) { \
    if (!expr_VerifyPointers(left, right)) \
        return NULL;                       \
    int32_t val = OP;                      \
    left = mergeExpressions(left, right);  \
    left->type = EXPR_OPERATION;           \
    left->operation = TOKEN;               \
    left->Value.Value = val;               \
    return left;                           \
}

#define COMBINE_BITWISE(NAME, OP, TOKEN) \
	COMBINE(NAME, (uint32_t) left->Value.Value OP (uint32_t) right->Value.Value, TOKEN)

#define COMBINE_ARITHMETIC(NAME, OP, TOKEN) \
	COMBINE(NAME, left->Value.Value OP right->Value.Value, TOKEN)

#define CREATE_BINARY_FUNC(NAME, OP, TOKEN) \
	COMBINE(NAME, OP(left->Value.Value, right->Value.Value), TOKEN)

#define CREATE_LIMIT(NAME, OP, TOKEN) \
SExpression* expr_ ## NAME(SExpression* expr, SExpression* bound) { \
    if (!expr_VerifyPointers(expr, bound))                \
        return NULL;                                      \
    int32_t val = expr->Value.Value;                      \
    if(expr_IsConstant(expr) && expr_IsConstant(bound)) { \
        if(expr->Value.Value OP bound->Value.Value) {     \
            expr_Free(expr);                              \
            expr_Free(bound);                             \
            return NULL;                                  \
        }                                                 \
    }                                                     \
    expr = mergeExpressions(expr, bound);                 \
    expr->type = EXPR_OPERATION;                          \
    expr->operation = TOKEN;                              \
    expr->Value.Value = val;                              \
    return expr;                                          \
}

#define CREATE_UNARY_FUNC(NAME, FUNC, TOKEN)      \
SExpression* expr_ ## NAME(SExpression* right) {  \
    if (!expr_VerifyPointer(right))               \
        return NULL;                              \
    SExpression* expr = (SExpression*) mem_Alloc(sizeof(SExpression)); \
    expr->right = right;                          \
    expr->left = NULL;                            \
    expr->Value.Value = FUNC(right->Value.Value); \
    expr->flags = right->flags & ~EXPRF_RELOC;    \
    expr->type = EXPR_OPERATION;                  \
    expr->operation = TOKEN;                      \
    return expr;                                  \
}

#define CREATE_EXPR_DIV(NAME, OP, TOKEN) \
SExpression* expr_ ## NAME(SExpression* left, SExpression* right) \
{                                                          \
    if (!expr_VerifyPointers(left, right))                 \
        return NULL;                                       \
    if (right->Value.Value == 0) {                         \
        prj_Fail(ERROR_ZERO_DIVIDE);                       \
        return NULL;                                       \
    }                                                      \
    int32_t val = left->Value.Value OP right->Value.Value; \
    left = mergeExpressions(left, right);                  \
    left->type = EXPR_OPERATION;                           \
    left->operation = TOKEN;                               \
    left->Value.Value = val;                               \
    return left;                                           \
}


static bool expr_VerifyPointers(SExpression* left, SExpression* right) {
	if (left != NULL && right != NULL)
		return true;

	prj_Fail(ERROR_INVALID_EXPRESSION);
	expr_Free(left);
	expr_Free(right);
	return false;
}

static bool expr_VerifyPointer(SExpression* expression) {
	if (expression != NULL)
		return true;

	prj_Fail(ERROR_INVALID_EXPRESSION);
	return false;
}

static SExpression* mergeExpressions(SExpression* left, SExpression* right) {
	SExpression* expr;

	if (!expr_VerifyPointers(left, right))
		return NULL;

	expr = (SExpression*) mem_Alloc(sizeof(SExpression));

	expr->flags = left->flags & right->flags;
	expr->left = left;
	expr->right = right;
	return expr;
}

CREATE_EXPR_DIV(Div, /, T_OP_DIV)

CREATE_EXPR_DIV(Mod, %, T_OP_MOD)

CREATE_UNARY_FUNC(BooleanNot, !, T_OP_LOGICNOT)

COMBINE_BITWISE(Xor, ^, T_OP_XOR)

COMBINE_BITWISE(Or, |, T_OP_OR)

COMBINE_BITWISE(And, &, T_OP_AND)

COMBINE_BITWISE(Asl, <<, T_OP_SHL)

COMBINE_ARITHMETIC(Add, +, T_OP_ADD)

COMBINE_ARITHMETIC(Mul, *, T_OP_ADD)

COMBINE_ARITHMETIC(BooleanOr, ||, T_OP_LOGICOR)

COMBINE_ARITHMETIC(BooleanAnd, &&, T_OP_LOGICAND)

COMBINE_ARITHMETIC(GreaterEqual, >=, T_OP_LOGICGE)

COMBINE_ARITHMETIC(GreaterThan, >, T_OP_LOGICGT)

COMBINE_ARITHMETIC(LessEqual, <=, T_OP_LOGICLE)

COMBINE_ARITHMETIC(LessThan, <, T_OP_LOGICLT)

COMBINE_ARITHMETIC(Equal, ==, T_OP_LOGICEQU)

COMBINE_ARITHMETIC(NotEqual, !=, T_OP_LOGICNE)

CREATE_LIMIT(LowLimit, <, T_FUNC_LOWLIMIT)

CREATE_LIMIT(HighLimit, >, T_FUNC_HIGHLIMIT)

CREATE_BINARY_FUNC(FixedMultiplication, fmul, T_FUNC_FMUL)

CREATE_BINARY_FUNC(Atan2, fatan2, T_FUNC_ATAN2)

CREATE_BINARY_FUNC(Asr, asr, T_OP_SHR)

CREATE_UNARY_FUNC(Sin, fsin, T_FUNC_SIN)

CREATE_UNARY_FUNC(Cos, fcos, T_FUNC_COS)

CREATE_UNARY_FUNC(Tan, ftan, T_FUNC_TAN)

CREATE_UNARY_FUNC(Asin, fasin, T_FUNC_ASIN)

CREATE_UNARY_FUNC(Acos, facos, T_FUNC_ACOS)

CREATE_UNARY_FUNC(Atan, fatan, T_FUNC_ATAN)

void expr_Clear(SExpression* pExpr) {
	if (pExpr == NULL)
		return;

	expr_Free(pExpr->left);
	pExpr->left = NULL;

	expr_Free(pExpr->right);
	pExpr->right = NULL;

	pExpr->flags = 0;
}

void expr_Free(SExpression* pExpr) {
	if (pExpr == NULL)
		return;

	expr_Free(pExpr->left);
	expr_Free(pExpr->right);
	mem_Free(pExpr);
}

SExpression* expr_Clone(SExpression* expr) {
	SExpression* r = (SExpression*) mem_Alloc(sizeof(SExpression));

	*r = *expr;
	if (r->left != NULL)
		r->left = expr_Clone(r->left);
	if (r->right != NULL)
		r->right = expr_Clone(r->right);

	return r;
}

SExpression* expr_Parens(SExpression* expression) {
	if (!expr_VerifyPointer(expression))
		return NULL;

	SExpression* r = (SExpression*) mem_Alloc(sizeof(SExpression));
	r->right = expression;
	r->left = NULL;
	r->Value.Value = expression->Value.Value;
	r->flags = expression->flags;
	r->type = EXPR_PARENS;
	r->operation = T_NONE;
	return r;
}

SExpression* expr_Abs(SExpression* right) {
	SExpression* sign = expr_Asr(expr_Clone(right), expr_Const(31));
	return expr_Sub(expr_Xor(right, expr_Clone(sign)), sign);
}

SExpression* expr_Bit(SExpression* right) {
	SExpression* expr;
	int b = 0;
	uint32_t v;

	if (!expr_VerifyPointer(right))
		return NULL;

	v = (uint32_t) right->Value.Value;

	if (expr_IsConstant(right) && (v & -v) != v) {
		prj_Error(ERROR_EXPR_TWO_POWER);
		return NULL;
	}

	b = 0;
	if (v != 0) {
		while (v != 1) {
			v >>= 1;
			++b;
		}
	}

	expr = (SExpression*) mem_Alloc(sizeof(SExpression));
	expr->right = right;
	expr->left = NULL;
	expr->Value.Value = b;
	expr->flags = right->flags;
	expr->type = EXPR_OPERATION;
	expr->operation = T_OP_BIT;
	return expr;
}

SExpression* expr_PcRelative(SExpression* pExpr, int nAdjust) {
	if (!expr_VerifyPointer(pExpr))
		return NULL;

	if ((pExpr->flags & EXPRF_CONSTANT) && (g_pCurrentSection->Flags & (SECTF_LOADFIXED | SECTF_ORGFIXED))) {
		pExpr->Value.Value -=
				(g_pCurrentSection->PC + g_pCurrentSection->BasePC + g_pCurrentSection->OrgOffset - nAdjust);
		return pExpr;
	} else if (g_pCurrentSection->Flags & (SECTF_LOADFIXED | SECTF_ORGFIXED)) {
		return expr_Add(pExpr, expr_Const(
				nAdjust - (g_pCurrentSection->PC + g_pCurrentSection->BasePC + g_pCurrentSection->OrgOffset)));
	} else {
		SExpression* expr = (SExpression*) mem_Alloc(sizeof(SExpression));

		expr->Value.Value = 0;
		expr->type = EXPR_PC_RELATIVE;
		expr->flags = EXPRF_RELOC;
		expr->left = expr_Const(nAdjust);
		expr->right = pExpr;
		return expr;
	}
}

SExpression* expr_Pc() {
	string* pName;
	SExpression* expr = (SExpression*) mem_Alloc(sizeof(SExpression));

	char sym[MAXSYMNAMELENGTH + 20];
	SSymbol* pSym;

	sprintf(sym, "$%s%u", str_String(g_pCurrentSection->Name), g_pCurrentSection->PC);
	pName = str_Create(sym);
	pSym = sym_CreateLabel(pName);
	str_Free(pName);

	if (pSym->nFlags & SYMF_CONSTANT) {
		expr->Value.Value = pSym->Value.Value;
		expr->type = EXPR_CONSTANT;
		expr->flags = EXPRF_CONSTANT | EXPRF_RELOC;
		expr->left = NULL;
		expr->right = NULL;
	} else {
		expr->right = NULL;
		expr->left = NULL;
		expr->Value.pSymbol = pSym;
		expr->flags = EXPRF_RELOC;
		expr->type = EXPR_SYMBOL;
	}

	return expr;
}

void expr_SetConst(SExpression* pExpr, int32_t nValue) {
	pExpr->left = NULL;
	pExpr->right = NULL;
	pExpr->type = EXPR_CONSTANT;
	pExpr->flags = EXPRF_CONSTANT | EXPRF_RELOC;
	pExpr->Value.Value = nValue;
}

SExpression* expr_Const(int32_t nValue) {
	SExpression* pExpr = (SExpression*) mem_Alloc(sizeof(SExpression));

	expr_SetConst(pExpr, nValue);
	return pExpr;
}

SExpression* expr_Sub(SExpression* left, SExpression* right) {
	if (!expr_VerifyPointers(left, right))
		return NULL;

	int32_t val = left->Value.Value - right->Value.Value;

	left = mergeExpressions(left, right);
	left->type = EXPR_OPERATION;
	left->operation = T_OP_SUB;
	left->Value.Value = val;

	if ((left->flags & EXPRF_CONSTANT) == 0 && left->left != NULL && left->right != NULL) {
		if (left->left->type == EXPR_SYMBOL && left->right->type == EXPR_SYMBOL) {
			if (left->left->Value.pSymbol->pSection == left->right->Value.pSymbol->pSection) {
				left->flags = EXPRF_CONSTANT;
				left->Value.Value = left->left->Value.pSymbol->Value.Value - left->right->Value.pSymbol->Value.Value;
			}
		}
	}
	return left;
}

SExpression* expr_FixedDivision(SExpression* left, SExpression* right) {
	if (!expr_VerifyPointers(left, right))
		return NULL;

	if (right->Value.Value != 0) {
		int32_t val = fdiv(left->Value.Value, right->Value.Value);

		left = mergeExpressions(left, right);
		left->type = EXPR_OPERATION;
		left->operation = T_FUNC_FDIV;
		left->Value.Value = val;
		left->flags &= ~EXPRF_RELOC;
		return left;
	}

	prj_Fail(ERROR_ZERO_DIVIDE);
	return NULL;
}

SExpression* expr_CheckRange(SExpression* pExpr, int32_t nLow, int32_t nHigh) {
	SExpression* pLowExpr = expr_Const(nLow);
	SExpression* pHighExpr = expr_Const(nHigh);

	pExpr = expr_LowLimit(pExpr, pLowExpr);
	if (pExpr != NULL)
		return expr_HighLimit(pExpr, pHighExpr);

	expr_Free(pHighExpr);
	return NULL;
}

SExpression* expr_Bank(const char* s) {
	SExpression* expr;
	string* pName;

	if (!g_pConfiguration->bSupportBanks)
		internalerror("Banks not supported");

	pName = str_Create(s);

	expr = (SExpression*) mem_Alloc(sizeof(SExpression));

	expr->right = NULL;
	expr->left = NULL;
	expr->Value.pSymbol = sym_FindSymbol(pName);
	expr->Value.pSymbol->nFlags |= SYMF_REFERENCED;
	expr->flags = EXPRF_RELOC;
	expr->type = EXPR_OPERATION;
	expr->operation = T_FUNC_BANK;

	str_Free(pName);

	return expr;
}

SExpression* expr_Symbol(const char* s) {
	string* pName = str_Create(s);
	SSymbol* pSym = sym_FindSymbol(pName);
	str_Free(pName);

	if (pSym->nFlags & SYMF_EXPR) {
		pSym->nFlags |= SYMF_REFERENCED;

		if (pSym->nFlags & SYMF_CONSTANT) {
			return expr_Const(sym_GetValue(pSym));
		} else {
			SExpression* pExpr = (SExpression*) mem_Alloc(sizeof(SExpression));

			pExpr->right = NULL;
			pExpr->left = NULL;
			pExpr->Value.pSymbol = pSym;
			pExpr->flags = EXPRF_RELOC;
			pExpr->type = EXPR_SYMBOL;

			return pExpr;
		}
	}

	prj_Fail(ERROR_SYMBOL_IN_EXPR);
	return NULL;
}

