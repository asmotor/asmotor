/*  Copyright 2008 Carsten Sørensen

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

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>

#include "asmotor.h"
#include "xasm.h"
#include "mem.h"
#include "patch.h"
#include "tokens.h"
#include "parse.h"
#include "project.h"
#include "fstack.h"


typedef int32_t (*pPredicate_t)(int32_t nLeft, int32_t nRight);
typedef int32_t (*pPredicate1_t)(int32_t nValue);

static bool_t patch_Evaluate(SPatch* patch, SExpression* expr, int32_t* v);


static int32_t patch_PredSub(int32_t nLeft, int32_t nRight)
{
	return nLeft - nRight;
}

static int32_t patch_PredAdd(int32_t nLeft, int32_t nRight)
{
	return nLeft + nRight;
}

static int32_t patch_PredXor(int32_t nLeft, int32_t nRight)
{
	return nLeft ^ nRight;
}

static int32_t patch_PredOr(int32_t nLeft, int32_t nRight)
{
	return nLeft | nRight;
}

static int32_t patch_PredAnd(int32_t nLeft, int32_t nRight)
{
	return nLeft & nRight;
}

static int32_t patch_PredShl(int32_t nLeft, int32_t nRight)
{
	return nLeft << nRight;
}

static int32_t patch_PredShr(int32_t nLeft, int32_t nRight)
{
	return nLeft >> nRight;
}

static int32_t patch_PredMul(int32_t nLeft, int32_t nRight)
{
	return nLeft * nRight;
}

static int32_t patch_PredDiv(int32_t nLeft, int32_t nRight)
{
	return nLeft / nRight;
}

static int32_t patch_PredMod(int32_t nLeft, int32_t nRight)
{
	return nLeft % nRight;
}

static int32_t patch_PredBooleanAnd(int32_t nLeft, int32_t nRight)
{
	return nLeft && nRight;
}

static int32_t patch_PredBooleanOr(int32_t nLeft, int32_t nRight)
{
	return nLeft || nRight;
}

static int32_t patch_PredGe(int32_t nLeft, int32_t nRight)
{
	return nLeft >= nRight;
}

static int32_t patch_PredGt(int32_t nLeft, int32_t nRight)
{
	return nLeft > nRight;
}

static int32_t patch_PredLe(int32_t nLeft, int32_t nRight)
{
	return nLeft <= nRight;
}

static int32_t patch_PredLt(int32_t nLeft, int32_t nRight)
{
	return nLeft < nRight;
}

static int32_t patch_PredEquals(int32_t nLeft, int32_t nRight)
{
	return nLeft == nRight;
}

static int32_t patch_PredNotEquals(int32_t nLeft, int32_t nRight)
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
	return ~nValue;
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


static bool_t patch_ReduceBinary(SPatch* pPatch, SExpression* pExpr, int32_t* v, pPredicate_t pPred)
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


static bool_t patch_ReduceUnary(SPatch* pPatch, SExpression* pExpr, int32_t* v, pPredicate1_t pPred)
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


bool_t patch_GetImportOffset(uint32_t* pOffset, SSymbol** ppSym, SExpression* pExpr)
{
	if(pExpr == NULL)
		return false;

	if(expr_GetType(pExpr) == EXPR_SYMBOL)
	{
		SSymbol* pSym = pExpr->Value.pSymbol;
		if(pSym->Type == SYM_IMPORT || pSym->Type == SYM_GLOBAL)
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


bool_t patch_GetSectionOffset(uint32_t* pOffset, SExpression* pExpr, SSection* pSection)
{
	if(pExpr == NULL)
		return false;

	if(expr_GetType(pExpr) == EXPR_CONSTANT && (pSection->Flags & SECTF_ORGFIXED))
	{
		*pOffset = pExpr->Value.Value - pSection->Org;
		return true;
	}
	
	if(expr_GetType(pExpr) == EXPR_SYMBOL)
	{
		SSymbol* pSym = pExpr->Value.pSymbol;
		if(pSym->pSection == pSection)
		{
			if((pSym->Flags & SYMF_CONSTANT) && (pSection->Flags & SECTF_ORGFIXED))
			{
				*pOffset = pSym->Value.Value - pSection->Org;
				return true;
			}
			else if((pSym->Flags & SYMF_RELOC) && (pSection->Flags == 0))
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
		if(patch_GetSectionOffset(&offset, pExpr->pLeft, pSection))
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

		if(patch_GetSectionOffset(&offset, pExpr->pRight, pSection))
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


SSection* patch_GetExpressionSectionAndOffset(SPatch* pPatch, SExpression* pExpr, uint32_t* pOffset)
{
	SSection* pSection = pSectionList;

	while(pSection)
	{
		if(patch_GetSectionOffset(pOffset, pExpr, pSection))
			return pSection;

		pSection = list_GetNext(pSection);
	}

	*pOffset = 0;
	return NULL;
}

static bool_t patch_EvaluatePcRel(SPatch* patch, SExpression* expr, int32_t* v)
{
	uint32_t offset;
	if(patch_GetSectionOffset(&offset, expr->pRight, patch->pSection))
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

static bool_t patch_EvaluateOperator(SPatch* patch, SExpression* expr, int32_t* v)
{
	switch(expr->eOperator)
	{
		case T_OP_SUB:
		{
			uint32_t l, r;

			SSection* pLeftSect = patch_GetExpressionSectionAndOffset(patch, expr->pLeft, &l);
			SSection* pRightSect = patch_GetExpressionSectionAndOffset(patch, expr->pRight, &r);

			if(pLeftSect && pRightSect && pLeftSect == pRightSect)
			{
				expr_Clear(expr);
				expr_SetConst(expr, *v = l - r);
				return true;
			}

			return patch_ReduceBinary(patch, expr, v, patch_PredSub);
		}
		case T_OP_ADD:
			return patch_ReduceBinary(patch, expr, v, patch_PredAdd);
		case T_OP_XOR:
			return patch_ReduceBinary(patch, expr, v, patch_PredXor);
		case T_OP_OR:
			return patch_ReduceBinary(patch, expr, v, patch_PredOr);
		case T_OP_AND:
			return patch_ReduceBinary(patch, expr, v, patch_PredAnd);
		case T_OP_SHL:
			return patch_ReduceBinary(patch, expr, v, patch_PredShl);
		case T_OP_SHR:
			return patch_ReduceBinary(patch, expr, v, patch_PredShr);
		case T_OP_MUL:
			return patch_ReduceBinary(patch, expr, v, patch_PredMul);
		case T_OP_DIV:
			return patch_ReduceBinary(patch, expr, v, patch_PredDiv);
		case T_OP_MOD:
			return patch_ReduceBinary(patch, expr, v, patch_PredMod);
		case T_OP_LOGICOR:
			return patch_ReduceBinary(patch, expr, v, patch_PredBooleanOr);
		case T_OP_LOGICAND:
			return patch_ReduceBinary(patch, expr, v, patch_PredBooleanAnd);
		case T_OP_LOGICNOT:
			return patch_ReduceUnary(patch, expr, v, patch_PredBooleanNot);
		case T_OP_NOT:
			return patch_ReduceUnary(patch, expr, v, patch_PredNot);
		case T_OP_LOGICGE:
			return patch_ReduceBinary(patch, expr, v, patch_PredGe);
		case T_OP_LOGICGT:
			return patch_ReduceBinary(patch, expr, v, patch_PredGt);
		case T_OP_LOGICLE:
			return patch_ReduceBinary(patch, expr, v, patch_PredLe);
		case T_OP_LOGICLT:
			return patch_ReduceBinary(patch, expr, v, patch_PredLt);
		case T_OP_LOGICEQU:
			return patch_ReduceBinary(patch, expr, v, patch_PredEquals);
		case T_OP_LOGICNE:
			return patch_ReduceBinary(patch, expr, v, patch_PredNotEquals);
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

			if((vr & -vr) == vr && vr != 0)
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

			prj_Error(ERROR_EXPR_TWO_POWER);
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
				prj_Fail(ERROR_OPERAND_RANGE);
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
				prj_Fail(ERROR_OPERAND_RANGE);
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
			break;
	}
	return false;
}

static bool_t patch_Evaluate(SPatch* patch, SExpression* expr, int32_t* v)
{
	if(expr == NULL)
		return false;

	if(expr_IsConstant(expr))
	{
		expr_Free(expr->pLeft);
		expr->pLeft = NULL;

		expr_Free(expr->pRight);
		expr->pRight = NULL;

		expr->eType = EXPR_CONSTANT;
		*v = expr->Value.Value;

		return true;
	}

	switch(expr_GetType(expr))
	{
		case EXPR_PCREL:
			return patch_EvaluatePcRel(patch, expr, v);
		case EXPR_OPERATOR:
			return patch_EvaluateOperator(patch, expr, v);
		case EXPR_CONSTANT:
			*v = expr->Value.Value;
			return true;
		case EXPR_SYMBOL:
			if(expr->Value.pSymbol->Flags & SYMF_CONSTANT)
			{
				*v = expr->Value.pSymbol->Value.Value;
				return true;
			}
			if(expr->Value.pSymbol->Type == SYM_UNDEFINED)
				prj_Error(ERROR_SYMBOL_UNDEFINED, expr->Value.pSymbol->Name);
			return false;
		default:
			internalerror("Unknown expression");
	}
	return false;
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
	patch->pszFile = _strdup(g_pFileContext->pName);
	patch->nLine = g_pFileContext->LineNumber;
}

void patch_BackPatch(void)
{
	SSection* sect;

	sect = pSectionList;
	while(sect)
	{
		SPatch* patch;

		patch = sect->pPatches;
		while(patch)
		{
			int32_t v;

			if(patch_Evaluate(patch, patch->pExpression, &v))
			{
				list_Remove(sect->pPatches, patch);
				mem_Free(patch->pszFile);

				switch(patch->Type)
				{
					case PATCH_BYTE:
						if(v >= -128 && v <= 255)
							sect->pData[patch->Offset] = (uint8_t)v;
						else
							prj_Error(ERROR_EXPRESSION_N_BIT, 8);
						break;

					case PATCH_LWORD:
						if(v >= -32768 && v <= 65535)
						{
							sect->pData[patch->Offset] = (uint8_t)v;
							sect->pData[patch->Offset + 1] = (uint8_t)(v >> 8);
						}
						else
							prj_Error(ERROR_EXPRESSION_N_BIT, 16);
						break;

					case PATCH_BWORD:
						if(v >= -32768 && v <= 65535)
						{
							sect->pData[patch->Offset] = (uint8_t)(v >> 8);
							sect->pData[patch->Offset + 1] = (uint8_t)(v & 0xFF);
						}
						else
							prj_Error(ERROR_EXPRESSION_N_BIT, 16);
						break;

					case PATCH_LLONG:
						sect->pData[patch->Offset] = (uint8_t)v;
						sect->pData[patch->Offset + 1] = (uint8_t)(v >> 8);
						sect->pData[patch->Offset + 2] = (uint8_t)(v >> 16);
						sect->pData[patch->Offset + 3] = (uint8_t)(v >> 24);
						break;

					case PATCH_BLONG:
						sect->pData[patch->Offset] = (uint8_t)(v >> 24);
						sect->pData[patch->Offset + 1] = (uint8_t)(v >> 16);
						sect->pData[patch->Offset + 2] = (uint8_t)(v >> 8);
						sect->pData[patch->Offset + 3] = (uint8_t)v;
						break;

					default:
						internalerror("Unknown patchtype");
						break;
				}
			}
			patch = list_GetNext(patch);
		}

		sect = list_GetNext(sect);
	}
}

void patch_OptimizeExpression(SExpression* pExpr)
{
	if(pExpr->pLeft != NULL)
		patch_OptimizeExpression(pExpr->pLeft);

	if(pExpr->pRight != NULL)
		patch_OptimizeExpression(pExpr->pRight);

	if(expr_IsConstant(pExpr))
	{
		expr_Free(pExpr->pLeft);
		pExpr->pLeft = NULL;
		expr_Free(pExpr->pRight);
		pExpr->pRight = NULL;

		pExpr->eType = EXPR_CONSTANT;
		pExpr->eOperator = 0;
	}
}

void patch_OptimizeAll(void)
{
	SSection* sect;

	sect = pSectionList;
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
