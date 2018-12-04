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

#include <fmath.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#include "asmotor.h"
#include "xasm.h"
#include "mem.h"
#include "patch.h"
#include "tokens.h"
#include "parse.h"
#include "project.h"
#include "filestack.h"


typedef int32_t (*pPredicate_t)(int32_t nLeft, int32_t nRight);
typedef int32_t (*pPredicate1_t)(int32_t nValue);

static bool exactlyOneBitSet(int32_t d) {
    return ((uint32_t) d & (uint32_t) -d) == (uint32_t) d && d != 0;
}

static bool patch_Evaluate(SPatch* patch, SExpression* expr, int32_t* v);

static int32_t patch_Subtract(int32_t nLeft, int32_t nRight)
{
    return nLeft - nRight;
}

static int32_t patch_Add(int32_t nLeft, int32_t nRight)
{
    return nLeft + nRight;
}

static int32_t patch_BitwiseXor(int32_t nLeft, int32_t nRight)
{
    return (uint32_t) nLeft ^ (uint32_t) nRight;
}

static int32_t patch_BitwiseOr(int32_t nLeft, int32_t nRight)
{
    return (uint32_t) nLeft | (uint32_t) nRight;
}

static int32_t patch_BitwiseAnd(int32_t nLeft, int32_t nRight)
{
    return (uint32_t) nLeft & (uint32_t) nRight;
}

static int32_t patch_BitwiseShiftLeft(int32_t nLeft, int32_t nRight)
{
    return (uint32_t) nLeft << (uint32_t) nRight;
}

static int32_t patch_BitwiseShiftRight(int32_t nLeft, int32_t nRight)
{
    return (uint32_t) nLeft >> (uint32_t) nRight;
}

static int32_t patch_Multiply(int32_t nLeft, int32_t nRight)
{
    return nLeft * nRight;
}

static int32_t patch_Divide(int32_t nLeft, int32_t nRight)
{
    return nLeft / nRight;
}

static int32_t patch_Modulo(int32_t nLeft, int32_t nRight)
{
    return nLeft % nRight;
}

static int32_t patch_BooleanAnd(int32_t nLeft, int32_t nRight)
{
    return nLeft && nRight;
}

static int32_t patch_BooleanOr(int32_t nLeft, int32_t nRight)
{
    return nLeft || nRight;
}

static int32_t patch_GreaterOrEqual(int32_t nLeft, int32_t nRight)
{
    return nLeft >= nRight;
}

static int32_t patch_GreaterThan(int32_t nLeft, int32_t nRight)
{
    return nLeft > nRight;
}

static int32_t patch_LessOrEqual(int32_t nLeft, int32_t nRight)
{
    return nLeft <= nRight;
}

static int32_t patch_LessThan(int32_t nLeft, int32_t nRight)
{
    return nLeft < nRight;
}

static int32_t patch_Equals(int32_t nLeft, int32_t nRight)
{
    return nLeft == nRight;
}

static int32_t patch_NotEquals(int32_t nLeft, int32_t nRight)
{
    return nLeft != nRight;
}

static int32_t patch_PredFDiv(int32_t nLeft, int32_t nRight)
{
    return fdiv(nLeft, nRight);
}

static int32_t patch_PredFMul(int32_t nLeft, int32_t nRight)
{
    return fmul(nLeft, nRight);
}

static int32_t patch_PredAtan2(int32_t nLeft, int32_t nRight)
{
    return fatan2(nLeft, nRight);
}

static int32_t patch_PredBooleanNot(int32_t nValue)
{
    return nValue ? 0 : 1;
}

static int32_t patch_PredNot(int32_t nValue)
{
    return ~ (uint32_t) nValue;
}

static int32_t patch_PredSin(int32_t nValue)
{
    return fsin(nValue);
}

static int32_t patch_PredCos(int32_t nValue)
{
    return fcos(nValue);
}

static int32_t patch_PredTan(int32_t nValue)
{
    return ftan(nValue);
}

static int32_t patch_PredAsin(int32_t nValue)
{
    return fasin(nValue);
}

static int32_t patch_PredAcos(int32_t nValue)
{
    return facos(nValue);
}

static int32_t patch_PredAtan(int32_t nValue)
{
    return fatan(nValue);
}


