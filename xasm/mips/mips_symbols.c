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
createGroup(const char* name, EGroupType type) {
    string* nameStr = str_Create(name);
    sym_CreateGroup(nameStr, type);
    str_Free(nameStr);
}

void
mips_DefineSymbols() {
    createGroup("CODE", GROUP_TEXT);
    createGroup("DATA", GROUP_TEXT);
    createGroup("BSS", GROUP_BSS);
}
