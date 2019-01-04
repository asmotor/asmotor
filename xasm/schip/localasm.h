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

#ifndef	XASM_GAMEBOY_TOKENS_H_INCLUDED_
#define	XASM_GAMEBOY_TOKENS_H_INCLUDED_

typedef enum
{
	MERROR_UNDEFINED_RESULT = 1000,
	MERROR_REGISTER_EXPECTED
} EMachineError;

typedef	enum
{
	/* reg */
	T_CHIP_BCD = 6000,
	T_CHIP_LDF,
	T_CHIP_LDF10,
	T_CHIP_SHL,
	T_CHIP_SKNP,
	T_CHIP_SKP,
	T_CHIP_SHR,
	T_CHIP_WKP,

	/* reg, reg */
	T_CHIP_AND,
	T_CHIP_OR,
	T_CHIP_SUB,
	T_CHIP_SUBN,
	T_CHIP_XOR,

	/* reg, reg, i4 */
	T_CHIP_DRW,

	/* reg|i|dt|st, reg|dt|i8|i12 */
	T_CHIP_LD,

	/* (i)|reg, reg|(i) */
	T_CHIP_LDM,

	/* reg|i, reg|i8 */
	T_CHIP_ADD,

	/* reg, reg|i8 */
	T_CHIP_SE,
	T_CHIP_SNE,

	/* reg, i8 */
	T_CHIP_RND,

	/* i4 */
	T_CHIP_SCRD,

	/* i12|v0+i12 */
	T_CHIP_JP,

	/* i12 */
	T_CHIP_CALL,

	/* No parameter: */
	T_CHIP_CLS,
	T_CHIP_EXIT,
	T_CHIP_LO,
	T_CHIP_HI,
	T_CHIP_RET,
	T_CHIP_SCRR,
	T_CHIP_SCRL,

	T_CHIP_INSTR_FIRST = T_CHIP_BCD,
	T_CHIP_INSTR_LAST = T_CHIP_SCRL,

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
	T_CHIP_REG_DT,
	T_CHIP_REG_ST,
	T_CHIP_REG_I,
	T_CHIP_REG_I_IND,
	T_CHIP_REG_RPL,

}	ETargetToken;

#endif	//XASM_GAMEBOY_TOKENS_H_INCLUDED_
