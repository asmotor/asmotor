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

#ifndef XASM_V_TOKENS_H_INCLUDED_
#define XASM_V_TOKENS_H_INCLUDED_

typedef	enum {
	T_V_ADD = 6000,

	/* Registers */
	
	T_V_REG_X0,
	T_V_REG_X1,
	T_V_REG_X2,
	T_V_REG_X3,
	T_V_REG_X4,
	T_V_REG_X5,
	T_V_REG_X6,
	T_V_REG_X7,
	T_V_REG_X8,
	T_V_REG_X9,
	T_V_REG_X10,
	T_V_REG_X11,
	T_V_REG_X12,
	T_V_REG_X13,
	T_V_REG_X14,
	T_V_REG_X15,
	T_V_REG_X16,
	T_V_REG_X17,
	T_V_REG_X18,
	T_V_REG_X19,
	T_V_REG_X20,
	T_V_REG_X21,
	T_V_REG_X22,
	T_V_REG_X23,
	T_V_REG_X24,
	T_V_REG_X25,
	T_V_REG_X26,
	T_V_REG_X27,
	T_V_REG_X28,
	T_V_REG_X29,
	T_V_REG_X30,
	T_V_REG_X31
} ETargetToken;

extern void 
v_DefineTokens(void);

#endif
