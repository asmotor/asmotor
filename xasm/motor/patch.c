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

#include <memory.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fmath.h"
#include "lists.h"
#include "mem.h"
#include "str.h"
#include "util.h"

#include "errors.h"
#include "expression.h"
#include "lexer_context.h"
#include "patch.h"
#include "section.h"
#include "symbol.h"
#include "tokens.h"
#include "xasm.h"

/* Private functions */

typedef int32_t (*binaryOperation)(int32_t nLeft, int32_t nRight);
typedef int32_t (*unaryOperation)(int32_t nValue);

static bool
evaluateExpression(const SPatch* patch, SExpression* expression, int32_t* result);

static int32_t
subtract(int32_t lhs, int32_t rhs) {
	return lhs - rhs;
}

static int32_t
add(int32_t lhs, int32_t rhs) {
	return lhs + rhs;
}

static int32_t
bitwiseXor(int32_t lhs, int32_t rhs) {
	return (uint32_t) lhs ^ (uint32_t) rhs;
}

static int32_t
bitwiseOr(int32_t lhs, int32_t rhs) {
	return (uint32_t) lhs | (uint32_t) rhs;
}

static int32_t
bitwiseAnd(int32_t lhs, int32_t rhs) {
	return (uint32_t) lhs & (uint32_t) rhs;
}

static int32_t
arithmeticLeftShift(int32_t lhs, int32_t rhs) {
	return ((uint32_t) lhs) << rhs;
}

static int32_t
arithmeticRightShift(int32_t lhs, int32_t rhs) {
	return asr(lhs, rhs);
}

static int32_t
multiply(int32_t lhs, int32_t rhs) {
	return lhs * rhs;
}

static int32_t
divide(int32_t lhs, int32_t rhs) {
	return lhs / rhs;
}

static int32_t
modulo(int32_t lhs, int32_t rhs) {
	return lhs % rhs;
}

static int32_t
booleanAnd(int32_t left, int32_t right) {
	return left && right;
}

static int32_t
booleanOr(int32_t lhs, int32_t rhs) {
	return lhs || rhs;
}

static int32_t
greaterOrEqual(int32_t lhs, int32_t rhs) {
	return lhs >= rhs;
}

static int32_t
greaterThan(int32_t lhs, int32_t rhs) {
	return lhs > rhs;
}

static int32_t
lessOrEqual(int32_t lhs, int32_t rhs) {
	return lhs <= rhs;
}

static int32_t
lessThan(int32_t lhs, int32_t rhs) {
	return lhs < rhs;
}

static int32_t
equals(int32_t lhs, int32_t rhs) {
	return lhs == rhs;
}

static int32_t
notEquals(int32_t lhs, int32_t rhs) {
	return lhs != rhs;
}

static int32_t
booleanNot(int32_t value) {
	return value ? 0 : 1;
}

static bool
evaluateBinary(const SPatch* patch, SExpression* expression, int32_t* result, binaryOperation operation) {
	int32_t lhs;
	int32_t rhs;
	bool leftEvaluated = false;
	bool rightEvaluated = false;

	if (evaluateExpression(patch, expression->left, &lhs)) {
		SExpression* old = expression->left;
		expression->left = expr_Const(lhs);
		expr_Free(old);
		leftEvaluated = true;
	}
	if (evaluateExpression(patch, expression->right, &rhs)) {
		SExpression* old = expression->right;
		expression->right = expr_Const(rhs);
		expr_Free(old);
		rightEvaluated = true;
	}

	if (leftEvaluated && rightEvaluated) {
		*result = operation(lhs, rhs);
		expr_Reset(expression);
		expr_SetConst(expression, *result);
		return true;
	}
	return false;
}

static bool
evaluateUnary(const SPatch* patch, SExpression* expression, int32_t* result, unaryOperation operation) {
	int32_t value;

	if (evaluateExpression(patch, expression->right, &value)) {
		*result = operation(value);
		expr_Free(expression->right);
		expression->right = NULL;
		expression->type = EXPR_INTEGER_CONSTANT;
		expression->isConstant = true;
		expression->value.integer = *result;
		return true;
	}
	return false;
}

