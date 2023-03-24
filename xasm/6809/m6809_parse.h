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

#ifndef XASM_6809_PARSE_H_INCLUDED_
#define XASM_6809_PARSE_H_INCLUDED_

#include "expression.h"

typedef enum {
	OFFSET_NONE,
	OFFSET_5BIT,
	OFFSET_8BIT,
	OFFSET_16BIT,
} EIndexedOffset;

#define MODE_NONE				0x001u
#define MODE_IMMEDIATE			0x002u		// #address
#define MODE_ADDRESS			0x004u		// relative, extended, direct
#define MODE_EXTENDED			0x008u		// extended (or forced ">address") (+indirect)
#define MODE_DIRECT				0x010u		// direct (or forced "<address")
#define MODE_INDEXED_R_5BIT		0x020u		// off5,R
#define MODE_INDEXED_R_INC1		0x040u		// ,R+
#define MODE_INDEXED_R_INC2		0x080u		// ,R++ (+indirect)
#define MODE_INDEXED_R_DEC1		0x100u		// ,-R
#define MODE_INDEXED_R_DEC2		0x200u		// ,--R (+indirect)
#define MODE_INDEXED_R			0x400u		// ,R (+indirect)
#define MODE_INDEXED_R_A		0x800u		// A,R (+indirect)
#define MODE_INDEXED_R_B		0x1000u		// B,R (+indirect)
#define MODE_INDEXED_R_8BIT		0x2000u		// off8,R (+indirect)
#define MODE_INDEXED_R_16BIT	0x4000u		// off16,R (+indirect)
#define MODE_INDEXED_R_D		0x8000u		// D,R (+indirect)
#define MODE_INDEXED_PC_8BIT	0x10000u	// off8,PC (+indirect)
#define MODE_INDEXED_PC_16BIT	0x20000u	// off16,PC (+indirect)
#define MODE_EXTENDED_INDIRECT	0x40000u	// [address] (always indirect)
#define MODE_INDIRECT_MODIFIER	0x80000u
#define MODE_REGISTER_LIST_S	0x100000u
#define MODE_REGISTER_LIST_U	0x200000u

#define MODE_ALL_INDEXED	(MODE_INDEXED_R_5BIT | MODE_INDEXED_R_INC1 | MODE_INDEXED_R_INC2 | MODE_INDEXED_R_DEC1 | MODE_INDEXED_R_DEC2 | MODE_INDEXED_R | MODE_INDEXED_R_A | MODE_INDEXED_R_B | MODE_INDEXED_R_8BIT | MODE_INDEXED_R_16BIT | MODE_INDEXED_R_D | MODE_INDEXED_PC_8BIT | MODE_INDEXED_PC_16BIT | MODE_EXTENDED_INDIRECT | MODE_INDIRECT_MODIFIER)
#define MODE_ALLOWED_INDIRECT	(MODE_ADDRESS | MODE_EXTENDED | MODE_INDEXED_R_INC2 | MODE_INDEXED_R_DEC2 | MODE_INDEXED_R | MODE_INDEXED_R_A | MODE_INDEXED_R_B | MODE_INDEXED_R_8BIT | MODE_INDEXED_R_16BIT | MODE_INDEXED_R_D | MODE_INDEXED_PC_8BIT | MODE_INDEXED_PC_16BIT)

typedef struct {
	uint32_t mode;
	uint8_t indexed_post_byte;
	EIndexedOffset offset_kind;
	SExpression* expr;
} SAddressingMode;

extern bool
m6809_ParseAddressingMode(SAddressingMode* addrMode, uint32_t allowedModes);

extern bool
m6809_ParseIntegerInstruction(void);

extern SExpression*
m6809_ParseExpressionSU8(void);

extern SExpression*
m6809_ParseFunction(void);

extern bool
m6809_ParseInstruction(void);

#endif
