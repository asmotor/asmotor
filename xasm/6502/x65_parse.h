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

#ifndef XASM_6502_PARSE_H_INCLUDED_
#define XASM_6502_PARSE_H_INCLUDED_

#include "expression.h"

#define MODE_NONE	0x001u
#define MODE_IMM	0x002u
#define MODE_ZP		0x004u
#define MODE_ZP_X	0x008u
#define MODE_ZP_Y	0x010u
#define MODE_ABS	0x020u
#define MODE_ABS_X	0x040u
#define MODE_ABS_Y	0x080u
#define MODE_IND_X	0x100u
#define MODE_IND_Y	0x200u
#define MODE_A		0x400u
#define MODE_IND	0x800u

typedef struct {
	uint16_t mode;
	SExpression* expr;
} SAddressingMode;

extern bool
parse_AddressingMode(SAddressingMode* addrMode, uint32_t allowedModes);

extern bool
parse_IntegerInstruction(void);

extern SExpression*
parse_ExpressionSU8(void);

extern SExpression*
x65_ParseFunction(void);

extern bool
x65_ParseInstruction(void);

#endif
