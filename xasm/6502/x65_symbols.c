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
#include "options.h"
#include "symbol.h"

#include "x65_options.h"


static int32_t
getMWidth(SSymbol* symbol) {
	return opt_Current->machineOptions->m16 ? 16 : 8;
}


static int32_t
getXWidth(SSymbol* symbol) {
	return opt_Current->machineOptions->x16 ? 16 : 8;
}


static int32_t
getBP(SSymbol* symbol) {
	return opt_Current->machineOptions->bp_base;
}


static void
createEquCallback(const char* name, int32_t (*callback)(SSymbol*)) {
    string* nameStr = str_Create(name);
	SSymbol* symbol = sym_CreateEqu(nameStr, 0);
	symbol->callback.integer = callback;
    str_Free(nameStr);
}


static void
createGroup(const char* name, EGroupType type) {
    string* nameStr = str_Create(name);
    sym_CreateGroup(nameStr, type);
    str_Free(nameStr);
}


void
x65_DefineSymbols(void) {
    createGroup("CODE", GROUP_TEXT);
    createGroup("DATA", GROUP_TEXT);
    createGroup("BSS", GROUP_BSS);
    createGroup("ZP", GROUP_BSS);
	createEquCallback("__816_M", getMWidth);
	createEquCallback("__816_X", getXWidth);
	createEquCallback("__4510_BP", getBP);
}


bool
x65_IsValidLocalName(const string* name) {
	return true;
}
