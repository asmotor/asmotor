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

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#include "asmotor.h"
#include "fmath.h"
#include "mem.h"

#include "xasm.h"
#include "filestack.h"
#include "parse.h"
#include "patch.h"
#include "project.h"
#include "tokens.h"

/* Private functions */

typedef int32_t (* binaryPredicate)(int32_t nLeft, int32_t nRight);
typedef int32_t (* unaryPredicate)(int32_t nValue);

static bool
evaluatePatch(SPatch* patch, SExpression* expression, int32_t* result);

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
reduceBinary(SPatch* pPatch, SExpression* pExpr, int32_t* v, binaryPredicate pPred) {
    int32_t vl;
    int32_t vr;

    if (evaluatePatch(pPatch, pExpr->left, &vl) && evaluatePatch(pPatch, pExpr->right, &vr)) {
        expr_Clear(pExpr);
        expr_SetConst(pExpr, *v = pPred(vl, vr));
        return true;
    }
    return false;
}

static bool
reduceUnary(SPatch* pPatch, SExpression* pExpr, int32_t* v, unaryPredicate pPred) {
    int32_t vr;

    if (evaluatePatch(pPatch, pExpr->right, &vr)) {
        expr_Free(pExpr->right);
        pExpr->right = NULL;

        pExpr->type = EXPR_CONSTANT;
        pExpr->isConstant = true;

        *v = pExpr->value.integer = pPred(vr);
        return true;
    }

    return false;
}

static bool
evaluatePcRelative(SPatch* patch, SExpression* expression, int32_t* result) {
    uint32_t offset;
    if (expr_GetSectionOffset(expression->right, patch->pSection, &offset)) {
        int32_t adjustment;
        if (evaluatePatch(patch, expression->left, &adjustment)) {
            expr_Clear(expression);
            expr_SetConst(expression, *result = offset + adjustment - patch->Offset);
            return true;
        }
    }
    return false;
}

