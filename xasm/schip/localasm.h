/*  Copyright 2008 Carsten Sørensen

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

#ifndef	LOCALASM_H
#define	LOCALASM_H

typedef enum
{
	MERROR_UNDEFINED_RESULT = 1000,
	MERROR_REGISTER_EXPECTED
} EMachineError;

typedef	enum
{
	T_CHIP_ADD = 6000,

	/* No parameter: */
	
	T_CHIP_RET,

	T_MIPS_INTEGER_NO_PARAMETER_FIRST = T_CHIP_RET,
	T_MIPS_INTEGER_NO_PARAMETER_LAST = T_CHIP_RET,

	/* Registers */

	T_CHIP_REG_V0,
	T_CHIP_REG_V1,
	T_CHIP_REG_V2,
	T_CHIP_REG_V3,
	T_CHIP_REG_V4,
	T_CHIP_REG_V5,
	T_CHIP_REG_V6,
	T_CHIP_REG_V7,
	T_CHIP_REG_V8,
	T_CHIP_REG_V9,
	T_CHIP_REG_V10,
	T_CHIP_REG_V11,
	T_CHIP_REG_V12,
	T_CHIP_REG_V13,
	T_CHIP_REG_V14,
	T_CHIP_REG_V15,

}	eTargetToken;

#endif	//LOCALASM_H
