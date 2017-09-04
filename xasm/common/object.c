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

/*	char	ID[4]="XOB\1";
 *	char	MinimumWordSize ; Used for address calculations.
 *							; 1 - A CPU address points to a byte in memory
 *							; 2 - A CPU address points to a 16 bit word in memory (CPU address 0x1000 is the 0x2000th byte)
 *							; 4 - A CPU address points to a 32 bit word in memory (CPU address 0x1000 is the 0x4000th byte)
 *	uint32_t	NumberOfGroups
 *	REPT	NumberOfGroups
 *			ASCIIZ	Name
 *			uint32_t	Type
 *	ENDR
 *	uint32_t	NumberOfSections
 *	REPT	NumberOfSections
 *			int32_t	GroupID	; -1 = exported EQU symbols
 *			ASCIIZ	Name
 *			int32_t	Bank	; -1 = not bankfixed
 *			int32_t	Position; -1 = not fixed
 *			int32_t	BasePC	; -1 = not fixed
 *			uint32_t	NumberOfSymbols
 *			REPT	NumberOfSymbols
 *					ASCIIZ	Name
 *					uint32_t	Type	;0=EXPORT
 *									;1=IMPORT
 *									;2=LOCAL
 *									;3=LOCALEXPORT
 *									;4=LOCALIMPORT
 *					IF Type==EXPORT or LOCAL or LOCALEXPORT
 *						int32_t	Value
 *					ENDC
 *			ENDR
 *			uint32_t	Size
 *			IF	SectionCanContainData
 *					uint8_t	Data[Size]
 *					uint32_t	NumberOfPatches
 *					REPT	NumberOfPatches
 *							uint32_t	Offset
 *							uint32_t	Type
 *							uint32_t	ExprSize
 *							uint8_t	Expr[ExprSize]
 *					ENDR
 *			ENDC
 *	ENDR
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "asmotor.h"
#include "xasm.h"
#include "section.h"
#include "expr.h"
#include "symbol.h"
#include "tokens.h"
#include "object.h"
#include "patch.h"




//	Private routines

static void fputll(int32_t d, FILE* f)
{
	fputc(d&0xFF, f);
	fputc((d>>8)&0xFF, f);
	fputc((d>>16)&0xFF, f);
	fputc((d>>24)&0xFF, f);
}

static void fputasciiz(char* s, FILE* f)
{
	while(*s)
	{
		fputc(*s++, f);
	}
	fputc(0, f);
}

static uint32_t calc_symbol_ids(SSection* sect, FILE* f, SExpression* expr, uint32_t ID)
{
	if(expr)
	{
		ID=calc_symbol_ids(sect, f, expr->pLeft, ID);
		ID=calc_symbol_ids(sect, f, expr->pRight, ID);

		if(expr_GetType(expr) == EXPR_SYMBOL
		||(g_pConfiguration->bSupportBanks && expr_IsOperator(expr, T_FUNC_BANK)))
		{
			if(expr->Value.pSymbol->ID == -1)
			{
				expr->Value.pSymbol->ID = ID++;
				fputasciiz(str_String(expr->Value.pSymbol->pName), f);
				if(expr->Value.pSymbol->pSection==sect)
				{
					if(expr->Value.pSymbol->nFlags & SYMF_LOCALEXPORT)
					{
						fputll(3, f);	//	LOCALEXPORT
						fputll(expr->Value.pSymbol->Value.Value, f);
					}
					else if(expr->Value.pSymbol->nFlags & SYMF_EXPORT)
					{
						fputll(0, f);	//	EXPORT
						fputll(expr->Value.pSymbol->Value.Value, f);
					}
					else
					{
						fputll(2, f);	//	LOCAL
						fputll(expr->Value.pSymbol->Value.Value, f);
					}
				}
				else if(expr->Value.pSymbol->eType == SYM_IMPORT || expr->Value.pSymbol->eType == SYM_GLOBAL)
				{
					fputll(1, f);	//	IMPORT
				}
				else
				{
					fputll(4, f);	//	LOCALIMPORT
				}
			}
		}
	}
	return ID;
}

static void fix_local_exports(SSection* sect, SExpression* expr)
{
	if(expr)
	{
		fix_local_exports(sect, expr->pLeft);
		fix_local_exports(sect, expr->pRight);

		if((expr_GetType(expr) == EXPR_SYMBOL || (g_pConfiguration->bSupportBanks && expr_IsOperator(expr, T_FUNC_BANK)))
		&& (expr->Value.pSymbol->nFlags & SYMF_EXPORTABLE)
		&& expr->Value.pSymbol->pSection != sect)
		{
			expr->Value.pSymbol->nFlags |= SYMF_LOCALEXPORT;
		}
	}
}

static int write_expr(FILE* f, SExpression* expr)
{
	int	r = 0;

	if(expr)
	{
		r += write_expr(f, expr->pLeft);
		r += write_expr(f, expr->pRight);

		switch(expr_GetType(expr))
		{
			case EXPR_PARENS:
			{
				return write_expr(f, expr->pRight);
			}
			case EXPR_OPERATOR:
			{
				switch(expr->eOperator)
				{
					default:
						internalerror("Unknown operator");
						break;
					case T_OP_SUB:
						fputc(OBJ_OP_SUB, f);
						++r;
						break;
					case T_OP_ADD:
						fputc(OBJ_OP_ADD, f);
						++r;
						break;
					case T_OP_XOR:
						fputc(OBJ_OP_XOR, f);
						++r;
						break;
					case T_OP_OR:
						fputc(OBJ_OP_OR, f);
						++r;
						break;
					case T_OP_AND:
						fputc(OBJ_OP_AND, f);
						++r;
						break;
					case T_OP_SHL:
						fputc(OBJ_OP_SHL, f);
						++r;
						break;
					case T_OP_SHR:
						fputc(OBJ_OP_SHR, f);
						++r;
						break;
					case T_OP_MUL:
						fputc(OBJ_OP_MUL, f);
						++r;
						break;
					case T_OP_DIV:
						fputc(OBJ_OP_DIV, f);
						++r;
						break;
					case T_OP_MOD:
						fputc(OBJ_OP_MOD, f);
						++r;
						break;
					case T_OP_LOGICOR:
						fputc(OBJ_OP_LOGICOR, f);
						++r;
						break;
					case T_OP_LOGICAND:
						fputc(OBJ_OP_LOGICAND, f);
						++r;
						break;
					case T_OP_LOGICNOT:
						fputc(OBJ_OP_LOGICNOT, f);
						++r;
						break;
					case T_OP_LOGICGE:
						fputc(OBJ_OP_LOGICGE, f);
						++r;
						break;
					case T_OP_LOGICGT:
						fputc(OBJ_OP_LOGICGT, f);
						++r;
						break;
					case T_OP_LOGICLE:
						fputc(OBJ_OP_LOGICLE, f);
						++r;
						break;
					case T_OP_LOGICLT:
						fputc(OBJ_OP_LOGICLT, f);
						++r;
						break;
					case T_OP_LOGICEQU:
						fputc(OBJ_OP_LOGICEQU, f);
						++r;
						break;
					case T_OP_LOGICNE:
						fputc(OBJ_OP_LOGICNE, f);
						++r;
						break;
					case T_FUNC_LOWLIMIT:
						fputc(OBJ_FUNC_LOWLIMIT, f);
						++r;
						break;
					case T_FUNC_HIGHLIMIT:
						fputc(OBJ_FUNC_HIGHLIMIT, f);
						++r;
						break;
					case T_FUNC_FDIV:
						fputc(OBJ_FUNC_FDIV, f);
						++r;
						break;
					case T_FUNC_FMUL:
						fputc(OBJ_FUNC_FMUL, f);
						++r;
						break;
					case T_FUNC_ATAN2:
						fputc(OBJ_FUNC_ATAN2, f);
						++r;
						break;
					case T_FUNC_SIN:
						fputc(OBJ_FUNC_SIN, f);
						++r;
						break;
					case T_FUNC_COS:
						fputc(OBJ_FUNC_COS, f);
						++r;
						break;
					case T_FUNC_TAN:
						fputc(OBJ_FUNC_TAN, f);
						++r;
						break;
					case T_FUNC_ASIN:
						fputc(OBJ_FUNC_ASIN, f);
						++r;
						break;
					case T_FUNC_ACOS:
						fputc(OBJ_FUNC_ACOS, f);
						++r;
						break;
					case T_FUNC_ATAN:
						fputc(OBJ_FUNC_ATAN, f);
						++r;
						break;
					case T_FUNC_BANK:
					{
						if(g_pConfiguration->bSupportBanks)
						{
							fputc(OBJ_FUNC_BANK, f);
							fputll(expr->Value.pSymbol->ID, f);
							r += 5;
						}
						else
							internalerror("Banks not supported");
						break;
					}
				}

				break;
			}
			case EXPR_CONSTANT:
			{
				fputc(OBJ_CONSTANT, f);
				fputll(expr->Value.Value, f);
				r += 5;
				break;
			}
			case EXPR_SYMBOL:
			{
				fputc(OBJ_SYMBOL, f);
				fputll(expr->Value.pSymbol->ID, f);
				r += 5;
				break;
			}
			case EXPR_PCREL:
			{
				fputc(OBJ_PCREL, f);
				++r;
				break;
			}
			default:
			{
				internalerror("Unknown expression");
				break;
			}
		}
	}

	return r;
}

static	void	write_patch(FILE* f, SPatch* patch)
{
/*
 *	uint32_t	Offset
 *	uint32_t	Type
 *	uint32_t	ExprSize
 *	uint8_t	Expr[ExprSize]
 */

	long size;
	long here;
	int newsize;

	fputll(patch->Offset, f);
	fputll(patch->Type, f);
	size = ftell(f);
	fputll(0, f);
	newsize = write_expr(f, patch->pExpression);
	here = ftell(f);
	fseek(f, size, SEEK_SET);
	fputll(newsize, f);
	fseek(f, here, SEEK_SET);
}




