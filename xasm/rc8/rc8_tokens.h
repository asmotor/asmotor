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

#ifndef XASM_RC8_TOKENS_H_INCLUDED_
#define XASM_RC8_TOKENS_H_INCLUDED_

typedef	enum {
	T_RC8_ADD = 6000,
	T_RC8_AND,
	T_RC8_CMP,
	T_RC8_DI,
	T_RC8_DJ,
	T_RC8_EI,
	T_RC8_EXG,
	T_RC8_EXT,
	T_RC8_J,
	T_RC8_JAL,
	T_RC8_LCO,
	T_RC8_LCR,
	T_RC8_LD,
	T_RC8_LIO,
	T_RC8_LS,
	T_RC8_NEG,
	T_RC8_NOP,
	T_RC8_NOT,
	T_RC8_OR,
	T_RC8_POP,
	T_RC8_POPA,
	T_RC8_PUSH,
	T_RC8_PUSHA,
	T_RC8_RETI,
	T_RC8_RS,
	T_RC8_RSA,
	T_RC8_SUB,
	T_RC8_SYS,
	T_RC8_TST,
	T_RC8_XOR,

	/* Registers */
	
	T_RC8_REG_F,
	T_RC8_REG_T,
	T_RC8_REG_B,
	T_RC8_REG_C,
	T_RC8_REG_D,
	T_RC8_REG_E,
	T_RC8_REG_H,
	T_RC8_REG_L,
	T_RC8_REG_FT,
	T_RC8_REG_BC,
	T_RC8_REG_DE,
	T_RC8_REG_HL,
	T_RC8_REG_C_IND,
	T_RC8_REG_FT_IND,
	T_RC8_REG_BC_IND,
	T_RC8_REG_DE_IND,
	T_RC8_REG_HL_IND,

	/* Conditions */
	T_RC8_CC_LE,
	T_RC8_CC_GT,
	T_RC8_CC_LT,
	T_RC8_CC_GE,
	T_RC8_CC_LEU,
	T_RC8_CC_GTU,
	T_RC8_CC_LTU,
	T_RC8_CC_GEU,
	T_RC8_CC_EQ,
	T_RC8_CC_NE,
} ETargetToken;

extern void 
rc8_DefineTokens(void);

#endif
