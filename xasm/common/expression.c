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
#include <assert.h>

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


/* Internal functions */

#define COMBINE(NAME, OP, TOKEN) \
SExpression*                                     \
expr_ ## NAME(SExpression* left, SExpression* right) { \
    if (!assertExpressions(left, right))            \
        return NULL;                             \
    int32_t val = OP;                            \
    left = mergeExpressions(left, right, TOKEN); \
    left->value.integer = val;                   \
    return left;                                 \
}

#define COMBINE_BITWISE(NAME, OP, TOKEN) \
    COMBINE(NAME, (uint32_t) left->value.integer OP (uint32_t) right->value.integer, TOKEN)

#define COMBINE_ARITHMETIC(NAME, OP, TOKEN) \
    COMBINE(NAME, left->value.integer OP right->value.integer, TOKEN)

#define CREATE_BINARY_FUNC(NAME, OP, TOKEN) \
    COMBINE(NAME, OP(left->value.integer, right->value.integer), TOKEN)

#define CREATE_LIMIT(NAME, OP, TOKEN) \
SExpression*                                              \
expr_ ## NAME(SExpression* expr, SExpression* bound) {    \
    if (!assertExpressions(expr, bound))                  \
        return NULL;                                      \
    int32_t value = expr->value.integer;                  \
    if(expr_IsConstant(expr) && expr_IsConstant(bound)) { \
        if(expr->value.integer OP bound->value.integer) { \
            expr_Free(expr);                              \
            expr_Free(bound);                             \
            return NULL;                                  \
        }                                                 \
    }                                                     \
    expr = mergeExpressions(expr, bound, TOKEN);          \
    expr->value.integer = value;                          \
    return expr;                                          \
}

#define CREATE_UNARY_FUNC(NAME, FUNC, TOKEN)      \
SExpression*                                      \
expr_ ## NAME(SExpression* expr) {                \
    if (!assertExpression(expr))                  \
        return NULL;                              \
    SExpression* r = (SExpression*) mem_Alloc(sizeof(SExpression)); \
    r->right = expr;                              \
    r->left = NULL;                               \
    r->value.integer = FUNC(expr->value.integer); \
    r->isConstant = expr->isConstant;             \
    r->type = EXPR_OPERATION;                     \
    r->operation = TOKEN;                         \
    return r;                                     \
}

#define CREATE_EXPR_DIV(NAME, OP, TOKEN) \
SExpression*                                                     \
expr_ ## NAME(SExpression* left, SExpression* right) {           \
    if (!assertExpressions(left, right))                         \
        return NULL;                                             \
    if (right->value.integer == 0) {                             \
        prj_Fail(ERROR_ZERO_DIVIDE);                             \
        return NULL;                                             \
    }                                                            \
    int32_t value = left->value.integer OP right->value.integer; \
    SExpression* r = mergeExpressions(left, right, TOKEN);       \
    r->value.integer = value;                                    \
    return r;                                                    \
}

static bool
assertExpressions(SExpression* left, SExpression* right) {
    if (left != NULL && right != NULL)
        return true;

    prj_Fail(ERROR_INVALID_EXPRESSION);
    expr_Free(left);
    expr_Free(right);
    return false;
}

static bool
assertExpression(SExpression* expression) {
    if (expression != NULL)
        return true;

    prj_Fail(ERROR_INVALID_EXPRESSION);
    expr_Free(expression);
    return false;
}

static SExpression*
mergeExpressions(SExpression* left, SExpression* right, EToken operation) {
    SExpression* expr;

    if (!assertExpressions(left, right))
        return NULL;

    expr = (SExpression*) mem_Alloc(sizeof(SExpression));

    expr->isConstant = left->isConstant && right->isConstant;
    expr->left = left;
    expr->right = right;
    expr->type = EXPR_OPERATION;
    expr->operation = operation;
    return expr;
}

static bool
isSymbol(SExpression* expression) {
    return expression != NULL && expression->type == EXPR_SYMBOL;
}


/* Expression creation functions */

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

SExpression*
expr_Parens(SExpression* expression) {
    if (!assertExpression(expression))
        return NULL;

    SExpression* r = (SExpression*) mem_Alloc(sizeof(SExpression));
    r->right = expression;
    r->left = NULL;
    r->value.integer = expression->value.integer;
    r->isConstant = expression->isConstant;
    r->type = EXPR_PARENS;
    r->operation = T_NONE;
    return r;
}

SExpression*
expr_Bit(SExpression* right) {
    if (!assertExpression(right))
        return NULL;

    uint32_t v = (uint32_t) right->value.integer;

    if (expr_IsConstant(right) && !isPowerOfTwo(v)) {
        prj_Error(ERROR_EXPR_TWO_POWER);
        return NULL;
    }

    SExpression* r = (SExpression*) mem_Alloc(sizeof(SExpression));
    r->right = right;
    r->left = NULL;
    r->value.integer = log2n(v);
    r->isConstant = right->isConstant;
    r->type = EXPR_OPERATION;
    r->operation = T_OP_BIT;
    return r;
}

