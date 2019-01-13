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

#ifndef XASM_6502_LOCERROR_H_INCLUDED_
#define XASM_6502_LOCERROR_H_INCLUDED_

#include "xasm.h"

typedef enum
{
	MERROR_ILLEGAL_ADDRMODE = 1000,
	ERROR_MACHINE_OPTION_UNDOCUMENTED_RANGE
} EMachineError;

extern const char* loc_GetError(size_t errorNumber);

#endif
