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

#ifndef XASM_6809_TOKENS_H_INCLUDED_
#define XASM_6809_TOKENS_H_INCLUDED_

#include "lexer_constants.h"

typedef enum {
    T_6809_NOP = 6000,

    /* Registers */

    T_6809_REG_A,
    T_6809_REG_X,
    T_6809_REG_Y,

} ETargetToken;

extern SLexConstantsWord*
m6809_GetUndocumentedInstructions(int n);

extern void
m6809_DefineTokens(void);

#endif
