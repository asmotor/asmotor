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
SExpression* expr_ ## NAME(SExpression* expr) {   \
    if (!expr_VerifyPointer(expr))                \
        return NULL;                              \
    SExpression* r = (SExpression*) mem_Alloc(sizeof(SExpression)); \
    r->right = expr;                              \
    r->left = NULL;                               \
    r->Value.Value = FUNC(expr->Value.Value);     \
    r->isConstant = expr->isConstant;             \
    r->type = EXPR_OPERATION;                     \
    r->operation = TOKEN;                         \
    return r;                                     \
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
    SExpression* r = mergeExpressions(left, right);        \
    r->type = EXPR_OPERATION;                              \
    r->operation = TOKEN;                                  \
    r->Value.Value = val;                                  \
    return r;                                              \
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

	expr->isConstant = left->isConstant && right->isConstant;
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

void expr_Clear(SExpression* expression) {
	if (expression != NULL) {
		expr_Free(expression->left);
		expression->left = NULL;

		expr_Free(expression->right);
		expression->right = NULL;

		expression->isConstant = true;
	}
}

void expr_Free(SExpression* expression) {
	if (expression != NULL) {
		expr_Free(expression->left);
		expr_Free(expression->right);
		mem_Free(expression);
	}
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
	r->isConstant = expression->isConstant;
	r->type = EXPR_PARENS;
	r->operation = T_NONE;
	return r;
}

SExpression* expr_Bit(SExpression* right) {
	if (!expr_VerifyPointer(right))
		return NULL;

	uint32_t v = (uint32_t) right->Value.Value;

	if (expr_IsConstant(right) && !exactlyOneBitSet(v)) {
		prj_Error(ERROR_EXPR_TWO_POWER);
		return NULL;
	}

	SExpression* r = (SExpression*) mem_Alloc(sizeof(SExpression));
	r->right = right;
	r->left = NULL;
	r->Value.Value = log2n(v);
	r->isConstant = right->isConstant;
	r->type = EXPR_OPERATION;
	r->operation = T_OP_BIT;
	return r;
}

SExpression* expr_PcRelative(SExpression* pExpr, int nAdjust) {
	if (!expr_VerifyPointer(pExpr))
		return NULL;

	if (expr_IsConstant(pExpr) && (g_pCurrentSection->Flags & (SECTF_LOADFIXED | SECTF_ORGFIXED))) {
		pExpr->Value.Value -= (g_pCurrentSection->PC + g_pCurrentSection->BasePC + g_pCurrentSection->OrgOffset - nAdjust);
		return pExpr;
	} else if (g_pCurrentSection->Flags & (SECTF_LOADFIXED | SECTF_ORGFIXED)) {
		return expr_Add(
            pExpr,
            expr_Const(nAdjust - (g_pCurrentSection->PC + g_pCurrentSection->BasePC + g_pCurrentSection->OrgOffset))
        );
	} else {
		SExpression* r = (SExpression*) mem_Alloc(sizeof(SExpression));

		r->Value.Value = 0;
		r->type = EXPR_PC_RELATIVE;
		r->isConstant = false;
		r->left = expr_Const(nAdjust);
		r->right = pExpr;
		return r;
	}
}

SExpression* expr_Pc() {
	char symbolName[MAXSYMNAMELENGTH + 20];
	sprintf(symbolName, "$%s%u", str_String(g_pCurrentSection->Name), g_pCurrentSection->PC);

	string* nameString = str_Create(symbolName);
	SSymbol* symbol = sym_CreateLabel(nameString);
	str_Free(nameString);

	SExpression* r = (SExpression*) mem_Alloc(sizeof(SExpression));

	if (symbol->nFlags & SYMF_CONSTANT) {
		r->Value.Value = symbol->Value.Value;
		r->type = EXPR_CONSTANT;
		r->isConstant = true;
		r->left = NULL;
		r->right = NULL;
	} else {
		r->right = NULL;
		r->left = NULL;
		r->Value.pSymbol = symbol;
		r->isConstant = false;
		r->type = EXPR_SYMBOL;
	}

	return r;
}

void expr_SetConst(SExpression* expression, int32_t nValue) {
	expression->left = NULL;
	expression->right = NULL;
	expression->type = EXPR_CONSTANT;
	expression->isConstant = true;
	expression->Value.Value = nValue;
}

SExpression* expr_Const(int32_t value) {
	SExpression* r = (SExpression*) mem_Alloc(sizeof(SExpression));
	expr_SetConst(r, value);

	return r;
}

SExpression* expr_Sub(SExpression* left, SExpression* right) {
	if (!expr_VerifyPointers(left, right))
		return NULL;

	int32_t value = left->Value.Value - right->Value.Value;

	left = mergeExpressions(left, right);
	left->type = EXPR_OPERATION;
	left->operation = T_OP_SUB;
	left->Value.Value = value;

	if (expr_IsConstant(left) == 0
	&& left->left != NULL
	&& left->right != NULL
	&& left->left->type == EXPR_SYMBOL
	&& left->right->type == EXPR_SYMBOL
	&& left->left->Value.pSymbol->pSection == left->right->Value.pSymbol->pSection) {
		left->Value.Value = left->left->Value.pSymbol->Value.Value - left->right->Value.pSymbol->Value.Value;
	}
	return left;
}

SExpression* expr_FixedDivision(SExpression* left, SExpression* right) {
	if (!expr_VerifyPointers(left, right))
		return NULL;

	if (right->Value.Value != 0) {
		int32_t value = fdiv(left->Value.Value, right->Value.Value);

		left = mergeExpressions(left, right);
		left->type = EXPR_OPERATION;
		left->operation = T_FUNC_FDIV;
		left->Value.Value = value;
		return left;
	}

	prj_Fail(ERROR_ZERO_DIVIDE);
	return NULL;
}

SExpression* expr_CheckRange(SExpression* expression, int32_t low, int32_t high) {
	SExpression* exprLow = expr_Const(low);
	SExpression* exprHigh = expr_Const(high);

	expression = expr_LowLimit(expression, exprLow);
	if (expression != NULL)
		return expr_HighLimit(expression, exprHigh);

	expr_Free(exprHigh);
	return NULL;
}

SExpression* expr_Bank(const char* symbolName) {
	if (!g_pConfiguration->bSupportBanks)
		internalerror("Banks not supported");

	string* nameString = str_Create(symbolName);

	SExpression* r = (SExpression*) mem_Alloc(sizeof(SExpression));
	r->right = NULL;
	r->left = NULL;
	r->Value.pSymbol = sym_FindSymbol(nameString);
	r->Value.pSymbol->nFlags |= SYMF_REFERENCED;
	r->isConstant = false;
	r->type = EXPR_OPERATION;
	r->operation = T_FUNC_BANK;

	str_Free(nameString);

	return r;
}

SExpression* expr_Symbol(const char* symbolName) {
	string* nameString = str_Create(symbolName);
	SSymbol* symbol = sym_FindSymbol(nameString);
	str_Free(nameString);

	if (symbol->nFlags & SYMF_EXPR) {
		symbol->nFlags |= SYMF_REFERENCED;

		if (symbol->nFlags & SYMF_CONSTANT) {
			return expr_Const(sym_GetValue(symbol));
		} else {
			SExpression* r = (SExpression*) mem_Alloc(sizeof(SExpression));

			r->right = NULL;
			r->left = NULL;
			r->Value.pSymbol = symbol;
			r->isConstant = false;
			r->type = EXPR_SYMBOL;

			return r;
		}
	}

	prj_Fail(ERROR_SYMBOL_IN_EXPR);
	return NULL;
}