SExpression*
expr_PcRelative(SExpression* pExpr, int adjustment) {
    if (!assertExpression(pExpr))
        return NULL;

    if (expr_IsConstant(pExpr) && (g_pCurrentSection->Flags & (SECTF_LOADFIXED | SECTF_ORGFIXED))) {
        pExpr->value.integer -=
                (g_pCurrentSection->PC + g_pCurrentSection->BasePC + g_pCurrentSection->OrgOffset - adjustment);
        return pExpr;
    } else if (g_pCurrentSection->Flags & (SECTF_LOADFIXED | SECTF_ORGFIXED)) {
        return expr_Add(pExpr, expr_Const(
                adjustment - (g_pCurrentSection->PC + g_pCurrentSection->BasePC + g_pCurrentSection->OrgOffset)));
    } else {
        SExpression* r = (SExpression*) mem_Alloc(sizeof(SExpression));

        r->value.integer = 0;
        r->type = EXPR_PC_RELATIVE;
        r->isConstant = false;
        r->left = expr_Const(adjustment);
        r->right = pExpr;
        return r;
    }
}

SExpression*
expr_Pc() {
    char symbolName[MAXSYMNAMELENGTH + 20];
    sprintf(symbolName, "$%s%u", str_String(g_pCurrentSection->Name), g_pCurrentSection->PC);

    string* nameString = str_Create(symbolName);
    SSymbol* symbol = sym_CreateLabel(nameString);
    str_Free(nameString);

    SExpression* r = (SExpression*) mem_Alloc(sizeof(SExpression));

    if (symbol->nFlags & SYMF_CONSTANT) {
        r->value.integer = symbol->Value.Value;
        r->type = EXPR_CONSTANT;
        r->isConstant = true;
        r->left = NULL;
        r->right = NULL;
    } else {
        r->right = NULL;
        r->left = NULL;
        r->value.symbol = symbol;
        r->isConstant = false;
        r->type = EXPR_SYMBOL;
    }

    return r;
}

SExpression*
expr_Const(int32_t value) {
    SExpression* r = (SExpression*) mem_Alloc(sizeof(SExpression));
    expr_SetConst(r, value);

    return r;
}

SExpression*
expr_Sub(SExpression* left, SExpression* right) {
    if (!assertExpressions(left, right))
        return NULL;

    int32_t value = left->value.integer - right->value.integer;

    left = mergeExpressions(left, right, T_OP_SUB);
    left->value.integer = value;

    if (!expr_IsConstant(left) && isSymbol(left->left) && isSymbol(left->right)
        && left->left->value.symbol->pSection == left->right->value.symbol->pSection) {
        left->value.integer = left->left->value.symbol->Value.Value - left->right->value.symbol->Value.Value;
    }
    return left;
}

SExpression*
expr_FixedDivision(SExpression* left, SExpression* right) {
    if (!assertExpressions(left, right))
        return NULL;

    if (right->value.integer != 0) {
        int32_t value = fdiv(left->value.integer, right->value.integer);

        left = mergeExpressions(left, right, T_FUNC_FDIV);
        left->value.integer = value;
        return left;
    }

    prj_Fail(ERROR_ZERO_DIVIDE);
    return NULL;
}

SExpression*
expr_CheckRange(SExpression* expression, int32_t low, int32_t high) {
    SExpression* exprLow = expr_Const(low);
    SExpression* exprHigh = expr_Const(high);

    expression = expr_LowLimit(expression, exprLow);
    if (expression != NULL)
        return expr_HighLimit(expression, exprHigh);

    expr_Free(exprHigh);
    return NULL;
}

SExpression*
expr_Bank(string* symbolName) {
    assert(g_pConfiguration->bSupportBanks);

    SExpression* r = (SExpression*) mem_Alloc(sizeof(SExpression));
    r->right = NULL;
    r->left = NULL;
    r->value.symbol = sym_FindSymbol(symbolName);
    r->value.symbol->nFlags |= SYMF_REFERENCED;
    r->isConstant = false;
    r->type = EXPR_OPERATION;
    r->operation = T_FUNC_BANK;

    return r;
}

SExpression*
expr_Symbol(string* symbolName) {
    SSymbol* symbol = sym_FindSymbol(symbolName);

    if (symbol->nFlags & SYMF_EXPR) {
        symbol->nFlags |= SYMF_REFERENCED;

        if (symbol->nFlags & SYMF_CONSTANT) {
            return expr_Const(sym_GetValue(symbol));
        } else {
            SExpression* r = (SExpression*) mem_Alloc(sizeof(SExpression));

            r->right = NULL;
            r->left = NULL;
            r->value.symbol = symbol;
            r->isConstant = false;
            r->type = EXPR_SYMBOL;

            return r;
        }
    }

    prj_Fail(ERROR_SYMBOL_IN_EXPR);
    return NULL;
}

void
expr_SetConst(SExpression* expression, int32_t nValue) {
    expression->left = NULL;
    expression->right = NULL;
    expression->type = EXPR_CONSTANT;
    expression->isConstant = true;
    expression->value.integer = nValue;
}

void
expr_Clear(SExpression* expression) {
    if (expression != NULL) {
        expr_Free(expression->left);
        expression->left = NULL;

        expr_Free(expression->right);
        expression->right = NULL;

        expression->isConstant = true;
    }
}

void
expr_Free(SExpression* expression) {
    if (expression != NULL) {
        expr_Free(expression->left);
        expr_Free(expression->right);
        mem_Free(expression);
    }
}
