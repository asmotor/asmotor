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

#ifndef XASM_M68K_ERRORS_H_INCLUDED_
#define XASM_M68K_ERRORS_H_INCLUDED_

#include <types.h>

typedef enum {
    MERROR_UNDEFINED_RESULT = 1000,
    MERROR_IGNORING_SIZE,
    MERROR_SCALE_RANGE,
    MERROR_INDEXREG_SIZE,
    MERROR_DISP_SIZE,
    MERROR_INSTRUCTION_SIZE,
    MERROR_INSTRUCTION_CPU,
    MERROR_EXPECT_BITFIELD,
    MERROR_INSTRUCTION_PRIV,
    MERROR_MOVEM_SKIPPED,
    MERROR_MISALIGNED_FAIL_68060,
    MERROR_FPU_REGISTER_EXPECTED,
    MERROR_INSTRUCTION_FPU,
    MERROR_FPU_NEEDS_020_030,
    MERROR_FPU_NEEDS_040,
    MERROR_FPU_NEEDS_060,
} EMachineError;

extern const char*
m68k_GetError(size_t errorNumber);

#endif