//	Public routines

bool_t obj_Write(string* pName)
{
	FILE* f;
	int i;
	uint32_t groupcount = 0;
	uint32_t equsetcount = 0;
	uint32_t sectioncount = 0;
	long pos;
	long pos2;
	SSection* sect;

	if((f = fopen(str_String(pName),"wb")) == NULL)
		return false;


	fwrite("XOB\1", 1, 4, f);
	fputc(g_pConfiguration->eMinimumWordSize, f);
	fputll(0, f);

	for(i = 0; i < HASHSIZE; ++i)
	{
		SSymbol* sym = g_pHashedSymbols[i];

		while(sym)
		{
			if(sym->eType == SYM_GROUP)
			{
				sym->ID = groupcount++;
				fputasciiz(str_String(sym->pName), f);
				fputll((sym->Value.GroupType) | (sym->nFlags & (SYMF_CHIP | SYMF_DATA)), f);
			}
			sym = list_GetNext(sym);
		}
	}

	pos = ftell(f);
	fseek(f, 5, SEEK_SET);
	fputll(groupcount, f);
	fseek(f, pos, SEEK_SET);

	//	Output sections

	for(sect = pSectionList; sect; sect = list_GetNext(sect))
		++sectioncount;

	fputll(sectioncount + 1, f);

	//	Output exported EQU and SET section
	fputll(-1, f);	//	GroupID , -1 for EQU symbols
	fputc(0, f);		//	Name
	fputll(-1, f);	//	Bank
	fputll(-1, f);	//	Org
	fputll(-1, f);	//	BasePC
	pos = ftell(f);
	fputll(0, f);		//	Number of symbols

	for(i = 0; i < HASHSIZE; ++i)
	{
		SSymbol* sym;
			
		for(sym = g_pHashedSymbols[i]; sym; sym = list_GetNext(sym))
		{
			if((sym->eType == SYM_EQU || sym->eType == SYM_SET)
			&& (sym->nFlags & SYMF_EXPORT))
			{
				++equsetcount;
				fputasciiz(str_String(sym->pName), f);
				fputll(0, f);	/* EXPORT */
				fputll(sym->Value.Value, f);
			}
		}
	}

	pos2 = ftell(f);
	fseek(f, pos, SEEK_SET);
	fputll(equsetcount, f);
	fseek(f, pos2, SEEK_SET);
	fputll(0, f);		//	Size
 
	//	Fix symbols that should be locally exported

	for(sect = pSectionList; sect; sect = list_GetNext(sect))
	{
		SPatch* patch;

		for(patch = sect->pPatches; patch; patch = list_GetNext(patch))
		{
			if(patch->pSection == sect)
				fix_local_exports(sect, patch->pExpression);
		}
	}


	//	Output other sections

	for(sect = pSectionList; sect; sect = list_GetNext(sect))
	{
		SPatch* patch;
		long sympos;
		long oldpos;
		long ID;
		long totalpatches;

		fputll(sect->pGroup->ID, f);
		fputasciiz(sect->Name, f);
		if(sect->Flags & SECTF_BANKFIXED)
		{
			if(!g_pConfiguration->bSupportBanks)
				internalerror("Banks not supported");
			fputll(sect->Bank, f);
		}
		else
			fputll(-1, f);

		fputll(sect->Flags & SECTF_LOADFIXED ? sect->Position : -1, f);
		fputll(sect->Flags & SECTF_LOADFIXED ? sect->BasePC : -1, f);

		//	Reset symbol IDs

		sympos = ftell(f);
		fputll(0, f);

		ID = 0;

		for(i = 0; i < HASHSIZE; ++i)
		{
			SSymbol* sym;

			for(sym = g_pHashedSymbols[i]; sym; sym = list_GetNext(sym))
			{
				if(sym->eType != SYM_GROUP)
					sym->ID = (uint32_t)-1;

				if(sym->pSection == sect
				&& (sym->nFlags & (SYMF_EXPORT | SYMF_LOCALEXPORT)))
				{
					sym->ID = ID++;

					fputasciiz(str_String(sym->pName), f);
					if(sym->nFlags & SYMF_EXPORT)
						fputll(0, f);	//	EXPORT
					else if(sym->nFlags & SYMF_LOCALEXPORT)
						fputll(3, f);	//	LOCALEXPORT
					fputll(sym->Value.Value, f);
				}
			}
		}

		//	Calculate and export symbols IDs by going through patches

		for(totalpatches = 0, patch = sect->pPatches; patch; patch = list_GetNext(patch))
		{
			if(patch->pSection == sect)
			{
				ID = calc_symbol_ids(sect, f, patch->pExpression, ID);
				++totalpatches;
			}
		}

		//	Fix up number of symbols

		oldpos = ftell(f);
		fseek(f, sympos, SEEK_SET);
		fputll(ID, f);
		fseek(f, oldpos, SEEK_SET);

/*
*			uint32_t	Size
*			IF	SectionCanContainData
*					uint8_t	Data[Size]
*					uint32_t	NumberOfPatches
*					REPT	NumberOfPatches
*							uint32_t	Offset
*							uint32_t	Type
*							uint32_t	ExprSize
*							uint8_t	Expr[ExprSize]
*					ENDR
*			ENDC
*/

		fputll(sect->UsedSpace, f);

		if(sect->pGroup->Value.GroupType == GROUP_TEXT)
		{
			fwrite(sect->pData, 1, sect->UsedSpace, f);
			fputll(totalpatches, f);

			for(patch = sect->pPatches; patch; patch=list_GetNext(patch))
			{
				if(patch->pSection == sect)
					write_patch(f, patch);
			}
		}
	}

	fclose(f);
	return true;
}
