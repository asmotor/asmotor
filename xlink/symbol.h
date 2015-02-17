/*  Copyright 2008-2015 Carsten Elton Sorensen

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

#ifndef	XLINK_SYMBOL_H_INCLUDED_
#define	XLINK_SYMBOL_H_INCLUDED_

typedef	enum
{
	SYM_EXPORT,
	SYM_IMPORT,
	SYM_LOCAL,
	SYM_LOCALEXPORT,
	SYM_LOCALIMPORT
} SymbolType;

typedef	struct Symbol_
{
	char       name[MAXSYMNAMELENGTH];
	SymbolType type;
	int32_t    value;
	bool_t     resolved;

	struct Section_* section;
} Symbol;

static inline bool_t symbol_IsLocal(Symbol* symbol)
{
    switch (symbol->type)
    {
        case SYM_LOCAL:
        case SYM_LOCALEXPORT:
        case SYM_LOCALIMPORT:
            return true;
        case SYM_EXPORT:
        case SYM_IMPORT:
            return false;
    }
}

#endif
