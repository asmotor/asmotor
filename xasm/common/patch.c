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

typedef SLONG (*pPredicate_t)(SLONG nLeft, SLONG nRight);
typedef SLONG (*pPredicate1_t)(SLONG nValue);




//	Public variables




//	Private routines
static BOOL parsepatch(SPatch* patch, SExpression* expr, SLONG* v);

static void freeexpr(SExpression* expr)
{
	if(expr)
	{
		free(expr->pLeft);
		free(expr->pRight);
	}
}

static SLONG patch_PredSub(SLONG nLeft, SLONG nRight)
{
	return nLeft - nRight;
}

static SLONG patch_PredAdd(SLONG nLeft, SLONG nRight)
{
	return nLeft + nRight;
}

static SLONG patch_PredXor(SLONG nLeft, SLONG nRight)
{
	return nLeft ^ nRight;
}

static SLONG patch_PredOr(SLONG nLeft, SLONG nRight)
{
	return nLeft | nRight;
}

static SLONG patch_PredAnd(SLONG nLeft, SLONG nRight)
{
	return nLeft & nRight;
}

static SLONG patch_PredShl(SLONG nLeft, SLONG nRight)
{
	return nLeft << nRight;
}

static SLONG patch_PredShr(SLONG nLeft, SLONG nRight)
{
	return nLeft >> nRight;
}

static SLONG patch_PredMul(SLONG nLeft, SLONG nRight)
{
	return nLeft * nRight;
}

static SLONG patch_PredDiv(SLONG nLeft, SLONG nRight)
{
	return nLeft / nRight;
}

static SLONG patch_PredMod(SLONG nLeft, SLONG nRight)
{
	return nLeft % nRight;
}

static SLONG patch_PredBoolAnd(SLONG nLeft, SLONG nRight)
{
	return nLeft && nRight;
}

static SLONG patch_PredBoolOr(SLONG nLeft, SLONG nRight)
{
	return nLeft || nRight;
}

static SLONG patch_PredGe(SLONG nLeft, SLONG nRight)
{
	return nLeft >= nRight;
}

static SLONG patch_PredGt(SLONG nLeft, SLONG nRight)
{
	return nLeft > nRight;
}

static SLONG patch_PredLe(SLONG nLeft, SLONG nRight)
{
	return nLeft <= nRight;
}

static SLONG patch_PredLt(SLONG nLeft, SLONG nRight)
{
	return nLeft < nRight;
}

static SLONG patch_PredEquals(SLONG nLeft, SLONG nRight)
{
	return nLeft == nRight;
}

static SLONG patch_PredNotEquals(SLONG nLeft, SLONG nRight)
{
	return nLeft != nRight;
}

static SLONG patch_PredFDiv(SLONG nLeft, SLONG nRight)
{
	return fdiv(nLeft, nRight);
}

static SLONG patch_PredFMul(SLONG nLeft, SLONG nRight)
{
	return fmul(nLeft, nRight);
}

static SLONG patch_PredAtan2(SLONG nLeft, SLONG nRight)
{
	return fatan2(nLeft, nRight);
}

static SLONG patch_PredBoolNot(SLONG nValue)
{
	return nValue ? 0 : 1;
}

static SLONG patch_PredNot(SLONG nValue)
{
	return ~nValue;
}

static SLONG patch_PredSin(SLONG nValue)
{
	return fsin(nValue);
}

static SLONG patch_PredCos(SLONG nValue)
{
	return fcos(nValue);
}

static SLONG patch_PredTan(SLONG nValue)
{
	return ftan(nValue);
}

static SLONG patch_PredAsin(SLONG nValue)
{
	return fasin(nValue);
}

static SLONG patch_PredAcos(SLONG nValue)
{
	return facos(nValue);
}

static SLONG patch_PredAtan(SLONG nValue)
{
	return fatan(nValue);
}



static BOOL joinexpr(SPatch* pPatch, SExpression* pExpr, pPredicate_t pPred)
{
	SLONG vl;
	SLONG vr;
	if(parsepatch(pPatch, pExpr->pLeft, &vl)
	&& parsepatch(pPatch, pExpr->pRight, &vr))
	{
		freeexpr(pExpr->pLeft);
		freeexpr(pExpr->pRight);
		pExpr->pLeft = pExpr->pRight = NULL;
		pExpr->Type = EXPR_CONSTANT;
		pExpr->Flags |= EXPRF_isCONSTANT;
		pExpr->Value.Value = pPred(vl, vr);
		return TRUE;
	}
	return FALSE;
}

