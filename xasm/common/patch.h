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

#ifndef	INCLUDE_PATCH_H
#define	INCLUDE_PATCH_H

#include "expr.h"

struct	Section;

typedef	enum
{
	PATCH_BYTE,
	PATCH_LWORD,
	PATCH_BWORD,
	PATCH_LLONG,
	PATCH_BLONG,
} EPatchType;

struct Patch
{
	list_Data(struct Patch);
	struct Section* pSection;
	ULONG		Offset;
 	EPatchType	Type;
	SExpression* pExpression;
	char*	pszFile;
	int		nLine;
};
typedef	struct Patch SPatch;

#include "section.h"
#include "symbol.h"

extern void patch_Create(SSection* sect, ULONG offset, SExpression* expr, EPatchType type);
extern void patch_BackPatch(void);
extern void patch_OptimizeAll(void);
extern BOOL patch_GetSectionOffset(ULONG* pOffset, SExpression* pExpr, SSection* pSection);
extern BOOL patch_GetImportOffset(ULONG* pOffset, SSymbol** ppSym, SExpression* pExpr);


#endif	/*INCLUDE_PATCH_H*/