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
createGroup(const char* name, EGroupType type) {
    string* nameString = str_Create(name);
    sym_CreateGroup(nameString, type);
    str_Free(nameString);
}

void
x10c_DefineSymbols(void) {
	if (opt_Current->createGroups) {
		createGroup("CODE", GROUP_TEXT);
		createGroup("DATA", GROUP_TEXT);
		createGroup("BSS", GROUP_BSS);
	}
}

bool
x10c_IsValidLocalName(const string* name) {
	return true;
}
