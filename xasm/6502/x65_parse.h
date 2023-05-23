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

#ifndef XASM_6502_PARSE_H_INCLUDED_
#define XASM_6502_PARSE_H_INCLUDED_

#include "expression.h"

#define MODE_NONE		0x00001u
#define MODE_IMM		0x00002u		/* #n8 */
#define MODE_ZP			0x00004u		/* n8 */
#define MODE_ZP_X		0x00008u		/* n8,x */
#define MODE_ZP_Y		0x00010u		/* n8,y */
#define MODE_ABS		0x00020u		/* n16 */
#define MODE_ABS_X		0x00040u		/* n16,x */
#define MODE_ABS_Y		0x00080u		/* n16,y */
#define MODE_IND_ZP_X	0x00100u		/* (n8,x) */
#define MODE_IND_ZP_Y	0x00200u		/* (n8),y */
#define MODE_A			0x00400u		/* a */
#define MODE_IND_ABS	0x00800u		/* (n16) */
#define MODE_ZP_ABS		0x01000u		/* n8,n16 */
#define MODE_BIT_ZP		0x02000u		/* n3,n8 */
#define MODE_BIT_ZP_ABS	0x04000u		/* n8,n8,n16 */
#define MODE_IND_ZP		0x08000u		/* (n8) */
#define MODE_IND_ABS_X	0x10000u		/* (n16,x) */

#define MODE_65C02		(MODE_ZP_ABS | MODE_BIT_ZP | MODE_BIT_ZP_ABS | MODE_IND_ZP | MODE_IND_ABS_X)

typedef struct {
	uint32_t mode;
	SExpression* expr;
	SExpression* expr2;
	SExpression* expr3;
} SAddressingMode;

extern bool
x65_ParseAddressingMode(SAddressingMode* addrMode, uint32_t allowedModes);

extern bool
x65_ParseIntegerInstruction(void);

extern SExpression*
x65_ParseExpressionSU8(void);

extern SExpression*
x65_ParseFunction(void);

extern bool
x65_ParseInstruction(void);

extern bool
x65_Parse65816Instruction(void);

#endif
