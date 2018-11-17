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

#ifndef	LOCALASM_0X10C_H_
#define	LOCALASM_0X10C_H_

typedef enum
{
	MERROR_ILLEGAL_ADDRMODE = 1000,
	MERROR_ADDRMODE_ONE_REGISTER,
	MERROR_ADDRMODE_SUBTRACT_REGISTER,
} EMachineError;

typedef	enum
{
	T_0X10C_ADD = 6000,
	T_0X10C_AND,
	T_0X10C_BOR,
	T_0X10C_DIV,
	T_0X10C_IFB,
	T_0X10C_IFE,
	T_0X10C_IFG,
	T_0X10C_IFN,
	T_0X10C_JSR,
	T_0X10C_MOD,
	T_0X10C_MUL,
	T_0X10C_SET,
	T_0X10C_SHL,
	T_0X10C_SHR,
	T_0X10C_SUB,
	T_0X10C_XOR,

	T_REG_A,
	T_REG_B,
	T_REG_C,
	T_REG_X,
	T_REG_Y,
	T_REG_Z,
	T_REG_I,
	T_REG_J,
	T_REG_POP,
	T_REG_PEEK,
	T_REG_PUSH,
	T_REG_SP,
	T_REG_PC,
	T_REG_O,

}	ETargetToken;

#endif	/* LOCALASM_H */