static bool
patch_EvaluateOperator(SPatch* patch, SExpression* expression, int32_t* result) {
    switch (expression->operation) {
        case T_OP_SUBTRACT: {
            uint32_t l, r;

            SSection* pLeftSect = expr_GetSectionAndOffset(expression->left, &l);
            SSection* pRightSect = expr_GetSectionAndOffset(expression->right, &r);

            if (pLeftSect && pRightSect && pLeftSect == pRightSect) {
                expr_Clear(expression);
                expr_SetConst(expression, *result = l - r);
                return true;
            }

            return reduceBinary(patch, expression, result, subtract);
        }
        case T_OP_ADD:
            return reduceBinary(patch, expression, result, add);
        case T_OP_BITWISE_XOR:
            return reduceBinary(patch, expression, result, bitwiseXor);
        case T_OP_BITWISE_OR:
            return reduceBinary(patch, expression, result, bitwiseOr);
        case T_OP_BITWISE_AND:
            return reduceBinary(patch, expression, result, bitwiseAnd);
        case T_OP_BITWISE_ASL:
            return reduceBinary(patch, expression, result, arithmeticLeftShift);
        case T_OP_BITWISE_ASR:
            return reduceBinary(patch, expression, result, arithmeticRightShift);
        case T_OP_MULTIPLY:
            return reduceBinary(patch, expression, result, multiply);
        case T_OP_DIVIDE:
            return reduceBinary(patch, expression, result, divide);
        case T_OP_MODULO:
            return reduceBinary(patch, expression, result, modulo);
        case T_OP_BOOLEAN_OR:
            return reduceBinary(patch, expression, result, booleanOr);
        case T_OP_BOOLEAN_AND:
            return reduceBinary(patch, expression, result, booleanAnd);
        case T_OP_BOOLEAN_NOT:
            return reduceUnary(patch, expression, result, booleanNot);
        case T_OP_GREATER_OR_EQUAL:
            return reduceBinary(patch, expression, result, greaterOrEqual);
        case T_OP_GREATER_THAN:
            return reduceBinary(patch, expression, result, greaterThan);
        case T_OP_LESS_OR_EQUAL:
            return reduceBinary(patch, expression, result, lessOrEqual);
        case T_OP_LESS_THAN:
            return reduceBinary(patch, expression, result, lessThan);
        case T_OP_EQUAL:
            return reduceBinary(patch, expression, result, equals);
        case T_OP_NOT_EQUAL:
            return reduceBinary(patch, expression, result, notEquals);
        case T_FUNC_FDIV:
            return reduceBinary(patch, expression, result, fdiv);
        case T_FUNC_FMUL:
            return reduceBinary(patch, expression, result, fmul);
        case T_FUNC_ATAN2:
            return reduceBinary(patch, expression, result, fatan2);
        case T_FUNC_SIN:
            return reduceUnary(patch, expression, result, fsin);
        case T_FUNC_COS:
            return reduceUnary(patch, expression, result, fcos);
        case T_FUNC_TAN:
            return reduceUnary(patch, expression, result, ftan);
        case T_FUNC_ASIN:
            return reduceUnary(patch, expression, result, fasin);
        case T_FUNC_ACOS:
            return reduceUnary(patch, expression, result, facos);
        case T_FUNC_ATAN:
            return reduceUnary(patch, expression, result, fatan);

        case T_OP_BIT: {
            int32_t value;
            if (!evaluatePatch(patch, expression->right, &value))
                return false;

            if (isPowerOfTwo(value)) {
                int32_t bit = log2n((size_t) value);

                expr_Free(expression->right);
                expression->right = NULL;

                expression->type = EXPR_CONSTANT;
                expression->isConstant = true;

                expression->value.integer = *result = bit;
                return true;
            }

            prj_PatchError(patch, ERROR_EXPR_TWO_POWER);
            break;
        }

        case T_FUNC_LOWLIMIT: {
            int32_t lhs, rhs;

            if (evaluatePatch(patch, expression->right, &rhs) && evaluatePatch(patch, expression->left, &lhs)) {
                if (lhs >= rhs) {
                    expr_Clear(expression);
                    expr_SetConst(expression, *result = lhs);

                    return true;
                }
                prj_PatchFail(patch, ERROR_OPERAND_RANGE);
            }
            return false;
        }

        case T_FUNC_HIGHLIMIT: {
            int32_t lhs, rhs;

            if (evaluatePatch(patch, expression->right, &rhs) && evaluatePatch(patch, expression->left, &lhs)) {
                if (lhs <= rhs) {
                    expr_Clear(expression);
                    expr_SetConst(expression, *result = lhs);
                    return true;
                }
                prj_PatchFail(patch, ERROR_OPERAND_RANGE);
            }
            return false;
        }

        case T_FUNC_BANK: {
            if (!g_pConfiguration->bSupportBanks)
                internalerror("Banks not supported");

            return false;
        }

        default:
            internalerror("Unknown operator");
    }
    return false;
}

static bool
evaluatePatch(SPatch* patch, SExpression* expression, int32_t* result) {
    if (expression == NULL)
        return false;

    if (expr_IsConstant(expression)) {
        expr_Free(expression->left);
        expression->left = NULL;

        expr_Free(expression->right);
        expression->right = NULL;

        expression->type = EXPR_CONSTANT;
        *result = expression->value.integer;

        return true;
    }

    switch (expr_Type(expression)) {
        case EXPR_PARENS:
            return evaluatePatch(patch, expression->right, result);
        case EXPR_PC_RELATIVE:
            return evaluatePcRelative(patch, expression, result);
        case EXPR_OPERATION:
            return patch_EvaluateOperator(patch, expression, result);
        case EXPR_CONSTANT:
            *result = expression->value.integer;
            return true;
        case EXPR_SYMBOL:
            if (expression->value.symbol->nFlags & SYMF_CONSTANT) {
                *result = expression->value.symbol->Value.Value;
                return true;
            } else if (expression->value.symbol->eType == SYM_UNDEFINED) {
                prj_PatchError(patch, ERROR_SYMBOL_UNDEFINED, str_String(expression->value.symbol->pName));
            }
            return false;
        default:
            internalerror("Unknown expression");
    }
}


/* Public functions */

