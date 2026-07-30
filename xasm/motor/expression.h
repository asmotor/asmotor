/*  Copyright 2008-2026 Carsten Elton Sorensen and contributors

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

#ifndef XASM_MOTOR_EXPRESSION_H_INCLUDED_
#define XASM_MOTOR_EXPRESSION_H_INCLUDED_

#include <stdbool.h>
#include <stddef.h>

#include "section.h"
#include "str.h"
#include "symbol.h"
#include "tokens.h"
#include "util.h"

struct Symbol;

/*
 * EXPRESSION OWNERSHIP CONTRACT
 *
 * Every SExpression* has exactly one owner responsible for expr_Free().
 * Expressions are NOT reference-counted — each allocation has one owner.
 *
 * PARAMETER CONVENTIONS:
 *   - const SExpression* — borrowed, function does NOT take ownership.
 *                          caller retains ownership and must free.
 *   - SExpression* — consumed, function takes ownership.
 *                    caller must NOT use the pointer after the call.
 *
 * RETURN VALUES:
 *   - SExpression* — caller owns the result; must eventually free via expr_Free().
 *   - NULL on error — inputs are already freed internally.
 *
 * OWNERSHIP HELPERS:
 *   - expr_Move(&dest, &src) — transfer ownership, sets src to NULL.
 *   - expr_Clear(&dest) — free *dest and set to NULL.
 *
 * NESTED CALLS: expr_Add(expr_Const(1), expr_Sub(a, b)) — each sub-call
 *   produces an owned expression consumed by the outer call. The caller
 *   owns the final result.
 *
 * CLONING: expr_Clone(const SExpression*) creates a deep copy. Use when
 *   an expression must be consumed by multiple callers.
 */

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

/* Free — consumes input (forward declaration for inline helpers) */

extern void
expr_Free(SExpression* expression);

/* Ownership helpers */

INLINE void
expr_Move(SExpression** dest, SExpression** src) {
	expr_Free(*dest);
	*dest = *src;
	*src = NULL;
}

INLINE void
expr_Clear(SExpression** dest) {
	expr_Free(*dest);
	*dest = NULL;
}

/* Accessors — borrow inputs */

INLINE EExpressionType
expr_Type(const SExpression* expression) {
	return expression->type;
}

INLINE bool
expr_IsOperator(const SExpression* expression, EToken operation) {
	return expression != NULL && expression->type == EXPR_OPERATION && expression->operation == operation;
}

INLINE bool
expr_IsConstant(const SExpression* expression) {
	return expression != NULL && expression->isConstant;
}

/* Leaf constructors — produce owned result, no consumed inputs */

extern SExpression*
expr_Const(int32_t value);

extern SExpression*
expr_Pc(void);

extern SExpression*
expr_Symbol(SSymbol* symbol);

extern SExpression*
expr_SymbolByName(string* symbolName);

extern SExpression*
expr_Bank(SSymbol* symbol);

/* Binary operators — consume both inputs, produce owned result */

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
expr_And(SExpression* left, SExpression* right);

extern SExpression*
expr_Or(SExpression* left, SExpression* right);

extern SExpression*
expr_Xor(SExpression* left, SExpression* right);

extern SExpression*
expr_Asl(SExpression* left, SExpression* right);

extern SExpression*
expr_Asr(SExpression* left, SExpression* right);

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
expr_BooleanOr(SExpression* left, SExpression* right);

extern SExpression*
expr_BooleanAnd(SExpression* left, SExpression* right);

extern SExpression*
expr_Atan2(SExpression* left, SExpression* right);

extern SExpression*
expr_FixedMultiplication(SExpression* left, SExpression* right);

extern SExpression*
expr_FixedDivision(SExpression* left, SExpression* right);

/* Unary operators — consume input, produce owned result */

extern SExpression*
expr_BooleanNot(SExpression* expr);

extern SExpression*
expr_Bit(SExpression* expr);

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
expr_Parens(SExpression* expression);

/* Special functions — consume input, produce owned result */

extern SExpression*
expr_CheckRange(SExpression* expression, int32_t low, int32_t high);

extern SExpression*
expr_Assert(SExpression* expression, SExpression* assertion);

extern SExpression*
expr_PcRelative(SExpression* expression, int adjustment);

/* Mutation — borrow input, modify in place */

extern void
expr_SetConst(SExpression* expression, int32_t value);

/* expr_Reset: frees children of the expression and nullifies them.
 * Used before expr_SetConst to avoid leaking child nodes. */
extern void
expr_Reset(SExpression* expression);

/*
 * expr_Optimize: mutates the owned expression tree in place.
 * The caller must own the expression (e.g., patch owns its expression).
 * Converts constant symbols to integer constants and prunes subtrees
 * of constant expressions. Used once per patch before backpatching.
 */
extern void
expr_Optimize(SExpression* expression);

/* Clone — borrows input, produces owned deep copy */

extern SExpression*
expr_Clone(const SExpression* expression);

/* Queries — borrow inputs */

extern bool
expr_GetSectionOffset(const SExpression* expression, const SSection* section, uint32_t* resultOffset);

extern bool
expr_IsRelativeToSection(const SExpression* expression, const SSection* section);

extern SSection*
expr_GetSectionAndOffset(const SExpression* expression, uint32_t* resultOffset);

extern bool
expr_GetImportOffset(uint32_t* resultOffset, SSymbol** resultSymbol, const SExpression* expression);

extern bool
expr_GetSymbolOffset(uint32_t* resultOffset, SSymbol** resultSymbol, const SExpression* expression);

#endif /* XASM_MOTOR_EXPRESSION_H_INCLUDED_ */