static bool patch_ReduceBinary(SPatch* pPatch, SExpression* pExpr, int32_t* v, pPredicate_t pPred)
{
    int32_t vl;
    int32_t vr;

    if(patch_Evaluate(pPatch, pExpr->pLeft, &vl)
    && patch_Evaluate(pPatch, pExpr->pRight, &vr))
    {
        expr_Clear(pExpr);
        expr_SetConst(pExpr, *v = pPred(vl, vr));
        return true;
    }
    return false;
}


static bool patch_ReduceUnary(SPatch* pPatch, SExpression* pExpr, int32_t* v, pPredicate1_t pPred)
{
    int32_t vr;

    if(patch_Evaluate(pPatch, pExpr->pRight, &vr))
    {
        expr_Free(pExpr->pRight);
        pExpr->pRight = NULL;

        pExpr->eType = EXPR_CONSTANT;
        pExpr->nFlags |= EXPRF_CONSTANT;

        *v = pExpr->Value.Value = pPred(vr);
        return true;
    }

    return false;
}


bool patch_GetImportOffset(uint32_t* pOffset, SSymbol** ppSym, SExpression* pExpr)
{
    if(pExpr == NULL)
        return false;

    if(expr_GetType(pExpr) == EXPR_SYMBOL)
    {
        SSymbol* pSym = pExpr->Value.pSymbol;
        if(pSym->eType == SYM_IMPORT || pSym->eType == SYM_GLOBAL)
        {
            if(*ppSym != NULL)
                return false;

            *ppSym = pSym;
            *pOffset = 0;
            return true;
        }
        return false;
    }
    else if(expr_IsOperator(pExpr, T_OP_ADD) || expr_IsOperator(pExpr, T_OP_SUB))
    {
        uint32_t offset;
        if(patch_GetImportOffset(&offset, ppSym, pExpr->pLeft))
        {
            if(expr_IsConstant(pExpr->pRight))
            {
                if(expr_IsOperator(pExpr, T_OP_ADD))
                    *pOffset = offset + pExpr->pRight->Value.Value;
                else
                    *pOffset = offset - pExpr->pRight->Value.Value;
                return true;
            }
        }
        if(patch_GetImportOffset(&offset, ppSym, pExpr->pRight))
        {
            if(expr_IsConstant(pExpr->pLeft))
            {
                if(expr_IsOperator(pExpr, T_OP_ADD))
                    *pOffset = pExpr->pLeft->Value.Value + offset;
                else
                    *pOffset = pExpr->pLeft->Value.Value - offset;
                return true;
            }
        }
    }
    return false;
}

bool patch_IsRelativeToSection(SExpression* pExpr, SSection* pSection)
{
    uint32_t nOffset;
    return patch_GetSectionPcOffset(&nOffset, pExpr, pSection);
}

bool patch_GetSectionPcOffset(uint32_t* pOffset, SExpression* pExpr, SSection* pSection)
{
    if(pExpr == NULL)
        return false;

    if(expr_GetType(pExpr) == EXPR_PARENS)
    {
        return patch_GetSectionPcOffset(pOffset, pExpr->pRight, pSection);
    }

    if(expr_GetType(pExpr) == EXPR_CONSTANT && (pSection->Flags & SECTF_LOADFIXED))
    {
        *pOffset = pExpr->Value.Value - pSection->BasePC;
        return true;
    }
    
    if(expr_GetType(pExpr) == EXPR_SYMBOL)
    {
        SSymbol* pSym = pExpr->Value.pSymbol;
        if((pSym->eType == SYM_EQU) && (pSection->Flags & SECTF_LOADFIXED))
        {
            *pOffset = pSym->Value.Value - pSection->BasePC;
            return true;
        }
        else if(pSym->pSection == pSection)
        {
            if((pSym->nFlags & SYMF_CONSTANT) && (pSection->Flags & SECTF_LOADFIXED))
            {
                *pOffset = pSym->Value.Value - pSection->BasePC;
                return true;
            }
            else if((pSym->nFlags & SYMF_RELOC) && (pSection->Flags == 0))
            {
                *pOffset = pSym->Value.Value;
                return true;
            }
        }
        return false;
    }
    
    if(expr_IsOperator(pExpr, T_OP_ADD) || expr_IsOperator(pExpr, T_OP_SUB))
    {
        uint32_t offset;
        if(patch_GetSectionPcOffset(&offset, pExpr->pLeft, pSection))
        {
            if(expr_IsConstant(pExpr->pRight))
            {
                if(expr_IsOperator(pExpr, T_OP_ADD))
                    *pOffset = offset + pExpr->pRight->Value.Value;
                else
                    *pOffset = offset - pExpr->pRight->Value.Value;
                return true;
            }
        }
    }

    if(expr_IsOperator(pExpr, T_OP_ADD))
    {
        uint32_t offset;
        if(patch_GetSectionPcOffset(&offset, pExpr->pRight, pSection))
        {
            if(expr_IsConstant(pExpr->pLeft))
            {
                *pOffset = pExpr->pLeft->Value.Value + offset;
                return true;
            }
        }
    }

    return false;
}


