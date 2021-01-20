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

#include "xasm.h"
#include "symbol.h"

static void
createGroup(const char* name, EGroupType type, uint32_t flags) {
    string* nameString = str_Create(name);
    sym_CreateGroup(nameString, type)->flags |= flags;
    str_Free(nameString);
}

void
m68k_DefineSymbols(void) {
    createGroup("CODE", GROUP_TEXT, 0);
    createGroup("DATA", GROUP_TEXT, SYMF_DATA);
    createGroup("BSS", GROUP_BSS, 0);
    createGroup("CODE_C", GROUP_TEXT, SYMF_SHARED);
    createGroup("DATA_C", GROUP_TEXT, SYMF_DATA | SYMF_SHARED);
    createGroup("BSS_C", GROUP_BSS, SYMF_SHARED);
}
