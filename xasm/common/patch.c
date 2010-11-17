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
#include "patch.h"
#include "tokens.h"
#include "parse.h"
#include "project.h"
#include "fstack.h"


//	Private defines

typedef int32_t (*pPredicate_t)(int32_t nLeft, int32_t nRight);
typedef int32_t (*pPredicate1_t)(int32_t nValue);




//	Public variables




//	Private routines
static bool_t parsepatch(SPatch* patch, SExpression* expr, int32_t* v);

static void freeexpr(SExpression* expr)
{
	if(expr)
	{
		free(expr->pLeft);
		free(expr->pRight);
	}
}

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

static int32_t patch_Predbool_tAnd(int32_t nLeft, int32_t nRight)
{
	return nLeft && nRight;
}

static int32_t patch_Predbool_tOr(int32_t nLeft, int32_t nRight)
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

static int32_t patch_Predbool_tNot(int32_t nValue)
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



static bool_t joinexpr(SPatch* pPatch, SExpression* pExpr, pPredicate_t pPred)
{
	int32_t vl;
	int32_t vr;
	if(parsepatch(pPatch, pExpr->pLeft, &vl)
	&& parsepatch(pPatch, pExpr->pRight, &vr))
	{
		freeexpr(pExpr->pLeft);
		freeexpr(pExpr->pRight);
		pExpr->pLeft = pExpr->pRight = NULL;
		pExpr->eType = EXPR_CONSTANT;
		pExpr->Flags |= EXPRF_isCONSTANT;
		pExpr->Value.Value = pPred(vl, vr);
		return true;
	}
	return false;
}