SSection* patch_GetExpressionSectionAndPcOffset(SExpression* pExpr, uint32_t* pOffset)
{
    SSection* pSection = g_pSectionList;

    while(pSection)
    {
        if(patch_GetSectionPcOffset(pOffset, pExpr, pSection))
            return pSection;

        pSection = list_GetNext(pSection);
    }

    *pOffset = 0;
    return NULL;
}

static bool patch_EvaluatePcRel(SPatch* patch, SExpression* expr, int32_t* v)
{
    uint32_t offset;
    if(patch_GetSectionPcOffset(&offset, expr->pRight, patch->pSection))
    {
        int32_t v1;
        if(patch_Evaluate(patch, expr->pLeft, &v1))
        {
            expr_Clear(expr);
            expr_SetConst(expr, *v = offset + v1 - patch->Offset);
            return true;
        }
    }
    return false;
}

static bool patch_EvaluateOperator(SPatch* patch, SExpression* expr, int32_t* v)
{
    switch(expr->eOperator)
    {
        case T_OP_SUB:
        {
            uint32_t l, r;

            SSection* pLeftSect = patch_GetExpressionSectionAndPcOffset(expr->pLeft, &l);
            SSection* pRightSect = patch_GetExpressionSectionAndPcOffset(expr->pRight, &r);

            if(pLeftSect && pRightSect && pLeftSect == pRightSect)
            {
                expr_Clear(expr);
                expr_SetConst(expr, *v = l - r);
                return true;
            }

            return patch_ReduceBinary(patch, expr, v, patch_Subtract);
        }
        case T_OP_ADD:
            return patch_ReduceBinary(patch, expr, v, patch_Add);
        case T_OP_XOR:
            return patch_ReduceBinary(patch, expr, v, patch_BitwiseXor);
        case T_OP_OR:
            return patch_ReduceBinary(patch, expr, v, patch_BitwiseOr);
        case T_OP_AND:
            return patch_ReduceBinary(patch, expr, v, patch_BitwiseAnd);
        case T_OP_SHL:
            return patch_ReduceBinary(patch, expr, v, patch_BitwiseShiftLeft);
        case T_OP_SHR:
            return patch_ReduceBinary(patch, expr, v, patch_BitwiseShiftRight);
        case T_OP_MUL:
            return patch_ReduceBinary(patch, expr, v, patch_Multiply);
        case T_OP_DIV:
            return patch_ReduceBinary(patch, expr, v, patch_Divide);
        case T_OP_MOD:
            return patch_ReduceBinary(patch, expr, v, patch_Modulo);
        case T_OP_LOGICOR:
            return patch_ReduceBinary(patch, expr, v, patch_BooleanOr);
        case T_OP_LOGICAND:
            return patch_ReduceBinary(patch, expr, v, patch_BooleanAnd);
        case T_OP_LOGICNOT:
            return patch_ReduceUnary(patch, expr, v, patch_PredBooleanNot);
        case T_OP_NOT:
            return patch_ReduceUnary(patch, expr, v, patch_PredNot);
        case T_OP_LOGICGE:
            return patch_ReduceBinary(patch, expr, v, patch_GreaterOrEqual);
        case T_OP_LOGICGT:
            return patch_ReduceBinary(patch, expr, v, patch_GreaterThan);
        case T_OP_LOGICLE:
            return patch_ReduceBinary(patch, expr, v, patch_LessOrEqual);
        case T_OP_LOGICLT:
            return patch_ReduceBinary(patch, expr, v, patch_LessThan);
        case T_OP_LOGICEQU:
            return patch_ReduceBinary(patch, expr, v, patch_Equals);
        case T_OP_LOGICNE:
            return patch_ReduceBinary(patch, expr, v, patch_NotEquals);
        case T_FUNC_FDIV:
            return patch_ReduceBinary(patch, expr, v, patch_PredFDiv);
        case T_FUNC_FMUL:
            return patch_ReduceBinary(patch, expr, v, patch_PredFMul);
        case T_FUNC_ATAN2:
            return patch_ReduceBinary(patch, expr, v, patch_PredAtan2);
        case T_FUNC_SIN:
            return patch_ReduceUnary(patch, expr, v, patch_PredSin);
        case T_FUNC_COS:
            return patch_ReduceUnary(patch, expr, v, patch_PredCos);
        case T_FUNC_TAN:
            return patch_ReduceUnary(patch, expr, v, patch_PredTan);
        case T_FUNC_ASIN:
            return patch_ReduceUnary(patch, expr, v, patch_PredAsin);
        case T_FUNC_ACOS:
            return patch_ReduceUnary(patch, expr, v, patch_PredAcos);
        case T_FUNC_ATAN:
            return patch_ReduceUnary(patch, expr, v, patch_PredAtan);

        case T_OP_BIT:
        {
            int32_t	vr;
            if(!patch_Evaluate(patch,expr->pRight,&vr))
                return false;

            if(exactlyOneBitSet(vr))
            {
                uint32_t t = vr;
                int b = 0;
                while(t != 1)
                {
                    t >>= 1;
                    ++b;
                }

                expr_Free(expr->pRight);
                expr->pRight = NULL;

                expr->eType = EXPR_CONSTANT;
                expr->nFlags |= EXPRF_CONSTANT;

                expr->Value.Value = *v = b;
                return true;
            }

            prj_PatchError(patch, ERROR_EXPR_TWO_POWER);
            break;
        }

        case T_FUNC_LOWLIMIT:
        {
            int32_t	vl, vr;

            if(patch_Evaluate(patch, expr->pRight, &vr)
            && patch_Evaluate(patch, expr->pLeft, &vl))
            {
                if(vl >= vr)
                {
                    expr_Clear(expr);
                    expr_SetConst(expr, *v = vl);

                    return true;
                }
                prj_PatchFail(patch, ERROR_OPERAND_RANGE);
            }
            return false;
        }

        case T_FUNC_HIGHLIMIT:
        {
            int32_t	vl, vr;

            if(patch_Evaluate(patch, expr->pRight, &vr)
            && patch_Evaluate(patch, expr->pLeft, &vl))
            {
                if(vl <= vr)
                {
                    expr_Clear(expr);
                    expr_SetConst(expr, *v = vl);
                    return true;
                }
                prj_PatchFail(patch, ERROR_OPERAND_RANGE);
            }
            return false;
        }

        case T_FUNC_BANK:
        {
            if(!g_pConfiguration->bSupportBanks)
                internalerror("Banks not supported");

            return false;
        }
        
        default:
            internalerror("Unknown operator");
    }
    return false;
}