static bool
evaluatePcRelative(const SPatch* patch, SExpression* expression, int32_t* result) {
	uint32_t offset;
	if (expr_GetSectionOffset(expression->right, patch->section, &offset)) {
		int32_t adjustment;
		if (evaluateExpression(patch, expression->left, &adjustment)) {
			*result = offset + adjustment - patch->offset;
			expr_Reset(expression);
			expr_SetConst(expression, *result);
			return true;
		}
	}
	return false;
}

static bool
evaluateBit(const SPatch* patch, SExpression* expression, int32_t* result) {
	int32_t value;
	if (!evaluateExpression(patch, expression->right, &value))
		return false;

	if (isPowerOfTwo(value)) {
		int32_t bit = log2n((size_t) value);
		expr_Free(expression->right);
		expression->right = NULL;
		expression->type = EXPR_INTEGER_CONSTANT;
		expression->isConstant = true;
		expression->value.integer = *result = bit;
		return true;
	}

	err_PatchError(patch, ERROR_EXPR_TWO_POWER);
	return false;
}

static bool
evaluateSubtract(const SPatch* patch, SExpression* expression, int32_t* result) {
	uint32_t l, r;

	SSection* pLeftSect = expr_GetSectionAndOffset(expression->left, &l);
	SSection* pRightSect = expr_GetSectionAndOffset(expression->right, &r);

	if (pLeftSect && pRightSect && pLeftSect == pRightSect) {
		*result = l - r;
		expr_Reset(expression);
		expr_SetConst(expression, *result);
		return true;
	}

	return evaluateBinary(patch, expression, result, subtract);
}

	static bool
evaluateLowLimit(const SPatch* patch, SExpression* expression, int32_t* result) {
	int32_t lhs, rhs;

	if (evaluateExpression(patch, expression->right, &rhs) && evaluateExpression(patch, expression->left, &lhs)) {
		if (lhs >= rhs) {
			*result = lhs;
			expr_Reset(expression);
			expr_SetConst(expression, *result);
			return true;
		}
		expr_Reset(expression);
		err_PatchFail(patch, ERROR_OPERAND_RANGE);
	}
	return false;
}

static bool
evaluateHighLimit(const SPatch* patch, SExpression* expression, int32_t* result) {
	int32_t lhs, rhs;

	if (evaluateExpression(patch, expression->right, &rhs) && evaluateExpression(patch, expression->left, &lhs)) {
		if (lhs <= rhs) {
			*result = lhs;
			expr_Reset(expression);
			expr_SetConst(expression, *result);
			return true;
		}
		expr_Reset(expression);
		err_PatchFail(patch, ERROR_OPERAND_RANGE);
	}
	return false;
}

static bool
evaluateAssert(const SPatch* patch, SExpression* expression, int32_t* result) {
	int32_t lhs, rhs;

	if (evaluateExpression(patch, expression->right, &rhs) && evaluateExpression(patch, expression->left, &lhs)) {
		if (rhs != 0) {
			*result = lhs;
			expr_Reset(expression);
			expr_SetConst(expression, *result);
			return true;
		}
		expr_Reset(expression);
		err_PatchFail(patch, ERROR_OPERAND_RANGE);
	}
	return false;
}

