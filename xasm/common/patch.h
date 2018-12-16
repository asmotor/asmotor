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

#ifndef XASM_COMMON_PATCH_H_INCLUDED_
#define XASM_COMMON_PATCH_H_INCLUDED_

#include "expression.h"
#include "lists.h"
#include "str.h"

struct Section;

typedef enum {
    PATCH_BYTE,
    PATCH_LWORD,
    PATCH_BWORD,
    PATCH_LLONG,
    PATCH_BLONG,
} EPatchType;

typedef struct Patch {
    list_Data(struct Patch);
    struct Section* pSection;
    uint32_t Offset;
    EPatchType Type;
    SExpression* pExpression;
    string* pFile;
    int nLine;
} SPatch;

#include "section.h"
#include "symbol.h"

extern void
patch_Create(SSection* sect, uint32_t offset, SExpression* expr, EPatchType type);

extern void
patch_BackPatch(void);

extern void
patch_OptimizeAll(void);

extern bool
patch_GetImportOffset(uint32_t* pOffset, SSymbol** ppSym, SExpression* pExpr);

#endif    /*XASM_COMMON_PATCH_H_INCLUDED_*/
