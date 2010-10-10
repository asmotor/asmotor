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

#include "xasm.h"
#include "symbol.h"
#include "localasm.h"

void locsym_Init(void)
{
	sym_AddGROUP("CODE", GROUP_TEXT);
	sym_AddGROUP("DATA", GROUP_TEXT)->Flags |= LOCSYMF_DATA;
	sym_AddGROUP("BSS", GROUP_BSS);
	sym_AddGROUP("CODE_C", GROUP_TEXT)->Flags |= LOCSYMF_CHIP;
	sym_AddGROUP("DATA_C", GROUP_TEXT)->Flags |= LOCSYMF_DATA | LOCSYMF_CHIP;
	sym_AddGROUP("BSS_C", GROUP_BSS)->Flags |= LOCSYMF_CHIP;
}
