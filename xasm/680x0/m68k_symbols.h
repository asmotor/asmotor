/*  Copyright 2008-2021 Carsten Elton Sorensen and contributors

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

#ifndef XASM_M68K_SYMBOLS_H_INCLUDED_
#define XASM_M68K_SYMBOLS_H_INCLUDED_

#include "m68k_options.h"

extern void
m68k_DefineSymbols(void);

extern void
m68k_DefineMachineGroups(EPlatform68k platform);

extern void
m68k_AddRegmask(uint16_t registers);

extern void
m68k_ResetRegmask(void);

#endif
