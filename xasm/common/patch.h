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

#include "lists.h"
#include "expr.h"
#include "str.h"

struct Section;

typedef	enum
{
	PATCH_BYTE,
	PATCH_LWORD,
	PATCH_BWORD,
	PATCH_LLONG,
	PATCH_BLONG,
} EPatchType;

typedef struct Patch
{
	list_Data(struct Patch);
	struct Section* pSection;
	uint32_t	Offset;
 	EPatchType	Type;
	SExpression* pExpression;
	string*		pFile;
	int			nLine;
} SPatch;

#include "section.h"
#include "symbol.h"

extern void patch_Create(SSection* sect, uint32_t offset, SExpression* expr, EPatchType type);
extern void patch_BackPatch(void);
extern void patch_OptimizeAll(void);
extern bool_t patch_IsRelativeToSection(SExpression* pExpr, SSection* pSection);
extern bool_t patch_GetSectionPcOffset(uint32_t* pOffset, SExpression* pExpr, SSection* pSection);
extern bool_t patch_GetImportOffset(uint32_t* pOffset, SSymbol** ppSym, SExpression* pExpr);


#endif	/*INCLUDE_PATCH_H*/