bool
patch_GetImportOffset(uint32_t* pOffset, SSymbol** ppSym, SExpression* pExpr) {
    if (pExpr == NULL)
        return false;

    if (expr_Type(pExpr) == EXPR_SYMBOL) {
        SSymbol* pSym = pExpr->value.symbol;
        if (pSym->eType == SYM_IMPORT || pSym->eType == SYM_GLOBAL) {
            if (*ppSym != NULL)
                return false;

            *ppSym = pSym;
            *pOffset = 0;
            return true;
        }
        return false;
    } else if (expr_IsOperator(pExpr, T_OP_ADD) || expr_IsOperator(pExpr, T_OP_SUBTRACT)) {
        uint32_t offset;
        if (patch_GetImportOffset(&offset, ppSym, pExpr->left)) {
            if (expr_IsConstant(pExpr->right)) {
                if (expr_IsOperator(pExpr, T_OP_ADD))
                    *pOffset = offset + pExpr->right->value.integer;
                else
                    *pOffset = offset - pExpr->right->value.integer;
                return true;
            }
        }
        if (patch_GetImportOffset(&offset, ppSym, pExpr->right)) {
            if (expr_IsConstant(pExpr->left)) {
                if (expr_IsOperator(pExpr, T_OP_ADD))
                    *pOffset = pExpr->left->value.integer + offset;
                else
                    *pOffset = pExpr->left->value.integer - offset;
                return true;
            }
        }
    }
    return false;
}

void
patch_Create(SSection* sect, uint32_t offset, SExpression* expr, EPatchType type) {
    SPatch* patch = mem_Alloc(sizeof(SPatch));
    memset(patch, 0, sizeof(SPatch));

    if (sect->pPatches) {
        list_InsertAfter(sect->pPatches, patch);
    } else {
        sect->pPatches = patch;
    }

    patch->pSection = sect;
    patch->Offset = offset;
    patch->Type = type;
    patch->pExpression = expr;
    patch->pFile = str_Copy(fstk_Current->name);
    patch->nLine = fstk_Current->lineNumber;
}

void
patch_BackPatch(void) {
    for (SSection* sect = g_pSectionList; sect != NULL; sect = list_GetNext(sect)) {
        for (SPatch* patch = sect->pPatches; patch != NULL; patch = list_GetNext(patch)) {
            int32_t v;

            if (evaluatePatch(patch, patch->pExpression, &v)) {
                list_Remove(sect->pPatches, patch);
                str_Free(patch->pFile);

                switch (patch->Type) {
                    case PATCH_BYTE:
                        if (v >= -128 && v <= 255)
                            sect->pData[patch->Offset] = (uint8_t) v;
                        else
                            prj_PatchError(patch, ERROR_EXPRESSION_N_BIT, 8);
                        break;

                    case PATCH_LWORD:
                        if (v >= -32768 && v <= 65535) {
                            sect->pData[patch->Offset] = (uint8_t) v;
                            sect->pData[patch->Offset + 1] = (uint8_t) ((uint32_t) v >> 8u);
                        } else
                            prj_PatchError(patch, ERROR_EXPRESSION_N_BIT, 16);
                        break;

                    case PATCH_BWORD:
                        if (v >= -32768 && v <= 65535) {
                            sect->pData[patch->Offset] = (uint8_t) ((uint32_t) v >> 8u);
                            sect->pData[patch->Offset + 1] = (uint8_t) v;
                        } else
                            prj_PatchError(patch, ERROR_EXPRESSION_N_BIT, 16);
                        break;

                    case PATCH_LLONG:
                        sect->pData[patch->Offset] = (uint8_t) v;
                        sect->pData[patch->Offset + 1] = (uint8_t) ((uint32_t) v >> 8u);
                        sect->pData[patch->Offset + 2] = (uint8_t) ((uint32_t) v >> 16u);
                        sect->pData[patch->Offset + 3] = (uint8_t) ((uint32_t) v >> 24u);
                        break;

                    case PATCH_BLONG:
                        sect->pData[patch->Offset] = (uint8_t) ((uint32_t) v >> 24u);
                        sect->pData[patch->Offset + 1] = (uint8_t) ((uint32_t) v >> 16u);
                        sect->pData[patch->Offset + 2] = (uint8_t) ((uint32_t) v >> 8u);
                        sect->pData[patch->Offset + 3] = (uint8_t) v;
                        break;

                    default:
                        internalerror("Unknown patch type");
                }
            }
        }
    }
}

void
patch_OptimizeAll(void) {
    SSection* sect;

    sect = g_pSectionList;
    while (sect) {
        SPatch* patch;

        patch = sect->pPatches;
        while (patch) {
            expr_Optimize(patch->pExpression);
            patch = list_GetNext(patch);
        }

        sect = list_GetNext(sect);
    }
}
