/*  Copyright 2008-2022 Carsten Elton Sorensen and contributors

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
#include "util.h"
#include "fmath.h"
#include "mem.h"

// From xasm
#include "xasm.h"
#include "expression.h"
#include "symbol.h"
#include "section.h"
#include "tokens.h"
#include "errors.h"


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
        err_Error(ERROR_ZERO_DIVIDE);                             \
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

    err_Fail(ERROR_INVALID_EXPRESSION);
    expr_Free(left);
    expr_Free(right);
    return false;
}

static bool
assertExpression(SExpression* expression) {
    if (expression != NULL)
        return true;

    err_Fail(ERROR_INVALID_EXPRESSION);
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

CREATE_BINARY_FUNC(FixedMultiplication, fmul, T_OP_FMUL)

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
        err_Error(ERROR_EXPR_TWO_POWER);
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
expr_PcRelative(SExpression* expression, int adjustment) {
    if (!assertExpression(expression))
        return NULL;

    if (expr_IsConstant(expression) && sect_Current != NULL && (sect_Current->flags & (SECTF_LOADFIXED | SECTF_ORGFIXED))) {
        expression->value.integer -= (sect_Current->cpuProgramCounter + sect_Current->cpuOrigin + sect_Current->cpuAdjust - adjustment);
        return expression;
    } else if (sect_Current != NULL && sect_Current->flags & (SECTF_LOADFIXED | SECTF_ORGFIXED)) {
        return expr_Add(
            expression,
            expr_Const(adjustment - (sect_Current->cpuProgramCounter + sect_Current->cpuOrigin + sect_Current->cpuAdjust))
        );
    } else {
        SExpression* r = (SExpression*) mem_Alloc(sizeof(SExpression));

        r->value.integer = 0;
        r->type = EXPR_PC_RELATIVE;
        r->isConstant = false;
        r->left = expr_Const(adjustment);
        r->right = expression;
        return r;
    }
}

SExpression*
expr_Pc(void) {
    string* nameString = str_CreateFormat("$%s%u", str_String(sect_Current->name), sect_Current->cpuProgramCounter);
    SSymbol* symbol = sym_CreateLabel(nameString);
    str_Free(nameString);

	if (symbol == NULL)
		return NULL;

    SExpression* r = (SExpression*) mem_Alloc(sizeof(SExpression));

    if (symbol->flags & SYMF_CONSTANT) {
        r->value.integer = symbol->value.integer;
        r->type = EXPR_INTEGER_CONSTANT;
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
    && left->left->value.symbol->section == left->right->value.symbol->section
    && left->left->value.symbol->type == SYM_LABEL
    && left->right->value.symbol->type == SYM_LABEL) {
        int32_t newValue = left->left->value.symbol->value.integer - left->right->value.symbol->value.integer;
        expr_Free(left);
        return expr_Const(newValue);
    }
    return left;
}

SExpression*
expr_FixedDivision(SExpression* left, SExpression* right) {
    if (!assertExpressions(left, right))
        return NULL;

    if (right->value.integer != 0) {
        int32_t value = fdiv(left->value.integer, right->value.integer);

        left = mergeExpressions(left, right, T_OP_FDIV);
        left->value.integer = value;
        return left;
    }

    err_Error(ERROR_ZERO_DIVIDE);
    return NULL;
}

SExpression*
expr_CheckRange(SExpression* expression, int32_t low, int32_t high) {
    expression = expr_LowLimit(expression, expr_Const(low));
    if (expression != NULL) {
        expression = expr_HighLimit(expression, expr_Const(high));
		if (expression != NULL)
			return expression;
    }

    err_Error(ERROR_OPERAND_RANGE);

    return NULL;
}


SExpression*
expr_Assert(SExpression* expression, SExpression* assertion) {
    if (!assertExpressions(expression, assertion))
        return NULL;

    if (expr_IsConstant(assertion)) {
         if (assertion->value.integer != 0)
            return expression;
        
        err_Error(ERROR_OPERAND_RANGE);
        return NULL;
    }

    return mergeExpressions(expression, assertion, T_FUNC_ASSERT);
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
expr_Symbol(SSymbol* symbol) {
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

    return NULL;
}

SExpression*
expr_SymbolByName(string* symbolName) {
    SSymbol* symbol = sym_GetSymbol(symbolName);
    return expr_Symbol(symbol);
}

void
expr_SetConst(SExpression* expression, int32_t nValue) {
    expression->left = NULL;
    expression->right = NULL;
    expression->type = EXPR_INTEGER_CONSTANT;
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

SExpression*
expr_Copy(SExpression* expression) {
    if (expression == NULL)
        return NULL;

    SExpression* r = (SExpression*) mem_Alloc(sizeof(SExpression));
    r->isConstant = expression->isConstant;
    r->left = expr_Copy(expression->left);
    r->right = expr_Copy(expression->right);
    r->operation = expression->operation;
    r->type = expression->type;
    r->value = expression->value;

    return r;
}

bool
expr_GetSectionOffset(SExpression* expression, SSection* section, uint32_t* resultOffset) {
    if (expression == NULL)
        return false;

    if (expr_Type(expression) == EXPR_PARENS) {
        return expr_GetSectionOffset(expression->right, section, resultOffset);
    }

    if (expr_Type(expression) == EXPR_INTEGER_CONSTANT && (section->flags & SECTF_LOADFIXED)) {
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

SExpression* 
expr_Clone(SExpression* expression) {
    if (expression == NULL)
        return NULL;

    SExpression* result = (SExpression *) mem_Alloc(sizeof(SExpression));
    result->isConstant = expression->isConstant;
    result->operation = expression->operation;
    result->type = expression->type;
    result->value = expression->value;

    result->left = expr_Clone(expression->left);
    result->right = expr_Clone(expression->right);
    return result;
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
        expression->type = EXPR_INTEGER_CONSTANT;
        expression->isConstant = true;
        expression->value.integer = expression->value.symbol->value.integer;
    }

    if (expr_IsConstant(expression)) {
        expr_Free(expression->left);
        expression->left = NULL;
        expr_Free(expression->right);
        expression->right = NULL;

        expression->type = EXPR_INTEGER_CONSTANT;
        expression->operation = T_NONE;
    }
}

static bool
isImport(const SSymbol* symbol) {
    return symbol->type == SYM_IMPORT || symbol->type == SYM_GLOBAL;
}

static bool
isSymbolic(const SSymbol* symbol) {
    return symbol->type == SYM_IMPORT || symbol->type == SYM_GLOBAL || symbol->type == SYM_LABEL;
}

static bool
getSymbolOffset(uint32_t* resultOffset, SSymbol** resultSymbol, SExpression* expression, bool (*predicate)(const SSymbol*)) {
    if (expression == NULL)
        return false;

    if (expr_Type(expression) == EXPR_SYMBOL) {
        SSymbol* symbol = expression->value.symbol;
        if (predicate(symbol)) {
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


extern bool
expr_GetImportOffset(uint32_t* resultOffset, SSymbol** resultSymbol, SExpression* expression) {
    *resultSymbol = NULL;
    return getSymbolOffset(resultOffset, resultSymbol, expression, isImport);
}

extern bool
expr_GetSymbolOffset(uint32_t* resultOffset, SSymbol** resultSymbol, SExpression* expression) {
    *resultSymbol = NULL;
    return getSymbolOffset(resultOffset, resultSymbol, expression, isSymbolic);
}