static bool patch_Evaluate(SPatch* patch, SExpression* expr, int32_t* v)
{
    if (expr == NULL)
        return false;

    if (expr_IsConstant(expr))
    {
        expr_Free(expr->pLeft);
        expr->pLeft = NULL;

        expr_Free(expr->pRight);
        expr->pRight = NULL;

        expr->eType = EXPR_CONSTANT;
        *v = expr->Value.Value;

        return true;
    }

    switch (expr_GetType(expr))
    {
        case EXPR_PARENS:
            return patch_Evaluate(patch, expr->pRight, v);
        case EXPR_PCREL:
            return patch_EvaluatePcRel(patch, expr, v);
        case EXPR_OPERATOR:
            return patch_EvaluateOperator(patch, expr, v);
        case EXPR_CONSTANT:
            *v = expr->Value.Value;
            return true;
        case EXPR_SYMBOL:
            if (expr->Value.pSymbol->nFlags & SYMF_CONSTANT)
            {
                *v = expr->Value.pSymbol->Value.Value;
                return true;
            }
            else if (expr->Value.pSymbol->eType == SYM_UNDEFINED)
            {
                prj_PatchError(patch, ERROR_SYMBOL_UNDEFINED, str_String(expr->Value.pSymbol->pName));
            }
            return false;
        default:
            internalerror("Unknown expression");
    }
}



//	Public routines

