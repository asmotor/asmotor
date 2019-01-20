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

#include "xasm.h"
#include "lexer_constants.h"

#include "mips_tokens.h"

static SLexConstantsWord s_tokens[]= {
	{ "ADD",	T_MIPS_ADD		},
	{ "ADDU",	T_MIPS_ADDU		},
	{ "AND",	T_MIPS_AND		},
	{ "MOVN",	T_MIPS_MOVN		},
	{ "MOVZ",	T_MIPS_MOVZ		},
	{ "MUL",	T_MIPS_MUL		},
	{ "NOR",	T_MIPS_NOR		},
	{ "OR",		T_MIPS_OR		},
	{ "ROTRV",	T_MIPS_ROTRV	},
	{ "SLLV",	T_MIPS_SLLV		},
	{ "SLT",	T_MIPS_SLT		},
	{ "SLTU",	T_MIPS_SLTU		},
	{ "SRAV",	T_MIPS_SRAV		},
	{ "SRLV",	T_MIPS_SRLV		},
	{ "SUB",	T_MIPS_SUB		},
	{ "SUBU",	T_MIPS_SUBU		},
	{ "XOR",	T_MIPS_XOR		},

	{ "ADDI",	T_MIPS_ADDI		},
	{ "ADDIU",	T_MIPS_ADDIU	},
	{ "ANDI",	T_MIPS_ANDI		},
	{ "ORI",	T_MIPS_ORI		},
	{ "SLTI",	T_MIPS_SLTI		},
	{ "SLTIU",	T_MIPS_SLTIU	},
	{ "XORI",	T_MIPS_XORI		},

	{ "BEQ",		T_MIPS_BEQ		},
	{ "BEQL",		T_MIPS_BEQL		},
	{ "BNE",		T_MIPS_BNE		},
	{ "BNEL",		T_MIPS_BNEL		},
	{ "BGEZ",		T_MIPS_BGEZ		},
	{ "BGEZAL",		T_MIPS_BGEZAL	},
	{ "BGEZALL",	T_MIPS_BGEZALL	},
	{ "BGEZL",		T_MIPS_BGEZL	},
	{ "BGTZ",		T_MIPS_BGTZ		},
	{ "BGTZL",		T_MIPS_BGTZL	},
	{ "BLEZ",		T_MIPS_BLEZ		},
	{ "BLEZL",		T_MIPS_BLEZL	},
	{ "BLTZ",		T_MIPS_BLTZ		},
	{ "BLTZAL",		T_MIPS_BLTZAL	},
	{ "BLTZALL",	T_MIPS_BLTZALL	},
	{ "BLTZL",		T_MIPS_BLTZL	},
	{ "B",			T_MIPS_B		},
	{ "BAL",		T_MIPS_BAL		},
	
	{ "ROTR",	T_MIPS_ROTR	},
	{ "SLL",	T_MIPS_SLL	},
	{ "SRA",	T_MIPS_SRA	},
	{ "SRL",	T_MIPS_SRL	},
	
	{ "LB",		T_MIPS_LB	},
	{ "LBU",	T_MIPS_LBU	},
	{ "LH",		T_MIPS_LH	},
	{ "LHU",	T_MIPS_LHU	},
	{ "LL",		T_MIPS_LL	},
	{ "LW",		T_MIPS_LW	},
	{ "LWC1",	T_MIPS_LWC1	},
	{ "LWC2",	T_MIPS_LWC2	},
	{ "LWL",	T_MIPS_LWL	},
	{ "LWR",	T_MIPS_LWR	},
	{ "SB",		T_MIPS_SB	},
	{ "SC",		T_MIPS_SC	},
	{ "SH",		T_MIPS_SH	},
	{ "SW",		T_MIPS_SW	},
	{ "SWC1",	T_MIPS_SWC1	},
	{ "SWC2",	T_MIPS_SWC2	},
	{ "SWL",	T_MIPS_SWL	},
	{ "SWR",	T_MIPS_SWR	},

	{ "DIV",	T_MIPS_DIV		},
	{ "DIVU",	T_MIPS_DIVU		},
	{ "MADD",	T_MIPS_MADD		},
	{ "MADDU",	T_MIPS_MADDU	},
	{ "MSUB",	T_MIPS_MSUB		},
	{ "MSUBU",	T_MIPS_MSUBU	},
	{ "MULT",	T_MIPS_MULT		},
	{ "MULU",	T_MIPS_MULU		},

	{ "RDPGPR", T_MIPS_RDPGPR	},
	{ "SEB",	T_MIPS_SEB		},
	{ "SEH",	T_MIPS_SEH		},
	{ "WRPGPR", T_MIPS_WRPGPR	},
	{ "WSBH",	T_MIPS_WSBH		},

	{ "TEQ",	T_MIPS_TEQ		},
	{ "TGE",	T_MIPS_TGE		},
	{ "TGEU",	T_MIPS_TGEU		},
	{ "TLT",	T_MIPS_TLT		},
	{ "TLTU",	T_MIPS_TLTU		},
	{ "TNE",	T_MIPS_TNE		},

	{ "TEQI",	T_MIPS_TEQI		},
	{ "TGEI",	T_MIPS_TGEI		},
	{ "TGEIU",	T_MIPS_TGEIU	},
	{ "TLTI",	T_MIPS_TLTI		},
	{ "TLTIU",	T_MIPS_TLTIU	},
	{ "TNEI",	T_MIPS_TNEI		},

	{ "CLO",	T_MIPS_CLO		},
	{ "CLZ",	T_MIPS_CLZ		},

	{ "DERET",	T_MIPS_DERET	},
	{ "EHB",	T_MIPS_EHB		},
	{ "ERET",	T_MIPS_ERET		},
	{ "NOP",	T_MIPS_NOP		},
	{ "SSNOP",	T_MIPS_SSNOP	},
	{ "TLBP",	T_MIPS_TLBP		},
	{ "TLBR",	T_MIPS_TLBR		},
	{ "TLBWI",	T_MIPS_TLBWI	},
	{ "TLBWR",	T_MIPS_TLBWR	},
	
	{ "DI",	T_MIPS_DI	},
	{ "EI",	T_MIPS_EI	},

	{ "MFHI",	T_MIPS_MFHI	},
	{ "MFLO",	T_MIPS_MFLO	},

	{ "MTHI",	T_MIPS_MTHI	},
	{ "MTLO",	T_MIPS_MTLO	},

	{ "J",		T_MIPS_J	},
	{ "JAL",	T_MIPS_JAL	},

	{ "JALR",		T_MIPS_JALR		},
	{ "JALR.HB",	T_MIPS_JALR_HB	},
	{ "JR",			T_MIPS_JR		},
	{ "JR.HB",		T_MIPS_JR_HB	},

	{ "LUI",	T_MIPS_LUI	},

	{ "R0",		T_MIPS_REG_R0	},
	{ "R1",		T_MIPS_REG_R1	},
	{ "R2",		T_MIPS_REG_R2	},
	{ "R3",		T_MIPS_REG_R3	},
	{ "R4",		T_MIPS_REG_R4	},
	{ "R5",		T_MIPS_REG_R5	},
	{ "R6",		T_MIPS_REG_R6	},
	{ "R7",		T_MIPS_REG_R7	},
	{ "R8",		T_MIPS_REG_R8	},
	{ "R9",		T_MIPS_REG_R9	},
	{ "R10",	T_MIPS_REG_R10	},
	{ "R11",	T_MIPS_REG_R11	},
	{ "R12",	T_MIPS_REG_R12	},
	{ "R13",	T_MIPS_REG_R13	},
	{ "R14",	T_MIPS_REG_R14	},
	{ "R15",	T_MIPS_REG_R15	},
	{ "R16",	T_MIPS_REG_R16	},
	{ "R17",	T_MIPS_REG_R17	},
	{ "R18",	T_MIPS_REG_R18	},
	{ "R19",	T_MIPS_REG_R19	},
	{ "R20",	T_MIPS_REG_R20	},
	{ "R21",	T_MIPS_REG_R21	},
	{ "R22",	T_MIPS_REG_R22	},
	{ "R23",	T_MIPS_REG_R23	},
	{ "R24",	T_MIPS_REG_R24	},
	{ "R25",	T_MIPS_REG_R25	},
	{ "R26",	T_MIPS_REG_R26	},
	{ "R27",	T_MIPS_REG_R27	},
	{ "R28",	T_MIPS_REG_R28	},
	{ "R29",	T_MIPS_REG_R29	},
	{ "R30",	T_MIPS_REG_R30	},
	{ "R31",	T_MIPS_REG_R31	},

	{ "ZERO",	T_MIPS_REG_R0	},
	{ "AT",		T_MIPS_REG_R1	},
	{ "V0",		T_MIPS_REG_R2	},
	{ "V1",		T_MIPS_REG_R3	},
	{ "A0",		T_MIPS_REG_R4	},
	{ "A1",		T_MIPS_REG_R5	},
	{ "A2",		T_MIPS_REG_R6	},
	{ "A3",		T_MIPS_REG_R7	},
	{ "T0",		T_MIPS_REG_R8	},
	{ "T1",		T_MIPS_REG_R9	},
	{ "T2",		T_MIPS_REG_R10	},
	{ "T3",		T_MIPS_REG_R11	},
	{ "T4",		T_MIPS_REG_R12	},
	{ "T5",		T_MIPS_REG_R13	},
	{ "T6",		T_MIPS_REG_R14	},
	{ "T7",		T_MIPS_REG_R15	},
	{ "S0",		T_MIPS_REG_R16	},
	{ "S1",		T_MIPS_REG_R17	},
	{ "S2",		T_MIPS_REG_R18	},
	{ "S3",		T_MIPS_REG_R19	},
	{ "S4",		T_MIPS_REG_R20	},
	{ "S5",		T_MIPS_REG_R21	},
	{ "S6",		T_MIPS_REG_R22	},
	{ "S7",		T_MIPS_REG_R23	},
	{ "T8",		T_MIPS_REG_R24	},
	{ "T9",		T_MIPS_REG_R25	},
	{ "K0",		T_MIPS_REG_R26	},
	{ "K1",		T_MIPS_REG_R27	},
	{ "GP",		T_MIPS_REG_R28	},
	{ "SP",		T_MIPS_REG_R29	},
	{ "FP",		T_MIPS_REG_R30	},
	{ "RA",		T_MIPS_REG_R31	},

	{ "MIPS32R1",	T_MIPS_MIPS32R1	},
	{ "MIPS32",		T_MIPS_MIPS32R2	},
	{ "MIPS32R2",	T_MIPS_MIPS32R2	},

	{ NULL, 0 }
};

void loclexer_Init(void) {
	lex_ConstantsDefineWords(s_tokens);
}
