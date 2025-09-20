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

#ifndef XASM_MOTOR_PARSE_SYMBOL_H_INCLUDED_
#define XASM_MOTOR_PARSE_SYMBOL_H_INCLUDED_

#include <stdbool.h>
#include <stdint.h>

#include "symbol.h"

extern SSymbol* g_rsSymbol;

extern int32_t
parse_IncrementRs(int32_t size);

extern void
parse_SetRs(int32_t rsValue);

extern bool
parse_SymbolDefinition(void);

#endif // XASM_MOTOR_PARSE_SYMBOL_H_INCLUDED_
