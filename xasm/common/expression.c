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

static bool
getSymbolSectionOffset(const SExpression* expression, const SSection* section, uint32_t* resultOffset) {
    SSymbol* symbol = expression->value.symbol;
    if ((symbol->type == SYM_EQU) && (section->flags & SECTF_LOADFIXED)) {
        *resultOffset = symbol->value.integer - section->cpuOrigin;
        return true;
    } else if (symbol->section == section) {
        if ((symbol->flags & SYMF_CONSTANT) && (section->flags & SECTF_LOADFIXED)) {
            *resultOffset = symbol->value.integer - section->cpuOrigin;
            return true;
        } else if ((symbol->flags & SYMF_RELOC) && (section->flags == 0)) {
            *resultOffset = (uint32_t) symbol->value.integer;
            return true;
        }
    }
    return false;
}

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

CREATE_EXPR_DIV(Div, /, T_OP_DIVIDE)

CREATE_EXPR_DIV(Mod, %, T_OP_MODULO)

CREATE_UNARY_FUNC(BooleanNot, !, T_OP_BOOLEAN_NOT)

COMBINE_BITWISE(Xor, ^, T_OP_BITWISE_XOR)

COMBINE_BITWISE(Or, |, T_OP_BITWISE_OR)

COMBINE_BITWISE(And, &, T_OP_BITWISE_AND)

COMBINE_BITWISE(Asl, <<, T_OP_BITWISE_ASL)

COMBINE_ARITHMETIC(Add, +, T_OP_ADD)

COMBINE_ARITHMETIC(Mul, *, T_OP_ADD)

COMBINE_ARITHMETIC(BooleanOr, ||, T_OP_BOOLEAN_OR)

COMBINE_ARITHMETIC(BooleanAnd, &&, T_OP_BOOLEAN_AND)

COMBINE_ARITHMETIC(GreaterEqual, >=, T_OP_GREATER_OR_EQUAL)

COMBINE_ARITHMETIC(GreaterThan, >, T_OP_GREATER_THAN)

COMBINE_ARITHMETIC(LessEqual, <=, T_OP_LESS_OR_EQUAL)

COMBINE_ARITHMETIC(LessThan, <, T_OP_LESS_THAN)

COMBINE_ARITHMETIC(Equal, ==, T_OP_EQUAL)

COMBINE_ARITHMETIC(NotEqual, !=, T_OP_NOT_EQUAL)

CREATE_LIMIT(LowLimit, <, T_FUNC_LOWLIMIT)

CREATE_LIMIT(HighLimit, >, T_FUNC_HIGHLIMIT)

CREATE_BINARY_FUNC(FixedMultiplication, fmul, T_FUNC_FMUL)

CREATE_BINARY_FUNC(Atan2, fatan2, T_FUNC_ATAN2)

