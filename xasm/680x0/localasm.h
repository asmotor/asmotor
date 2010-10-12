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

#define MAXSECTIONSIZE 0xFFFFFFFF

#define LOCASM_CPUID "680x0"
#define	LOCASM_VERSION	"0.5"

#define	ASM_DEFAULT_ENDIAN	ASM_BIG_ENDIAN

#define	EXENAME			"motor68k"

#define	NAME_DB			"dc.b"
#define	NAME_DW			"dc.w"
#define	NAME_DL			"dc.l"
#define	NAME_RB			"rs.b"
#define	NAME_RW			"rs.w"
#define	NAME_RL			"rs.l"
#define	NAME_DSB		"ds.b"
#define	NAME_DSW		"ds.w"
#define	NAME_DSL		"ds.l"

typedef enum
{
	MERROR_UNDEFINED_RESULT = 1000,
	MERROR_IGNORING_SIZE,
	MERROR_SCALE_RANGE,
	MERROR_INDEXREG_SIZE,
	MERROR_DISP_SIZE,
	MERROR_INSTRUCTION_SIZE,
	MERROR_INSTRUCTION_CPU,
	MERROR_EXPECT_BITFIELD,
	MERROR_INSTRUCTION_PRIV,
	MERROR_MOVEM_SKIPPED
} EMachineError;

