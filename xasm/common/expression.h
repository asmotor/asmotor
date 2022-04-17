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

#ifndef XASM_COMMON_EXPRESSION_H_INCLUDED_
#define XASM_COMMON_EXPRESSION_H_INCLUDED_

#include "util.h"
#include "str.h"
#include "section.h"
#include "symbol.h"
#include "tokens.h"

struct Symbol;

typedef enum {
    EXPR_OPERATION,
    EXPR_PC_RELATIVE,
    EXPR_INTEGER_CONSTANT,
    EXPR_SYMBOL,
    EXPR_PARENS
} EExpressionType;

typedef struct Expression {
    struct Expression* left;
    struct Expression* right;
    EExpressionType type;
    bool isConstant;
    EToken operation;
    union {
        long double floating;
        int32_t integer;
        struct Symbol* symbol;
    } value;
} SExpression;

INLINE EExpressionType
expr_Type(const SExpression* expression) {
    return expression->type;
}

INLINE bool
expr_IsOperator(SExpression* expression, EToken operation) {
    return expression != NULL && expression->type == EXPR_OPERATION && expression->operation == operation;
}

INLINE bool
expr_IsConstant(const SExpression* expression) {
    return expression != NULL && expression->isConstant;
}

extern SExpression*
expr_CheckRange(SExpression* expression, int32_t low, int32_t high);

extern SExpression*
expr_Assert(SExpression* expression, SExpression* assertion);

extern SExpression*
expr_Equal(SExpression* left, SExpression* right);

extern SExpression*
expr_NotEqual(SExpression* left, SExpression* right);

extern SExpression*
expr_GreaterThan(SExpression* left, SExpression* right);

extern SExpression*
expr_LessThan(SExpression* left, SExpression* right);

extern SExpression*
expr_GreaterEqual(SExpression* left, SExpression* right);

extern SExpression*
expr_LessEqual(SExpression* left, SExpression* right);

extern SExpression*
expr_BooleanNot(SExpression* expr);

extern SExpression*
expr_BooleanOr(SExpression* left, SExpression* right);

extern SExpression*
expr_BooleanAnd(SExpression* left, SExpression* right);

extern SExpression*
expr_Or(SExpression* left, SExpression* right);

extern SExpression*
expr_And(SExpression* left, SExpression* right);

extern SExpression*
expr_Xor(SExpression* left, SExpression* right);

extern SExpression*
expr_Add(SExpression* left, SExpression* right);

extern SExpression*
expr_Sub(SExpression* left, SExpression* right);

extern SExpression*
expr_Mul(SExpression* left, SExpression* right);

extern SExpression*
expr_Div(SExpression* left, SExpression* right);

extern SExpression*
expr_Mod(SExpression* left, SExpression* right);

extern SExpression*
expr_Bit(SExpression* expr);

extern SExpression*
expr_Asl(SExpression* left, SExpression* right);

extern SExpression*
expr_Asr(SExpression* left, SExpression* right);

extern SExpression*
expr_Sin(SExpression* expr);

extern SExpression*
expr_Cos(SExpression* expr);

extern SExpression*
expr_Tan(SExpression* expr);

extern SExpression*
expr_Asin(SExpression* expr);

extern SExpression*
expr_Acos(SExpression* expr);

extern SExpression*
expr_Atan(SExpression* expr);

extern SExpression*
expr_Atan2(SExpression* left, SExpression* right);

extern SExpression*
expr_FixedMultiplication(SExpression* left, SExpression* right);

extern SExpression*
expr_FixedDivision(SExpression* left, SExpression* right);

extern SExpression*
expr_Parens(SExpression* expression);

extern SExpression*
expr_PcRelative(SExpression* expr, int adjustment);

extern SExpression*
expr_Pc();

extern SExpression*
expr_Const(int32_t value);

extern void
expr_SetConst(SExpression* expression, int32_t value);

extern SExpression*
expr_Symbol(string* symbolName);

extern SExpression*
expr_ScopedSymbol(SExpression* expr, string* symbolName);

extern SExpression*
expr_Bank(string* symbolName);

extern void
expr_Free(SExpression* expression);

extern SExpression*
expr_Copy(SExpression* expression);

extern void
expr_Clear(SExpression* expression);

extern SExpression*
expr_Clone(SExpression* expression);

extern bool
expr_GetSectionOffset(SExpression* expression, SSection* section, uint32_t* resultOffset);

extern bool
expr_IsRelativeToSection(SExpression* expression, SSection* section);

extern SSection*
expr_GetSectionAndOffset(SExpression* expression, uint32_t* resultOffset);

extern void
expr_Optimize(SExpression* expression);

extern bool
expr_GetImportOffset(uint32_t* resultOffset, SSymbol** resultSymbol, SExpression* expression);

extern bool
expr_GetSymbolOffset(uint32_t* resultOffset, SSymbol** resultSymbol, SExpression* expression);


#endif /* XASM_COMMON_EXPRESSION_H_INCLUDED_ */