CREATE_BINARY_FUNC(Asr, asr, T_OP_BITWISE_ASR)

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

    if (expr_IsConstant(pExpr) && (sect_Current->flags & (SECTF_LOADFIXED | SECTF_ORGFIXED))) {
        pExpr->value.integer -=
                (sect_Current->cpuProgramCounter + sect_Current->cpuOrigin + sect_Current->cpuAdjust - adjustment);
        return pExpr;
    } else if (sect_Current->flags & (SECTF_LOADFIXED | SECTF_ORGFIXED)) {
        return expr_Add(pExpr, expr_Const(
                adjustment - (sect_Current->cpuProgramCounter + sect_Current->cpuOrigin + sect_Current->cpuAdjust)));
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
    char symbolName[MAX_SYMBOL_NAME_LENGTH + 20];
    sprintf(symbolName, "$%s%u", str_String(sect_Current->name), sect_Current->cpuProgramCounter);

    string* nameString = str_Create(symbolName);
    SSymbol* symbol = sym_CreateLabel(nameString);
    str_Free(nameString);

    SExpression* r = (SExpression*) mem_Alloc(sizeof(SExpression));

    if (symbol->flags & SYMF_CONSTANT) {
        r->value.integer = symbol->value.integer;
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

    left = mergeExpressions(left, right, T_OP_SUBTRACT);
    left->value.integer = value;

    if (!expr_IsConstant(left) && isSymbol(left->left) && isSymbol(left->right)
        && left->left->value.symbol->section == left->right->value.symbol->section) {
        left->value.integer = left->left->value.symbol->value.integer - left->right->value.symbol->value.integer;
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
    assert(xasm_Configuration->supportBanks);

    SExpression* r = (SExpression*) mem_Alloc(sizeof(SExpression));
    r->right = NULL;
    r->left = NULL;
    r->value.symbol = sym_GetSymbol(symbolName);
    r->isConstant = false;
    r->type = EXPR_OPERATION;
    r->operation = T_FUNC_BANK;

    return r;
}

SExpression*
expr_Symbol(string* symbolName) {
    SSymbol* symbol = sym_GetSymbol(symbolName);

    if (symbol->flags & SYMF_EXPRESSION) {
        if (symbol->flags & SYMF_CONSTANT) {
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

bool
expr_GetSectionOffset(SExpression* expression, SSection* section, uint32_t* resultOffset) {
    if (expression == NULL)
        return false;

    if (expr_Type(expression) == EXPR_PARENS) {
        return expr_GetSectionOffset(expression->right, section, resultOffset);
    }

    if (expr_Type(expression) == EXPR_CONSTANT && (section->flags & SECTF_LOADFIXED)) {
        *resultOffset = expression->value.integer - section->cpuOrigin;
        return true;
    }

    if (expr_Type(expression) == EXPR_SYMBOL) {
        return getSymbolSectionOffset(expression, section, resultOffset);
    }

    if (expr_IsOperator(expression, T_OP_ADD) || expr_IsOperator(expression, T_OP_SUBTRACT)) {
        uint32_t offset;
        if (expr_GetSectionOffset(expression->left, section, &offset)) {
            if (expr_IsConstant(expression->right)) {
                if (expr_IsOperator(expression, T_OP_ADD))
                    *resultOffset = offset + expression->right->value.integer;
                else
                    *resultOffset = offset - expression->right->value.integer;
                return true;
            }
        }
    }

    if (expr_IsOperator(expression, T_OP_ADD)) {
        uint32_t offset;
        if (expr_GetSectionOffset(expression->right, section, &offset)) {
            if (expr_IsConstant(expression->left)) {
                *resultOffset = expression->left->value.integer + offset;
                return true;
            }
        }
    }

    return false;
}

bool
expr_IsRelativeToSection(SExpression* expression, SSection* section) {
    uint32_t throwAway;
    return expr_GetSectionOffset(expression, section, &throwAway);
}

SSection*
expr_GetSectionAndOffset(SExpression* expression, uint32_t* resultOffset) {
    for (SSection* section = sect_Sections; section != NULL; section = list_GetNext(section)) {
        if (expr_GetSectionOffset(expression, section, resultOffset))
            return section;
    }

    *resultOffset = 0;
    return NULL;
}

void
expr_Optimize(SExpression* expression) {
    if (expression->left != NULL)
        expr_Optimize(expression->left);

    if (expression->right != NULL)
        expr_Optimize(expression->right);

    if (expr_Type(expression) == EXPR_PARENS) {
        SExpression* pToFree = expression->right;
        *expression = *(expression->right);
        mem_Free(pToFree);
    }

    if ((expression->type == EXPR_SYMBOL) && (expression->value.symbol->flags & SYMF_CONSTANT)) {
        expression->type = EXPR_CONSTANT;
        expression->isConstant = true;
        expression->value.integer = expression->value.symbol->value.integer;
    }

    if (expr_IsConstant(expression)) {
        expr_Free(expression->left);
        expression->left = NULL;
        expr_Free(expression->right);
        expression->right = NULL;

        expression->type = EXPR_CONSTANT;
        expression->operation = T_NONE;
    }
}

bool
expr_GetImportOffset(uint32_t* resultOffset, SSymbol** resultSymbol, SExpression* expression) {
    if (expression == NULL)
        return false;

    if (expr_Type(expression) == EXPR_SYMBOL) {
        SSymbol* symbol = expression->value.symbol;
        if (symbol->type == SYM_IMPORT || symbol->type == SYM_GLOBAL) {
            if (*resultSymbol != NULL)
                return false;

            *resultSymbol = symbol;
            *resultOffset = 0;
            return true;
        }
        return false;
    } else if (expr_IsOperator(expression, T_OP_ADD) || expr_IsOperator(expression, T_OP_SUBTRACT)) {
        uint32_t offset;
        if (expr_GetImportOffset(&offset, resultSymbol, expression->left)) {
            if (expr_IsConstant(expression->right)) {
                if (expr_IsOperator(expression, T_OP_ADD))
                    *resultOffset = offset + expression->right->value.integer;
                else
                    *resultOffset = offset - expression->right->value.integer;
                return true;
            }
        }
        if (expr_GetImportOffset(&offset, resultSymbol, expression->right)) {
            if (expr_IsConstant(expression->left)) {
                if (expr_IsOperator(expression, T_OP_ADD))
                    *resultOffset = expression->left->value.integer + offset;
                else
                    *resultOffset = expression->left->value.integer - offset;
                return true;
            }
        }
    }
    return false;
}
