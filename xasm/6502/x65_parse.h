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

#include <stdbool.h>

#include "expression.h"
#include "x65_tokens.h"

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

// 65C02
#define MODE_ZP_ABS		0x01000u		/* n8,n16 */
#define MODE_BIT_ZP		0x02000u		/* n3,n8 */
#define MODE_BIT_ZP_ABS	0x04000u		/* n8,n8,n16 */
#define MODE_IND_ZP		0x08000u		/* (n8) */
#define MODE_IND_ABS_X	0x10000u		/* (n16,x) */
#define MODE_IMM_IMM	0x20000u		/* #n8,#n8 */

#define MODE_816_DISP_S			0x00040000u		/* n8,S */
#define MODE_816_LONG_IND_ZP	0x00080000u		/* [n8] */
#define MODE_816_LONG_ABS		0x00100000u		/* n24 */
#define MODE_816_IND_DISP_S_Y	0x00200000u		/* (n8,S),Y */
#define MODE_816_LONG_IND_ZP_Y	0x00400000u		/* [n8],y */
#define MODE_816_LONG_ABS_X		0x00800000u		/* n24,x */
#define MODE_816_LONG_IND_ABS	0x01000000u		/* [n16] */

#define MODE_4510_IND_ZP_Z		0x02000000u	/* (n8),z */
#define MODE_4510_ABS_X			0x04000000u	/* n16,x */
#define MODE_4510_ABS_Y			0x08000000u	/* n16,y */

#define MODE_45GS02_IND_ZP_Z_QUAD	0x10000000u	/* [n8],z */
#define MODE_45GS02_IND_ZP_QUAD		0x20000000u	/* [n8] */
#define MODE_45GS02_Q				0x40000000u /* q */

#define MODE_6502	(MODE_NONE | MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ZP_Y | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_ZP_X | MODE_IND_ZP_Y | MODE_A | MODE_IND_ABS)
#define MODE_65C02	(MODE_6502 | MODE_IND_ZP | MODE_IND_ABS_X)
#define MODE_65C02S	(MODE_65C02 | MODE_ZP_ABS | MODE_BIT_ZP | MODE_BIT_ZP_ABS)
#define MODE_65816	(MODE_65C02 | MODE_816_DISP_S | MODE_816_LONG_IND_ZP | MODE_816_LONG_ABS | MODE_816_IND_DISP_S_Y | MODE_816_LONG_IND_ZP_Y | MODE_816_LONG_ABS_X | MODE_816_LONG_IND_ABS)
#define MODE_4510	(MODE_6502 | MODE_IND_ABS_X | MODE_ZP_ABS | MODE_BIT_ZP | MODE_BIT_ZP_ABS | MODE_4510_IND_ZP_Z | MODE_816_IND_DISP_S_Y | MODE_4510_ABS_X | MODE_4510_ABS_Y)
#define MODE_45GS02	(MODE_4510 | MODE_45GS02_IND_ZP_Z_QUAD | MODE_45GS02_IND_ZP_QUAD | MODE_45GS02_Q)

typedef struct {
	uint32_t mode;
	SExpression* expr;
	SExpression* expr2;
	SExpression* expr3;
	bool size_forced;
} SAddressingMode;

typedef enum {
	IMM_NONE,
	IMM_8_BIT,
	IMM_16_BIT,
	IMM_32_BIT,
	IMM_ACC,
	IMM_INDEX
} EImmediateSize;


extern void
x65_OutputLongInstruction(uint8_t opcode, SExpression* expr);

extern bool
x65_ParseAddressingMode(SAddressingMode* addrMode, uint32_t allowedModes, EImmediateSize immSize);

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

extern bool
x65_Parse4510Instruction(void);

extern bool
x65_HandleToken(ETargetToken token, uint32_t allowedModes);

extern bool
x65_HandleTokenAddressMode(ETargetToken token, SAddressingMode* addrMode);

extern void
x65_OutputSU16Expression(SExpression* expr);

extern void
x65_OutputU16Expression(SExpression* expr);

extern void
x65_OutputSU8Expression(SExpression* expr);

extern void
x65_OutputU8Expression(SExpression* expr);


#endif
