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


#ifndef XASM_GAMEBOY_ERRORS_H_INCLUDED_
#define XASM_GAMEBOY_ERRORS_H_INCLUDED_

#include <stdint.h>

typedef enum {
    MERROR_EXPECT_A = 1000,
    MERROR_EXPECT_SP,
    MERROR_SUGGEST_OPCODE,
    MERROR_EXPRESSION_FF00,
    MERROR_INSTRUCTION_NOT_SUPPORTED_BY_CPU
} EMachineError;

extern const char*
loc_GetError(uint32_t errorNumber);

#endif //XASM_GAMEBOY_ERRORS_H_INCLUDED_
