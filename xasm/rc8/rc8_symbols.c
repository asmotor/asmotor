/*  Copyright 2008-2022 Carsten Elton Sorensen and contributors

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
    string* nameStr = str_Create(name);
    sym_CreateGroup(nameStr, type)->flags |= flags;
    str_Free(nameStr);
}


void 
rc8_DefineSymbols(void) {
	createGroup("CODE", GROUP_TEXT, 0);
	createGroup("DATA", GROUP_TEXT, SYMF_DATA);
	createGroup("DATA_S", GROUP_TEXT, SYMF_DATA|SYMF_SHARED);
	createGroup("BSS", GROUP_BSS, 0);
    createGroup("BSS_S", GROUP_BSS, SYMF_SHARED);
}

bool
rc8_IsValidLocalName(const string* name) {
	return true;
}