void patch_Create(SSection* sect, uint32_t offset, SExpression* expr, EPatchType type)
{
    SPatch* patch = mem_Alloc(sizeof(SPatch));
    memset(patch, 0, sizeof(SPatch));
    
    if(sect->pPatches)
    {
        list_InsertAfter(sect->pPatches, patch);
    }
    else
    {
        sect->pPatches = patch;
    }

    patch->pSection = sect;
    patch->Offset = offset;
    patch->Type = type;
    patch->pExpression = expr;
    patch->pFile = str_Copy(g_pFileContext->pName);
    patch->nLine = g_pFileContext->LineNumber;
}

void patch_BackPatch(void)
{
    for (SSection* sect = g_pSectionList; sect != NULL; sect = list_GetNext(sect))
    {
        for (SPatch* patch = sect->pPatches; patch != NULL; patch = list_GetNext(patch))
        {
            int32_t v;

            if (patch_Evaluate(patch, patch->pExpression, &v))
            {
                list_Remove(sect->pPatches, patch);
                str_Free(patch->pFile);

                switch(patch->Type)
                {
                    case PATCH_BYTE:
                        if(v >= -128 && v <= 255)
                            sect->pData[patch->Offset] = (uint8_t)v;
                        else
                            prj_PatchError(patch, ERROR_EXPRESSION_N_BIT, 8);
                        break;

                    case PATCH_LWORD:
                        if(v >= -32768 && v <= 65535)
                        {
                            sect->pData[patch->Offset] = (uint8_t) v;
                            sect->pData[patch->Offset + 1] = (uint8_t) ((uint32_t) v >> 8u);
                        }
                        else
                            prj_PatchError(patch, ERROR_EXPRESSION_N_BIT, 16);
                        break;

                    case PATCH_BWORD:
                        if(v >= -32768 && v <= 65535)
                        {
                            sect->pData[patch->Offset] = (uint8_t) ((uint32_t) v >> 8u);
                            sect->pData[patch->Offset + 1] = (uint8_t) v;
                        }
                        else
                            prj_PatchError(patch, ERROR_EXPRESSION_N_BIT, 16);
                        break;

                    case PATCH_LLONG:
                        sect->pData[patch->Offset] = (uint8_t) v;
                        sect->pData[patch->Offset + 1] = (uint8_t)((uint32_t) v >> 8u);
                        sect->pData[patch->Offset + 2] = (uint8_t)((uint32_t) v >> 16u);
                        sect->pData[patch->Offset + 3] = (uint8_t)((uint32_t) v >> 24u);
                        break;

                    case PATCH_BLONG:
                        sect->pData[patch->Offset] = (uint8_t)((uint32_t) v >> 24u);
                        sect->pData[patch->Offset + 1] = (uint8_t)((uint32_t) v >> 16u);
                        sect->pData[patch->Offset + 2] = (uint8_t)((uint32_t) v >> 8u);
                        sect->pData[patch->Offset + 3] = (uint8_t) v;
                        break;

                    default:
                        internalerror("Unknown patch type");
                }
            }
        }
    }
}

void patch_OptimizeExpression(SExpression* pExpr)
{
    if(pExpr->pLeft != NULL)
        patch_OptimizeExpression(pExpr->pLeft);

    if(pExpr->pRight != NULL)
        patch_OptimizeExpression(pExpr->pRight);

    if(expr_GetType(pExpr) == EXPR_PARENS)
    {
        SExpression* pToFree = pExpr->pRight;
        *pExpr = *(pExpr->pRight);
        mem_Free(pToFree);
    }

    if((pExpr->eType == EXPR_SYMBOL) && (pExpr->Value.pSymbol->nFlags & SYMF_CONSTANT))
    {
        pExpr->eType = EXPR_CONSTANT;
        pExpr->nFlags = EXPRF_CONSTANT | EXPRF_RELOC;
        pExpr->Value.Value = pExpr->Value.pSymbol->Value.Value;
    }

    if(expr_IsConstant(pExpr))
    {
        expr_Free(pExpr->pLeft);
        pExpr->pLeft = NULL;
        expr_Free(pExpr->pRight);
        pExpr->pRight = NULL;

        pExpr->eType = EXPR_CONSTANT;
        pExpr->eOperator = T_NONE;
    }
}

void patch_OptimizeAll(void)
{
    SSection* sect;

    sect = g_pSectionList;
    while(sect)
    {
        SPatch* patch;

        patch = sect->pPatches;
        while(patch)
        {
            patch_OptimizeExpression(patch->pExpression);
            patch = list_GetNext(patch);
        }

        sect=list_GetNext(sect);
    }
}