static bool_t joinexpr1(SPatch* pPatch, SExpression* pExpr, pPredicate1_t pPred)
{
	int32_t vr;
	if(parsepatch(pPatch, pExpr->pRight, &vr))
	{
		freeexpr(pExpr->pRight);
		pExpr->pLeft = pExpr->pRight = NULL;
		pExpr->eType = EXPR_CONSTANT;
		pExpr->Flags |= EXPRF_isCONSTANT;
		pExpr->Value.Value = pPred(vr);
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
	else if(expr_GetType(pExpr) == EXPR_OPERATOR
	&& (pExpr->Operator == T_OP_ADD || pExpr->Operator == T_OP_SUB))
	{
		uint32_t offset;
		if(patch_GetImportOffset(&offset, ppSym, pExpr->pLeft))
		{
			if(pExpr->pRight->Flags & EXPRF_isCONSTANT)
			{
				if(pExpr->Operator == T_OP_ADD)
					*pOffset = offset + pExpr->pRight->Value.Value;
				else
					*pOffset = offset - pExpr->pRight->Value.Value;
				return true;
			}
		}
		if(patch_GetImportOffset(&offset, ppSym, pExpr->pRight))
		{
			if(pExpr->pLeft->Flags & EXPRF_isCONSTANT)
			{
				if(pExpr->Operator == T_OP_ADD)
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
	else if(expr_GetType(pExpr) == EXPR_SYMBOL)
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
	else if(expr_GetType(pExpr) == EXPR_OPERATOR
	&& (pExpr->Operator == T_OP_ADD || pExpr->Operator == T_OP_SUB))
	{
		uint32_t offset;
		if(patch_GetSectionOffset(&offset, pExpr->pLeft, pSection))
		{
			if(pExpr->pRight->Flags & EXPRF_isCONSTANT)
			{
				if(pExpr->Operator == T_OP_ADD)
					*pOffset = offset + pExpr->pRight->Value.Value;
				else
					*pOffset = offset - pExpr->pRight->Value.Value;
				return true;
			}
		}
		if(patch_GetSectionOffset(&offset, pExpr->pRight, pSection))
		{
			if(pExpr->pLeft->Flags & EXPRF_isCONSTANT)
			{
				if(pExpr->Operator == T_OP_ADD)
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

static bool_t parsepatch(SPatch* patch, SExpression* expr, int32_t* v)
{
	if(expr)
	{
		if(expr->Flags & EXPRF_isCONSTANT)
		{
			freeexpr(expr->pLeft);
			freeexpr(expr->pRight);
			expr->pLeft = NULL;
			expr->pRight = NULL;
			expr->eType = EXPR_CONSTANT;
			*v = expr->Value.Value;
			return true;
		}
		else
		{
			switch(expr_GetType(expr))
			{
				case EXPR_PCREL:
				{
					uint32_t offset;
					if(patch_GetSectionOffset(&offset, expr->pRight, patch->pSection))
					{
						int32_t v1;
						if(parsepatch(patch, expr->pLeft, &v1))
						{
							expr_FreeExpression(expr->pRight);
							expr->pRight = NULL;
							expr_FreeExpression(expr->pLeft);
							expr->pLeft = NULL;
							*v = offset + v1 - patch->Offset;
							return true;
						}
					}
					return false;
					break;
				}
				case EXPR_OPERATOR:
				{
					switch(expr->Operator)
					{
						case	T_OP_SUB:
						{
							uint32_t l, r;
							SSection* pLeftSect = patch_GetExpressionSectionAndOffset(patch, expr->pLeft, &l);
							SSection* pRightSect = patch_GetExpressionSectionAndOffset(patch, expr->pRight, &r);
							if(pLeftSect && pRightSect && pLeftSect == pRightSect)
							{
								expr->eType = EXPR_CONSTANT;
								expr->Flags = EXPRF_isCONSTANT;
								expr->Operator = 0;
								*v = expr->Value.Value = l - r;
								expr_FreeExpression(expr->pLeft);
								expr->pLeft = NULL;
								expr_FreeExpression(expr->pRight);
								expr->pRight = NULL;
								return true;
							}
							else
							{
								bool_t b = joinexpr(patch, expr, patch_PredSub);
								*v = expr->Value.Value;
								return b;
							}
							break;
						}
						case	T_OP_ADD:
						{
							bool_t b = joinexpr(patch, expr, patch_PredAdd);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_XOR:
						{
							bool_t b = joinexpr(patch, expr, patch_PredXor);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_OR:
						{
							bool_t b = joinexpr(patch, expr, patch_PredOr);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_AND:
						{
							bool_t b = joinexpr(patch, expr, patch_PredAnd);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_SHL:
						{
							bool_t b = joinexpr(patch, expr, patch_PredShl);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_SHR:
						{
							bool_t b = joinexpr(patch, expr, patch_PredShr);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_MUL:
						{
							bool_t b = joinexpr(patch, expr, patch_PredMul);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_DIV:
						{
							bool_t b = joinexpr(patch, expr, patch_PredDiv);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_MOD:
						{
							bool_t b = joinexpr(patch, expr, patch_PredMod);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_LOGICOR:
						{
							bool_t b = joinexpr(patch, expr, patch_Predbool_tOr);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_LOGICAND:
						{
							bool_t b = joinexpr(patch, expr, patch_Predbool_tAnd);
							*v = expr->Value.Value;
							return b;
							break;
						}

						case	T_OP_LOGICNOT:
						{
							bool_t b = joinexpr1(patch, expr, patch_Predbool_tNot);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_NOT:
						{
							bool_t b = joinexpr1(patch, expr, patch_PredNot);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_BIT:
						{
							int32_t	vr;
							if(parsepatch(patch,expr->pRight,&vr))
							{
								if((vr & -vr) == vr && vr != 0)
								{
									uint32_t t = vr;
									int b = 0;
									while(t != 1)
									{
										t >>= 1;
										++b;
									}
									*v = b;
									freeexpr(expr->pRight);
									expr->pLeft = expr->pRight = NULL;
									expr->eType = EXPR_CONSTANT;
									expr->Flags |= EXPRF_isCONSTANT;
									expr->Value.Value = *v;
									return true;
								}
								else
									prj_Error(ERROR_EXPR_TWO_POWER);
							}
							return false;
							break;
						}

						case	T_OP_LOGICGE:
						{
							bool_t b = joinexpr(patch, expr, patch_PredGe);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_LOGICGT:
						{
							bool_t b = joinexpr(patch, expr, patch_PredGt);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_LOGICLE:
						{
							bool_t b = joinexpr(patch, expr, patch_PredLe);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_LOGICLT:
						{
							bool_t b = joinexpr(patch, expr, patch_PredLt);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_LOGICEQU:
						{
							bool_t b = joinexpr(patch, expr, patch_PredEquals);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_LOGICNE:
						{
							bool_t b = joinexpr(patch, expr, patch_PredNotEquals);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_FUNC_LOWLIMIT:
						{
							int32_t	vl,vr;

							if( parsepatch(patch,expr->pRight,&vr)
							&&	parsepatch(patch,expr->pLeft,&vl) )
							{
								if(vl>=vr)
								{
									freeexpr(expr->pRight);
									freeexpr(expr->pLeft);
									expr->pLeft=expr->pRight=NULL;
									expr->eType=EXPR_CONSTANT;
									*v=expr->Value.Value=vl;
									return true;
								}
								prj_Fail(ERROR_OPERAND_RANGE);
								return false;
							}
							return false;
							break;
						}
						case	T_FUNC_HIGHLIMIT:
						{
							int32_t	vl,vr;

							if( parsepatch(patch,expr->pRight,&vr)
							&&	parsepatch(patch,expr->pLeft,&vl) )
							{
								if(vl<=vr)
								{
									freeexpr(expr->pRight);
									freeexpr(expr->pLeft);
									expr->pLeft=expr->pRight=NULL;
									expr->eType=EXPR_CONSTANT;
									*v=expr->Value.Value=vl;
									return true;
								}
								prj_Fail(ERROR_OPERAND_RANGE);
								return false;
							}
							return false;
							break;
						}
						case	T_FUNC_FDIV:
						{
							bool_t b = joinexpr(patch, expr, patch_PredFDiv);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_FUNC_FMUL:
						{
							bool_t b = joinexpr(patch, expr, patch_PredFMul);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_FUNC_ATAN2:
						{
							bool_t b = joinexpr(patch, expr, patch_PredAtan2);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_FUNC_SIN:
						{
							bool_t b = joinexpr1(patch, expr, patch_PredSin);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_FUNC_COS:
						{
							bool_t b = joinexpr1(patch, expr, patch_PredCos);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_FUNC_TAN:
						{
							bool_t b = joinexpr1(patch, expr, patch_PredTan);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_FUNC_ASIN:
						{
							bool_t b = joinexpr1(patch, expr, patch_PredAsin);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_FUNC_ACOS:
						{
							bool_t b = joinexpr1(patch, expr, patch_PredAcos);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_FUNC_ATAN:
						{
							bool_t b = joinexpr1(patch, expr, patch_PredAtan);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case T_FUNC_BANK:
						{
							if(!g_pConfiguration->bSupportBanks)
								internalerror("Banks not supported");

							return false;
						}
					}
					break;
				}
				case EXPR_CONSTANT:
				{
					*v = expr->Value.Value;
					return true;
					break;
				}
				case EXPR_SYMBOL:
				{
					if(expr->Value.pSymbol->Flags & SYMF_CONSTANT)
					{
						*v = expr->Value.pSymbol->Value.Value;
						return true;
					}
					else if(expr->Value.pSymbol->Type == SYM_UNDEFINED)
					{
						prj_Error(ERROR_SYMBOL_UNDEFINED, expr->Value.pSymbol->Name);
					}

					return false;
				}
				default:
				{
					internalerror("Unknown expression");
				}
			}
		}
	}
	return false;
}




//	Public routines

void	patch_Create(SSection* sect, uint32_t offset, SExpression* expr, EPatchType type)
{
	SPatch* patch;

	if((patch = malloc(sizeof(SPatch))) != NULL)
	{
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
	else
	{
		internalerror("Out of memory");
	}

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

			if(parsepatch(patch, patch->pExpression, &v))
			{
				if(patch->pPrev)
					patch->pPrev->pNext = patch->pNext;
				else
					sect->pPatches = patch->pNext;

				if(patch->pNext)
					patch->pNext->pPrev = patch->pPrev;

				free(patch->pszFile);

				switch(patch->Type)
				{
					case	PATCH_BYTE:
					{
						if(v>=-128 && v<=255)
						{
							sect->pData[patch->Offset] = (uint8_t)v;
						}
						else
						{
							prj_Error(ERROR_EXPRESSION_N_BIT, 8);
						}
						break;
					}
					case	PATCH_LWORD:
					{
						if(v>=-32768 && v<=65535)
						{
							sect->pData[patch->Offset] = (uint8_t)v;
							sect->pData[patch->Offset+1] = (uint8_t)(v>>8);
						}
						else
						{
							prj_Error(ERROR_EXPRESSION_N_BIT, 16);
						}
						break;
					}
					case	PATCH_BWORD:
					{
						if(v>=-32768 && v<=65535)
						{
							sect->pData[patch->Offset]=(uint8_t)(v>>8);
							sect->pData[patch->Offset+1]=(uint8_t)(v&0xFF);
						}
						else
						{
							prj_Error(ERROR_EXPRESSION_N_BIT, 16);
						}
						break;
					}
					case	PATCH_LLONG:
					{
						sect->pData[patch->Offset] = (uint8_t)v;
						sect->pData[patch->Offset+1] = (uint8_t)(v>>8);
						sect->pData[patch->Offset+2] = (uint8_t)(v>>16);
						sect->pData[patch->Offset+3] = (uint8_t)(v>>24);
						break;
					}
					case	PATCH_BLONG:
					{
						sect->pData[patch->Offset] = (uint8_t)(v>>24);
						sect->pData[patch->Offset+1] = (uint8_t)(v>>16);
						sect->pData[patch->Offset+2] = (uint8_t)(v>>8);
						sect->pData[patch->Offset+3] = (uint8_t)v;
						break;
					}
					default:
					{
						internalerror("Unknown patchtype");
						break;
					}
				}
			}
			patch = list_GetNext(patch);
		}

		sect=list_GetNext(sect);
	}
}

void patch_OptimizeExpression(SExpression* pExpr)
{
	if(pExpr->pLeft != NULL)
		patch_OptimizeExpression(pExpr->pLeft);

	if(pExpr->pRight != NULL)
		patch_OptimizeExpression(pExpr->pRight);

	if(pExpr->Flags & EXPRF_isCONSTANT)
	{
		expr_FreeExpression(pExpr->pLeft);
		pExpr->pLeft = NULL;
		expr_FreeExpression(pExpr->pRight);
		pExpr->pRight = NULL;
		pExpr->eType = EXPR_CONSTANT;
		pExpr->Operator = 0;
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