static BOOL joinexpr1(SPatch* pPatch, SExpression* pExpr, pPredicate1_t pPred)
{
	SLONG vr;
	if(parsepatch(pPatch, pExpr->pRight, &vr))
	{
		freeexpr(pExpr->pRight);
		pExpr->pLeft = pExpr->pRight = NULL;
		pExpr->Type = EXPR_CONSTANT;
		pExpr->Flags |= EXPRF_isCONSTANT;
		pExpr->Value.Value = pPred(vr);
		return TRUE;
	}
	return FALSE;
}

BOOL patch_GetImportOffset(ULONG* pOffset, SSymbol** ppSym, SExpression* pExpr)
{
	if(pExpr == NULL)
		return FALSE;

	if(pExpr->Type == EXPR_SYMBOL)
	{
		SSymbol* pSym = pExpr->Value.pSymbol;
		if(pSym->Type == SYM_IMPORT || pSym->Type == SYM_GLOBAL)
		{
			if(*ppSym != NULL)
				return FALSE;

			*ppSym = pSym;
			*pOffset = 0;
			return TRUE;
		}
		return FALSE;
	}
	else if(pExpr->Type == EXPR_OPERATOR
	&& (pExpr->Operator == T_OP_ADD || pExpr->Operator == T_OP_SUB))
	{
		ULONG offset;
		if(patch_GetImportOffset(&offset, ppSym, pExpr->pLeft))
		{
			if(pExpr->pRight->Flags & EXPRF_isCONSTANT)
			{
				if(pExpr->Operator == T_OP_ADD)
					*pOffset = offset + pExpr->pRight->Value.Value;
				else
					*pOffset = offset - pExpr->pRight->Value.Value;
				return TRUE;
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
				return TRUE;
			}
		}
	}
	return FALSE;
}

BOOL patch_GetSectionOffset(ULONG* pOffset, SExpression* pExpr, SSection* pSection)
{
	if(pExpr == NULL)
		return FALSE;

	if(pExpr->Type == EXPR_CONSTANT && (pSection->Flags & SECTF_ORGFIXED))
	{
		*pOffset = pExpr->Value.Value - pSection->Org;
		return TRUE;
	}
	else if(pExpr->Type == EXPR_SYMBOL)
	{
		SSymbol* pSym = pExpr->Value.pSymbol;
		if(pSym->pSection == pSection)
		{
			if((pSym->Flags & SYMF_CONSTANT) && (pSection->Flags & SECTF_ORGFIXED))
			{
				*pOffset = pSym->Value.Value - pSection->Org;
				return TRUE;
			}
			else if((pSym->Flags & SYMF_RELOC) && (pSection->Flags == 0))
			{
				*pOffset = pSym->Value.Value;
				return TRUE;
			}
		}
		return FALSE;
	}
	else if(pExpr->Type == EXPR_OPERATOR
	&& (pExpr->Operator == T_OP_ADD || pExpr->Operator == T_OP_SUB))
	{
		ULONG offset;
		if(patch_GetSectionOffset(&offset, pExpr->pLeft, pSection))
		{
			if(pExpr->pRight->Flags & EXPRF_isCONSTANT)
			{
				if(pExpr->Operator == T_OP_ADD)
					*pOffset = offset + pExpr->pRight->Value.Value;
				else
					*pOffset = offset - pExpr->pRight->Value.Value;
				return TRUE;
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
				return TRUE;
			}
		}
	}
	return FALSE;
}

SSection* patch_GetExpressionSectionAndOffset(SPatch* pPatch, SExpression* pExpr, ULONG* pOffset)
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

