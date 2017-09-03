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

#include "symbol.h"
#include "xasm.h"

void locsym_Init(void)
{
	string* pName;
	
	pName = str_Create("HOME");
	sym_CreateGROUP(pName, GROUP_TEXT);
	str_Free(pName);
	
	pName = str_Create("CODE");
	sym_CreateGROUP(pName, GROUP_TEXT);
	str_Free(pName);
	
	pName = str_Create("DATA");
	sym_CreateGROUP(pName, GROUP_TEXT);
	str_Free(pName);
	
	pName = str_Create("BSS");
	sym_CreateGROUP(pName, GROUP_BSS);
	str_Free(pName);
	
	pName = str_Create("HRAM");
	sym_CreateGROUP(pName, GROUP_BSS);
	str_Free(pName);
	
	pName = str_Create("VRAM");
	sym_CreateGROUP(pName, GROUP_BSS);
	str_Free(pName);
}