typedef	enum
{
	T_68K_ABCD = 6000,
	T_68K_ADD,
	T_68K_ADDA,
	T_68K_ADDI,
	T_68K_ADDQ,
	T_68K_ADDX,
	T_68K_AND,
	T_68K_ANDI,
	T_68K_ASL,
	T_68K_ASR,

	// Bcc start
	T_68K_BCC,
	T_68K_BCS,
	T_68K_BEQ,
	T_68K_BGE,
	T_68K_BGT,
	T_68K_BHI,
	T_68K_BLE,
	T_68K_BLS,
	T_68K_BLT,
	T_68K_BMI,
	T_68K_BNE,
	T_68K_BPL,
	T_68K_BVC,
	T_68K_BVS,
	// Bcc end

	T_68K_BCHG,
	T_68K_BCLR,
	T_68K_BFCHG,
	T_68K_BFCLR,
	T_68K_BFEXTS,
	T_68K_BFEXTU,
	T_68K_BFFFO,
	T_68K_BFINS,
	T_68K_BFSET,
	T_68K_BFTST,
	T_68K_BKPT,
	T_68K_BRA,
	T_68K_BSET,
	T_68K_BSR,
	T_68K_BTST,
	T_68K_CALLM,
	T_68K_CAS,
	T_68K_CAS2,
	T_68K_CHK,
	T_68K_CHK2,
	T_68K_CINVA,
	T_68K_CINVL,
	T_68K_CINVP,
	T_68K_CLR,
	T_68K_CMP,
	T_68K_CMPA,
	T_68K_CMPI,
	T_68K_CMPM,
	T_68K_CMP2,
	T_68K_CPUSHA,
	T_68K_CPUSHL,
	T_68K_CPUSHP,

	// DBcc start
	T_68K_DBCC,
	T_68K_DBCS,
	T_68K_DBEQ,
	T_68K_DBF,
	T_68K_DBGE,
	T_68K_DBGT,
	T_68K_DBHI,
	T_68K_DBLE,
	T_68K_DBLS,
	T_68K_DBLT,
	T_68K_DBMI,
	T_68K_DBNE,
	T_68K_DBPL,
	T_68K_DBT,
	T_68K_DBVC,
	T_68K_DBVS,
	// DBcc end

	T_68K_DIVS,
	T_68K_DIVSL,
	T_68K_DIVU,
	T_68K_DIVUL,
	T_68K_EOR,
	T_68K_EORI,
	T_68K_EXG,
	T_68K_EXT,
	T_68K_EXTB,
	T_68K_ILLEGAL,
	T_68K_JMP,
	T_68K_JSR,
	T_68K_LEA,
	T_68K_LINK,
	T_68K_LSL,
	T_68K_LSR,
	T_68K_MOVE,
	T_68K_MOVEA,
	T_68K_MOVEC,
	T_68K_MOVE16,
	T_68K_MOVEM,
	T_68K_MOVEP,
	T_68K_MOVEQ,
	T_68K_MOVES,
	T_68K_MULS,
	T_68K_MULU,
	T_68K_NBCD,
	T_68K_NEG,
	T_68K_NEGX,
	T_68K_NOP,
	T_68K_NOT,
	T_68K_OR,
	T_68K_ORI,
	T_68K_PACK,
	T_68K_PEA,
	T_68K_RESET,
	T_68K_ROL,
	T_68K_ROR,
	T_68K_ROXL,
	T_68K_ROXR,
	T_68K_RTD,
	T_68K_RTE,
	T_68K_RTM,
	T_68K_RTR,
	T_68K_RTS,
	T_68K_SBCD,

	// Scc start
	T_68K_SCC,
	T_68K_SCS,
	T_68K_SEQ,
	T_68K_SF,
	T_68K_SGE,
	T_68K_SGT,
	T_68K_SHI,
	T_68K_SLE,
	T_68K_SLS,
	T_68K_SLT,
	T_68K_SMI,
	T_68K_SNE,
	T_68K_SPL,
	T_68K_ST,
	T_68K_SVC,
	T_68K_SVS,
	// Scc end

	T_68K_STOP,
	T_68K_SUB,
	T_68K_SUBA,
	T_68K_SUBI,
	T_68K_SUBQ,
	T_68K_SUBX,
	T_68K_SWAP,
	T_68K_TAS,
	T_68K_TRAP,

	// TRAPcc start
	T_68K_TRAPCC,
	T_68K_TRAPCS,
	T_68K_TRAPEQ,
	T_68K_TRAPF,
	T_68K_TRAPGE,
	T_68K_TRAPGT,
	T_68K_TRAPHI,
	T_68K_TRAPLE,
	T_68K_TRAPLS,
	T_68K_TRAPLT,
	T_68K_TRAPMI,
	T_68K_TRAPNE,
	T_68K_TRAPPL,
	T_68K_TRAPT,
	T_68K_TRAPVC,
	T_68K_TRAPVS,
	// TRAPcc end

	T_68K_TRAPV,
	T_68K_TST,
	T_68K_UNLK,
	T_68K_UNPK,

	T_68K_INTEGER_FIRST = T_68K_ABCD,
	T_68K_INTEGER_LAST = T_68K_UNPK,

	/*
	T_68K_SIZE_BYTE,
	T_68K_SIZE_WORD,
	T_68K_SIZE_LONG,
	*/

	T_68K_REG_CCR,
	T_68K_REG_SR,
	T_68K_REG_BC,
	T_68K_REG_DC,
	T_68K_REG_IC,
	T_68K_REG_SFC,
	T_68K_REG_DFC,
	T_68K_REG_USP,
	T_68K_REG_VBR,
	T_68K_REG_CACR,
	T_68K_REG_CAAR,
	T_68K_REG_MSP,
	T_68K_REG_ISP,
	T_68K_REG_TC,
	T_68K_REG_ITT0,
	T_68K_REG_ITT1,
	T_68K_REG_DTT0,
	T_68K_REG_DTT1,
	T_68K_REG_MMUSR,
	T_68K_REG_URP,
	T_68K_REG_SRP,
	T_68K_REG_IACR0,
	T_68K_REG_IACR1,
	T_68K_REG_DACR0,
	T_68K_REG_DACR1,

	T_68K_SYSREG_FIRST = T_68K_REG_CCR,
	T_68K_SYSREG_LAST = T_68K_REG_DACR1,

	T_68K_REG_PC,
	T_68K_REG_PC_IND,

	T_68K_REG_D0,
	T_68K_REG_D1,
	T_68K_REG_D2,
	T_68K_REG_D3,
	T_68K_REG_D4,
	T_68K_REG_D5,
	T_68K_REG_D6,
	T_68K_REG_D7,

	T_68K_REG_A0,
	T_68K_REG_A1,
	T_68K_REG_A2,
	T_68K_REG_A3,
	T_68K_REG_A4,
	T_68K_REG_A5,
	T_68K_REG_A6,
	T_68K_REG_A7,

	T_68K_REG_A0_IND,
	T_68K_REG_A1_IND,
	T_68K_REG_A2_IND,
	T_68K_REG_A3_IND,
	T_68K_REG_A4_IND,
	T_68K_REG_A5_IND,
	T_68K_REG_A6_IND,
	T_68K_REG_A7_IND,

	T_68K_REG_A0_DEC,
	T_68K_REG_A1_DEC,
	T_68K_REG_A2_DEC,
	T_68K_REG_A3_DEC,
	T_68K_REG_A4_DEC,
	T_68K_REG_A5_DEC,
	T_68K_REG_A6_DEC,
	T_68K_REG_A7_DEC,

	T_68K_REG_A0_INC,
	T_68K_REG_A1_INC,
	T_68K_REG_A2_INC,
	T_68K_REG_A3_INC,
	T_68K_REG_A4_INC,
	T_68K_REG_A5_INC,
	T_68K_REG_A6_INC,
	T_68K_REG_A7_INC,

	T_68K_MC68000,
	T_68K_MC68010,
	T_68K_MC68020,
	T_68K_MC68030,
	T_68K_MC68040,
	T_68K_MC68060,

	T_68K_REGMASK,
}	eTargetToken;

#endif	//LOCALASM_H