static BOOL parsepatch(SPatch* patch, SExpression* expr, SLONG* v)
{
	if(expr)
	{
		if(expr->Flags&EXPRF_isCONSTANT)
		{
			freeexpr(expr->pLeft);
			freeexpr(expr->pRight);
			expr->pLeft=NULL;
			expr->pRight=NULL;
			expr->Type=EXPR_CONSTANT;
			*v=expr->Value.Value;
			return TRUE;
		}
		else
		{
			switch(expr->Type)
			{
				case EXPR_PCREL:
				{
					ULONG offset;
					if(patch_GetSectionOffset(&offset, expr->pRight, patch->pSection))
					{
						SLONG v1;
						if(parsepatch(patch, expr->pLeft, &v1))
						{
							parse_FreeExpression(expr->pRight);
							expr->pRight = NULL;
							parse_FreeExpression(expr->pLeft);
							expr->pLeft = NULL;
							*v = offset + v1 - patch->Offset;
							return TRUE;
						}
					}
					return FALSE;
					break;
				}
				case EXPR_OPERATOR:
				{
					switch(expr->Operator)
					{
						case	T_OP_SUB:
						{
							ULONG l, r;
							SSection* pLeftSect = patch_GetExpressionSectionAndOffset(patch, expr->pLeft, &l);
							SSection* pRightSect = patch_GetExpressionSectionAndOffset(patch, expr->pRight, &r);
							if(pLeftSect && pRightSect && pLeftSect == pRightSect)
							{
								expr->Type = EXPR_CONSTANT;
								expr->Flags = EXPRF_isCONSTANT;
								expr->Operator = 0;
								*v = expr->Value.Value = l - r;
								parse_FreeExpression(expr->pLeft);
								expr->pLeft = NULL;
								parse_FreeExpression(expr->pRight);
								expr->pRight = NULL;
								return TRUE;
							}
							else
							{
								BOOL b = joinexpr(patch, expr, patch_PredSub);
								*v = expr->Value.Value;
								return b;
							}
							break;
						}
						case	T_OP_ADD:
						{
							BOOL b = joinexpr(patch, expr, patch_PredAdd);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_XOR:
						{
							BOOL b = joinexpr(patch, expr, patch_PredXor);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_OR:
						{
							BOOL b = joinexpr(patch, expr, patch_PredOr);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_AND:
						{
							BOOL b = joinexpr(patch, expr, patch_PredAnd);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_SHL:
						{
							BOOL b = joinexpr(patch, expr, patch_PredShl);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_SHR:
						{
							BOOL b = joinexpr(patch, expr, patch_PredShr);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_MUL:
						{
							BOOL b = joinexpr(patch, expr, patch_PredMul);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_DIV:
						{
							BOOL b = joinexpr(patch, expr, patch_PredDiv);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_MOD:
						{
							BOOL b = joinexpr(patch, expr, patch_PredMod);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_LOGICOR:
						{
							BOOL b = joinexpr(patch, expr, patch_PredBoolOr);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_LOGICAND:
						{
							BOOL b = joinexpr(patch, expr, patch_PredBoolAnd);
							*v = expr->Value.Value;
							return b;
							break;
						}

						case	T_OP_LOGICNOT:
						{
							BOOL b = joinexpr1(patch, expr, patch_PredBoolNot);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_NOT:
						{
							BOOL b = joinexpr1(patch, expr, patch_PredNot);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_BIT:
						{
							SLONG	vr;
							if(parsepatch(patch,expr->pRight,&vr))
							{
								if((vr & -vr) == vr && vr != 0)
								{
									ULONG t = vr;
									int b = 0;
									while(t != 1)
									{
										t >>= 1;
										++b;
									}
									*v = b;
									freeexpr(expr->pRight);
									expr->pLeft = expr->pRight = NULL;
									expr->Type = EXPR_CONSTANT;
									expr->Flags |= EXPRF_isCONSTANT;
									expr->Value.Value = *v;
									return TRUE;
								}
								else
									prj_Error(ERROR_EXPR_TWO_POWER);
							}
							return FALSE;
							break;
						}

						case	T_OP_LOGICGE:
						{
							BOOL b = joinexpr(patch, expr, patch_PredGe);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_LOGICGT:
						{
							BOOL b = joinexpr(patch, expr, patch_PredGt);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_LOGICLE:
						{
							BOOL b = joinexpr(patch, expr, patch_PredLe);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_LOGICLT:
						{
							BOOL b = joinexpr(patch, expr, patch_PredLt);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_LOGICEQU:
						{
							BOOL b = joinexpr(patch, expr, patch_PredEquals);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_OP_LOGICNE:
						{
							BOOL b = joinexpr(patch, expr, patch_PredNotEquals);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_FUNC_LOWLIMIT:
						{
							SLONG	vl,vr;

							if( parsepatch(patch,expr->pRight,&vr)
							&&	parsepatch(patch,expr->pLeft,&vl) )
							{
								if(vl>=vr)
								{
									freeexpr(expr->pRight);
									freeexpr(expr->pLeft);
									expr->pLeft=expr->pRight=NULL;
									expr->Type=EXPR_CONSTANT;
									*v=expr->Value.Value=vl;
									return TRUE;
								}
								prj_Fail(ERROR_OPERAND_RANGE);
								return FALSE;
							}
							return FALSE;
							break;
						}
						case	T_FUNC_HIGHLIMIT:
						{
							SLONG	vl,vr;

							if( parsepatch(patch,expr->pRight,&vr)
							&&	parsepatch(patch,expr->pLeft,&vl) )
							{
								if(vl<=vr)
								{
									freeexpr(expr->pRight);
									freeexpr(expr->pLeft);
									expr->pLeft=expr->pRight=NULL;
									expr->Type=EXPR_CONSTANT;
									*v=expr->Value.Value=vl;
									return TRUE;
								}
								prj_Fail(ERROR_OPERAND_RANGE);
								return FALSE;
							}
							return FALSE;
							break;
						}
						case	T_FUNC_FDIV:
						{
							BOOL b = joinexpr(patch, expr, patch_PredFDiv);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_FUNC_FMUL:
						{
							BOOL b = joinexpr(patch, expr, patch_PredFMul);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_FUNC_ATAN2:
						{
							BOOL b = joinexpr(patch, expr, patch_PredAtan2);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_FUNC_SIN:
						{
							BOOL b = joinexpr1(patch, expr, patch_PredSin);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_FUNC_COS:
						{
							BOOL b = joinexpr1(patch, expr, patch_PredCos);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_FUNC_TAN:
						{
							BOOL b = joinexpr1(patch, expr, patch_PredTan);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_FUNC_ASIN:
						{
							BOOL b = joinexpr1(patch, expr, patch_PredAsin);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_FUNC_ACOS:
						{
							BOOL b = joinexpr1(patch, expr, patch_PredAcos);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case	T_FUNC_ATAN:
						{
							BOOL b = joinexpr1(patch, expr, patch_PredAtan);
							*v = expr->Value.Value;
							return b;
							break;
						}
						case T_FUNC_BANK:
						{
							if(!g_pConfiguration->bSupportBanks)
								internalerror("Banks not supported");

							return FALSE;
						}
					}
					break;
				}
				case EXPR_CONSTANT:
				{
					*v = expr->Value.Value;
					return TRUE;
					break;
				}
				case EXPR_SYMBOL:
				{
					if(expr->Value.pSymbol->Flags & SYMF_CONSTANT)
					{
						*v = expr->Value.pSymbol->Value.Value;
						return TRUE;
					}
					else if(expr->Value.pSymbol->Type == SYM_UNDEFINED)
					{
						prj_Error(ERROR_SYMBOL_UNDEFINED, expr->Value.pSymbol->Name);
					}

					return FALSE;
				}
				default:
				{
					internalerror("Unknown expression");
				}
			}
		}
	}
	return FALSE;
}




//	Public routines

void	patch_Create(SSection* sect, ULONG offset, SExpression* expr, EPatchType type)
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
			SLONG v;

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
							sect->pData[patch->Offset] = (UBYTE)v;
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
							sect->pData[patch->Offset] = (UBYTE)v;
							sect->pData[patch->Offset+1] = (UBYTE)(v>>8);
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
							sect->pData[patch->Offset]=(UBYTE)(v>>8);
							sect->pData[patch->Offset+1]=(UBYTE)(v&0xFF);
						}
						else
						{
							prj_Error(ERROR_EXPRESSION_N_BIT, 16);
						}
						break;
					}
					case	PATCH_LLONG:
					{
						sect->pData[patch->Offset] = (UBYTE)v;
						sect->pData[patch->Offset+1] = (UBYTE)(v>>8);
						sect->pData[patch->Offset+2] = (UBYTE)(v>>16);
						sect->pData[patch->Offset+3] = (UBYTE)(v>>24);
						break;
					}
					case	PATCH_BLONG:
					{
						sect->pData[patch->Offset] = (UBYTE)(v>>24);
						sect->pData[patch->Offset+1] = (UBYTE)(v>>16);
						sect->pData[patch->Offset+2] = (UBYTE)(v>>8);
						sect->pData[patch->Offset+3] = (UBYTE)v;
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
		parse_FreeExpression(pExpr->pLeft);
		pExpr->pLeft = NULL;
		parse_FreeExpression(pExpr->pRight);
		pExpr->pRight = NULL;
		pExpr->Type = EXPR_CONSTANT;
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
