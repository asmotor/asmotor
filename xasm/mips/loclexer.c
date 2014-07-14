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

#include "xasm.h"
#include "lexer.h"

#include "localasm.h"

static SLexInitString localstrings[]=
{
	{ "add",	T_MIPS_ADD		},
	{ "addu",	T_MIPS_ADDU		},
	{ "and",	T_MIPS_AND		},
	{ "movn",	T_MIPS_MOVN		},
	{ "movz",	T_MIPS_MOVZ		},
	{ "mul",	T_MIPS_MUL		},
	{ "nor",	T_MIPS_NOR		},
	{ "or",		T_MIPS_OR		},
	{ "rotrv",	T_MIPS_ROTRV	},
	{ "sllv",	T_MIPS_SLLV		},
	{ "slt",	T_MIPS_SLT		},
	{ "sltu",	T_MIPS_SLTU		},
	{ "srav",	T_MIPS_SRAV		},
	{ "srlv",	T_MIPS_SRLV		},
	{ "sub",	T_MIPS_SUB		},
	{ "subu",	T_MIPS_SUBU		},
	{ "xor",	T_MIPS_XOR		},

	{ "addi",	T_MIPS_ADDI		},
	{ "addiu",	T_MIPS_ADDIU	},
	{ "andi",	T_MIPS_ANDI		},
	{ "ori",	T_MIPS_ORI		},
	{ "slti",	T_MIPS_SLTI		},
	{ "sltiu",	T_MIPS_SLTIU	},
	{ "xori",	T_MIPS_XORI		},

	{ "beq",		T_MIPS_BEQ		},
	{ "beql",		T_MIPS_BEQL		},
	{ "bne",		T_MIPS_BNE		},
	{ "bnel",		T_MIPS_BNEL		},
	{ "bgez",		T_MIPS_BGEZ		},
	{ "bgezal",		T_MIPS_BGEZAL	},
	{ "bgezall",	T_MIPS_BGEZALL	},
	{ "bgezl",		T_MIPS_BGEZL	},
	{ "bgtz",		T_MIPS_BGTZ		},
	{ "bgtzl",		T_MIPS_BGTZL	},
	{ "blez",		T_MIPS_BLEZ		},
	{ "blezl",		T_MIPS_BLEZL	},
	{ "bltz",		T_MIPS_BLTZ		},
	{ "bltzal",		T_MIPS_BLTZAL	},
	{ "bltzall",	T_MIPS_BLTZALL	},
	{ "bltzl",		T_MIPS_BLTZL	},
	{ "b",			T_MIPS_B		},
	{ "bal",		T_MIPS_BAL		},
	
	{ "rotr",	T_MIPS_ROTR	},
	{ "sll",	T_MIPS_SLL	},
	{ "sra",	T_MIPS_SRA	},
	{ "srl",	T_MIPS_SRL	},
	
	{ "lb",		T_MIPS_LB	},
	{ "lbu",	T_MIPS_LBU	},
	{ "lh",		T_MIPS_LH	},
	{ "lhu",	T_MIPS_LHU	},
	{ "ll",		T_MIPS_LL	},
	{ "lw",		T_MIPS_LW	},
	{ "lwc1",	T_MIPS_LWC1	},
	{ "lwc2",	T_MIPS_LWC2	},
	{ "lwl",	T_MIPS_LWL	},
	{ "lwr",	T_MIPS_LWR	},
	{ "sb",		T_MIPS_SB	},
	{ "sc",		T_MIPS_SC	},
	{ "sh",		T_MIPS_SH	},
	{ "sw",		T_MIPS_SW	},
	{ "swc1",	T_MIPS_SWC1	},
	{ "swc2",	T_MIPS_SWC2	},
	{ "swl",	T_MIPS_SWL	},
	{ "swr",	T_MIPS_SWR	},

	{ "div",	T_MIPS_DIV		},
	{ "divu",	T_MIPS_DIVU		},
	{ "madd",	T_MIPS_MADD		},
	{ "maddu",	T_MIPS_MADDU	},
	{ "msub",	T_MIPS_MSUB		},
	{ "msubu",	T_MIPS_MSUBU	},
	{ "mult",	T_MIPS_MULT		},
	{ "mulu",	T_MIPS_MULU		},

	{ "rdpgpr", T_MIPS_RDPGPR	},
	{ "seb",	T_MIPS_SEB		},
	{ "seh",	T_MIPS_SEH		},
	{ "wrpgpr", T_MIPS_WRPGPR	},
	{ "wsbh",	T_MIPS_WSBH		},

	{ "teq",	T_MIPS_TEQ		},
	{ "tge",	T_MIPS_TGE		},
	{ "tgeu",	T_MIPS_TGEU		},
	{ "tlt",	T_MIPS_TLT		},
	{ "tltu",	T_MIPS_TLTU		},
	{ "tne",	T_MIPS_TNE		},

	{ "teqi",	T_MIPS_TEQI		},
	{ "tgei",	T_MIPS_TGEI		},
	{ "tgeiu",	T_MIPS_TGEIU	},
	{ "tlti",	T_MIPS_TLTI		},
	{ "tltiu",	T_MIPS_TLTIU	},
	{ "tnei",	T_MIPS_TNEI		},

	{ "clo",	T_MIPS_CLO		},
	{ "clz",	T_MIPS_CLZ		},

	{ "deret",	T_MIPS_DERET	},
	{ "ehb",	T_MIPS_EHB		},
	{ "eret",	T_MIPS_ERET		},
	{ "nop",	T_MIPS_NOP		},
	{ "ssnop",	T_MIPS_SSNOP	},
	{ "tlbp",	T_MIPS_TLBP		},
	{ "tlbr",	T_MIPS_TLBR		},
	{ "tlbwi",	T_MIPS_TLBWI	},
	{ "tlbwr",	T_MIPS_TLBWR	},
	
	{ "di",	T_MIPS_DI	},
	{ "ei",	T_MIPS_EI	},

	{ "mfhi",	T_MIPS_MFHI	},
	{ "mflo",	T_MIPS_MFLO	},

	{ "mthi",	T_MIPS_MTHI	},
	{ "mtlo",	T_MIPS_MTLO	},

	{ "j",		T_MIPS_J	},
	{ "jal",	T_MIPS_JAL	},

	{ "jalr",		T_MIPS_JALR		},
	{ "jalr.hb",	T_MIPS_JALR_HB	},
	{ "jr",			T_MIPS_JR		},
	{ "jr.hb",		T_MIPS_JR_HB	},

	{ "lui",	T_MIPS_LUI	},

	{ "r0",		T_MIPS_REG_R0	},
	{ "r1",		T_MIPS_REG_R1	},
	{ "r2",		T_MIPS_REG_R2	},
	{ "r3",		T_MIPS_REG_R3	},
	{ "r4",		T_MIPS_REG_R4	},
	{ "r5",		T_MIPS_REG_R5	},
	{ "r6",		T_MIPS_REG_R6	},
	{ "r7",		T_MIPS_REG_R7	},
	{ "r8",		T_MIPS_REG_R8	},
	{ "r9",		T_MIPS_REG_R9	},
	{ "r10",	T_MIPS_REG_R10	},
	{ "r11",	T_MIPS_REG_R11	},
	{ "r12",	T_MIPS_REG_R12	},
	{ "r13",	T_MIPS_REG_R13	},
	{ "r14",	T_MIPS_REG_R14	},
	{ "r15",	T_MIPS_REG_R15	},
	{ "r16",	T_MIPS_REG_R16	},
	{ "r17",	T_MIPS_REG_R17	},
	{ "r18",	T_MIPS_REG_R18	},
	{ "r19",	T_MIPS_REG_R19	},
	{ "r20",	T_MIPS_REG_R20	},
	{ "r21",	T_MIPS_REG_R21	},
	{ "r22",	T_MIPS_REG_R22	},
	{ "r23",	T_MIPS_REG_R23	},
	{ "r24",	T_MIPS_REG_R24	},
	{ "r25",	T_MIPS_REG_R25	},
	{ "r26",	T_MIPS_REG_R26	},
	{ "r27",	T_MIPS_REG_R27	},
	{ "r28",	T_MIPS_REG_R28	},
	{ "r29",	T_MIPS_REG_R29	},
	{ "r30",	T_MIPS_REG_R30	},
	{ "r31",	T_MIPS_REG_R31	},

	{ "zero",	T_MIPS_REG_R0	},
	{ "at",		T_MIPS_REG_R1	},
	{ "v0",		T_MIPS_REG_R2	},
	{ "v1",		T_MIPS_REG_R3	},
	{ "a0",		T_MIPS_REG_R4	},
	{ "a1",		T_MIPS_REG_R5	},
	{ "a2",		T_MIPS_REG_R6	},
	{ "a3",		T_MIPS_REG_R7	},
	{ "t0",		T_MIPS_REG_R8	},
	{ "t1",		T_MIPS_REG_R9	},
	{ "t2",		T_MIPS_REG_R10	},
	{ "t3",		T_MIPS_REG_R11	},
	{ "t4",		T_MIPS_REG_R12	},
	{ "t5",		T_MIPS_REG_R13	},
	{ "t6",		T_MIPS_REG_R14	},
	{ "t7",		T_MIPS_REG_R15	},
	{ "s0",		T_MIPS_REG_R16	},
	{ "s1",		T_MIPS_REG_R17	},
	{ "s2",		T_MIPS_REG_R18	},
	{ "s3",		T_MIPS_REG_R19	},
	{ "s4",		T_MIPS_REG_R20	},
	{ "s5",		T_MIPS_REG_R21	},
	{ "s6",		T_MIPS_REG_R22	},
	{ "s7",		T_MIPS_REG_R23	},
	{ "t8",		T_MIPS_REG_R24	},
	{ "t9",		T_MIPS_REG_R25	},
	{ "k0",		T_MIPS_REG_R26	},
	{ "k1",		T_MIPS_REG_R27	},
	{ "gp",		T_MIPS_REG_R28	},
	{ "sp",		T_MIPS_REG_R29	},
	{ "fp",		T_MIPS_REG_R30	},
	{ "ra",		T_MIPS_REG_R31	},

	{ "mips32r1",	T_MIPS_MIPS32R1	},
	{ "mips32",		T_MIPS_MIPS32R2	},
	{ "mips32r2",	T_MIPS_MIPS32R2	},

	{ NULL, 0 }
};

void	loclexer_Init(void)
{
	lex_AddStrings(localstrings);
}