static bool
evaluateOperation(const SPatch* patch, SExpression* expression, int32_t* result) {
	switch (expression->operation) {
		case T_OP_SUBTRACT:
			return evaluateSubtract(patch, expression, result);
		case T_OP_ADD:
			return evaluateBinary(patch, expression, result, add);
		case T_OP_BITWISE_XOR:
			return evaluateBinary(patch, expression, result, bitwiseXor);
		case T_OP_BITWISE_OR:
			return evaluateBinary(patch, expression, result, bitwiseOr);
		case T_OP_BITWISE_AND:
			return evaluateBinary(patch, expression, result, bitwiseAnd);
		case T_OP_BITWISE_ASL:
			return evaluateBinary(patch, expression, result, arithmeticLeftShift);
		case T_OP_BITWISE_ASR:
			return evaluateBinary(patch, expression, result, arithmeticRightShift);
		case T_OP_MULTIPLY:
			return evaluateBinary(patch, expression, result, multiply);
		case T_OP_DIVIDE:
			return evaluateBinary(patch, expression, result, divide);
		case T_OP_MODULO:
			return evaluateBinary(patch, expression, result, modulo);
		case T_OP_BOOLEAN_OR:
			return evaluateBinary(patch, expression, result, booleanOr);
		case T_OP_BOOLEAN_AND:
			return evaluateBinary(patch, expression, result, booleanAnd);
		case T_OP_BOOLEAN_NOT:
			return evaluateUnary(patch, expression, result, booleanNot);
		case T_OP_GREATER_OR_EQUAL:
			return evaluateBinary(patch, expression, result, greaterOrEqual);
		case T_OP_GREATER_THAN:
			return evaluateBinary(patch, expression, result, greaterThan);
		case T_OP_LESS_OR_EQUAL:
			return evaluateBinary(patch, expression, result, lessOrEqual);
		case T_OP_LESS_THAN:
			return evaluateBinary(patch, expression, result, lessThan);
		case T_OP_EQUAL:
			return evaluateBinary(patch, expression, result, equals);
		case T_OP_NOT_EQUAL:
			return evaluateBinary(patch, expression, result, notEquals);
		case T_OP_FDIV:
			return evaluateBinary(patch, expression, result, fdiv);
		case T_OP_FMUL:
			return evaluateBinary(patch, expression, result, fmul);
		case T_FUNC_ATAN2:
			return evaluateBinary(patch, expression, result, fatan2);
		case T_FUNC_SIN:
			return evaluateUnary(patch, expression, result, fsin);
		case T_FUNC_COS:
			return evaluateUnary(patch, expression, result, fcos);
		case T_FUNC_TAN:
			return evaluateUnary(patch, expression, result, ftan);
		case T_FUNC_ASIN:
			return evaluateUnary(patch, expression, result, fasin);
		case T_FUNC_ACOS:
			return evaluateUnary(patch, expression, result, facos);
		case T_FUNC_ATAN:
			return evaluateUnary(patch, expression, result, fatan);
		case T_OP_BIT:
			return evaluateBit(patch, expression, result);
		case T_FUNC_LOWLIMIT:
			return evaluateLowLimit(patch, expression, result);
		case T_FUNC_HIGHLIMIT:
			return evaluateHighLimit(patch, expression, result);
		case T_FUNC_ASSERT:
			return evaluateAssert(patch, expression, result);

		case T_FUNC_BANK: {
			if (!xasm_Configuration->supportBanks)
				internalerror("Banks not supported");

			SSymbol* symbol = expression->value.symbol;
			if (symbol->section != NULL && (symbol->section->flags & SECTF_BANKFIXED)) {
				*result = symbol->section->bank;
				return true;
			}

			return false;
		}

		default:
			internalerror("Unknown operator");
	}
}

static bool
evaluateExpression(const SPatch* patch, SExpression* expression, int32_t* result) {
	if (expression == NULL)
		return false;

	if (expr_IsConstant(expression)) {
		expr_Free(expression->left);
		expression->left = NULL;
		expr_Free(expression->right);
		expression->right = NULL;
		expression->type = EXPR_INTEGER_CONSTANT;
		*result = expression->value.integer;
		return true;
	}

	switch (expr_Type(expression)) {
		case EXPR_PARENS:
			return evaluateExpression(patch, expression->right, result);
		case EXPR_PC_RELATIVE:
			return evaluatePcRelative(patch, expression, result);
		case EXPR_OPERATION:
			return evaluateOperation(patch, expression, result);
		case EXPR_INTEGER_CONSTANT:
			*result = expression->value.integer;
			return true;
		case EXPR_SYMBOL:
			if (expression->value.symbol->flags & SYMF_CONSTANT) {
				*result = expression->value.symbol->value.integer;
				expression->type = EXPR_INTEGER_CONSTANT;
				expression->isConstant = true;
				return true;
			} else if (expression->value.symbol->type == SYM_UNDEFINED) {
				err_PatchError(patch, ERROR_SYMBOL_UNDEFINED, str_String(expression->value.symbol->name));
			}
			return false;
		default:
			internalerror("Unknown expression");
	}
}

