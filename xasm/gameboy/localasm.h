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

#ifndef	LOCALASM_H
#define	LOCALASM_H

typedef enum
{
	MERROR_EXPECT_A = 1000,
	MERROR_EXPECT_SP,
	MERROR_SUGGEST_OPCODE,
	MERROR_EXPRESSION_FF00,
	MERROR_INSTRUCTION_NOT_SUPPORTED_BY_CPU
} EMachineError;

typedef	enum
{
	T_Z80_ADC = 6000,
	T_Z80_ADD,
	T_Z80_AND,
	T_Z80_BIT,
	T_Z80_CALL,
	T_Z80_CCF,
	T_Z80_CP,
	T_Z80_CPD,	// Z80 only
	T_Z80_CPDR,	// Z80 only
	T_Z80_CPI,	// Z80 only
	T_Z80_CPIR,	// Z80 only
	T_Z80_CPL,
	T_Z80_DAA,
	T_Z80_DEC,
	T_Z80_DI,
	T_Z80_DJNZ,	// Z80 only
	T_Z80_EI,
	T_Z80_EX,	// Z80 only
	T_Z80_EXX,	// Z80 only
	T_Z80_HALT,
	T_Z80_IM,	// Z80 only
	T_Z80_IN,	// Z80 only
	T_Z80_INC,
	T_Z80_IND,	// Z80 only
	T_Z80_INDR,	// Z80 only
	T_Z80_INI,	// Z80 only
	T_Z80_INIR,	// Z80 only
	T_Z80_JP,
	T_Z80_JR,
	T_Z80_LD,
	T_Z80_LDD,
	T_Z80_LDDR,	// Z80 only
	T_Z80_LDI,
	T_Z80_LDIR,	// Z80 only
	T_Z80_LDH,	// GB only
	T_Z80_LDHL,	// GB only
	T_Z80_NEG,	// Z80 only
	T_Z80_NOP,
	T_Z80_OR,
	T_Z80_OTDR,	// Z80 only
	T_Z80_OTIR,	// Z80 only
	T_Z80_OUT,	// Z80 only
	T_Z80_OUTD,	// Z80 only
	T_Z80_OUTI,	// Z80 only
	T_Z80_POP,
	T_Z80_PUSH,
	T_Z80_RES,
	T_Z80_RET,
	T_Z80_RETI,
	T_Z80_RETN,	// Z80 only
	T_Z80_RL,
	T_Z80_RLA,
	T_Z80_RLC,
	T_Z80_RLCA,
	T_Z80_RLD,	// Z80 only
	T_Z80_RR,
	T_Z80_RRA,
	T_Z80_RRC,
	T_Z80_RRCA,
	T_Z80_RRD,	// Z80 only
	T_Z80_RST,
	T_Z80_SBC,
	T_Z80_SCF,

// Handled by globallex.c
// "set"        ,       T_SYM_SET,
	T_Z80_SET,

	T_Z80_SLA,
	T_Z80_SLL,	// Z80 only
	T_Z80_SRA,
	T_Z80_SRL,
	T_Z80_STOP,	// GB only
	T_Z80_SUB,
	T_Z80_SWAP,	// GB only
	T_Z80_XOR,

	T_MODE_B,
	T_MODE_C,
	T_MODE_D,
	T_MODE_E,
	T_MODE_H,
	T_MODE_L,
	T_MODE_HL_IND,
	T_MODE_A,

	T_MODE_BC,
	T_MODE_DE,
	T_MODE_HL,
	T_MODE_SP,
	T_MODE_IX,	// Z80 only
	T_MODE_IY,	// Z80 only

	T_MODE_C_IND,
	T_MODE_GB_C_IND,
	T_MODE_SP_IND,
	T_MODE_BC_IND,
	T_MODE_DE_IND,

	T_MODE_HL_INDINC,	// GB only
	T_MODE_HL_INDDEC,	// GB only

	T_MODE_AF,
	T_MODE_AF_ALT,

	T_MODE_I,
	T_MODE_R,

	T_CC_NZ,
	T_CC_Z,
	T_CC_NC,
	//T_CC_C,		// handled by C register token
	T_CC_PO,	// Z80 only
	T_CC_PE,	// Z80 only
	T_CC_P,		// Z80 only
	T_CC_M,		// Z80 only


}	ETargetToken;

#endif	//LOCALASM_H