static void
patch_8(const SSection* section, const SPatch* patch, int32_t value) {
	if (value >= -128 && value <= 255) {
		section->data[patch->offset] = (uint8_t) value;
	} else {
		err_PatchError(patch, ERROR_EXPRESSION_N_BIT, 8);
	}
}

static void
patch_le_16(const SSection* section, const SPatch* patch, int32_t value) {
	if (value >= -32768 && value <= 65535) {
		section->data[patch->offset] = (uint8_t) value;
		section->data[patch->offset + 1] = (uint8_t) ((uint32_t) value >> 8u);
	} else {
		err_PatchError(patch, ERROR_EXPRESSION_N_BIT, 16);
	}
}

static void
patch_be_16(const SSection* section, const SPatch* patch, int32_t value) {
	if (value >= -32768 && value <= 65535) {
		section->data[patch->offset] = (uint8_t) ((uint32_t) value >> 8u);
		section->data[patch->offset + 1] = (uint8_t) value;
	} else {
		err_PatchError(patch, ERROR_EXPRESSION_N_BIT, 16);
	}
}

static void
patch_le_32(const SSection* section, const SPatch* patch, int32_t value) {
	section->data[patch->offset] = (uint8_t) value;
	section->data[patch->offset + 1] = (uint8_t) ((uint32_t) value >> 8u);
	section->data[patch->offset + 2] = (uint8_t) ((uint32_t) value >> 16u);
	section->data[patch->offset + 3] = (uint8_t) ((uint32_t) value >> 24u);
}

static void
patch_be_32(const SSection* section, const SPatch* patch, int32_t value) {
	section->data[patch->offset] = (uint8_t) ((uint32_t) value >> 24u);
	section->data[patch->offset + 1] = (uint8_t) ((uint32_t) value >> 16u);
	section->data[patch->offset + 2] = (uint8_t) ((uint32_t) value >> 8u);
	section->data[patch->offset + 3] = (uint8_t) value;
}

typedef void (*applyPatch)(const SSection*, const SPatch*, int32_t);

static applyPatch g_patchFunctions[PATCH_BE_32 - PATCH_8 + 1] = {
    patch_8, patch_le_16, patch_be_16, patch_le_32, patch_be_32,
};

/* Public functions */

void
patch_Create(SSection* section, uint32_t offset, SExpression* expression, EPatchType type) {
	SPatch* patch = mem_Alloc(sizeof(SPatch));
	memset(patch, 0, sizeof(SPatch));

	if (section->patches) {
		list_InsertAfter(section->patches, patch);
	} else {
		section->patches = patch;
	}

	patch->section = section;
	patch->offset = offset;
	patch->type = type;
	patch->expression = expression;
	str_Assign(&patch->filename, lex_Context->buffer.name);
	patch->lineNumber = lex_Context->lineNumber;
}

void
patch_Free(SPatch* patch) {
	str_Free(patch->filename);
	expr_Free(patch->expression);
	mem_Free(patch);
}

void
patch_BackPatch(void) {
	for (SSection* section = sect_Sections; section != NULL; section = list_GetNext(section)) {
		SPatch* patch = section->patches;
		while (patch != NULL) {
			SPatch* next = list_GetNext(patch);
			int32_t value;

			if (evaluateExpression(patch, patch->expression, &value)) {
				list_Remove(section->patches, patch);
				g_patchFunctions[patch->type](section, patch, value);
				patch_Free(patch);
			}
			patch = next;
		}
	}
}

void
patch_OptimizeAll(void) {
	for (SSection* section = sect_Sections; section != NULL; section = list_GetNext(section)) {
		for (SPatch* patch = section->patches; patch != NULL; patch = list_GetNext(patch)) {
			expr_Optimize(patch->expression);
		}
	}
}
