/*  Copyright 2008-2026 Carsten Elton Sorensen and contributors

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

#include <stdbool.h>

#include "errors.h"
#include "expression.h"
#include "lexer_context.h"
#include "parse.h"
#include "parse_expression.h"
#include "section.h"

#include "mips_errors.h"
#include "mips_tokens.h"

#define REG_NONE -1

static int
getRegister(void) {
	if (lex_Context->token.id >= T_MIPS_REG_R0 && lex_Context->token.id <= T_MIPS_REG_R31) {
		int r = lex_Context->token.id - T_MIPS_REG_R0;
		parse_GetToken();

		return r;
	}

	return REG_NONE;
}

static int
expectRegister(void) {
	int result = getRegister();

	if (result == REG_NONE)
		err_Error(MERROR_REGISTER_EXPECTED);

	return result;
}

static uint32_t s_InstructionsRRR[T_MIPS_INTEGER_RRR_LAST - T_MIPS_INTEGER_RRR_FIRST + 1] = {
    0x00000020, // ADD
    0x00000021, // ADDU
    0x00000024, // AND
    0x0000000B, // MOVN
    0x0000000A, // MOVZ,
    0x70000002, // MUL
    0x00000027, // NOR
    0x00000025, // OR
    0x00000046, // ROTRV
    0x00000004, // SLLV
    0x0000002A, // SLT
    0x0000002B, // SLTU
    0x00000007, // SRAV
    0x00000006, // SRLV
    0x00000022, // SUB
    0x00000023, // SUBU
    0x00000026, // XOR
};

static bool
handleIntegerInstructionRRR(void) {
	if (lex_Context->token.id >= T_MIPS_INTEGER_RRR_FIRST && lex_Context->token.id <= T_MIPS_INTEGER_RRR_LAST) {
		uint32_t opcode = s_InstructionsRRR[lex_Context->token.id - T_MIPS_INTEGER_RRR_FIRST];

		parse_GetToken();

		int rd = expectRegister();
		if (rd == REG_NONE)
			return false;

		if (!parse_ExpectChar(','))
			return false;

		int rs = expectRegister();
		if (rs == REG_NONE)
			return false;

		if (!parse_ExpectChar(','))
			return false;

		int rt = expectRegister();
		if (rt == REG_NONE)
			return false;

		sect_OutputConst32(opcode | (rs << 21) | (rt << 16) | (rd << 11));
		return true;
	}

	return false;
}

typedef struct {
	uint32_t opcode;
	bool isSigned;
} SInstructionRRI;

static SInstructionRRI g_RRIInstructions[T_MIPS_INTEGER_RRI_LAST - T_MIPS_INTEGER_RRI_FIRST + 1] = {
    {0x20000000, true }, // ADDI
    {0x24000000, true }, // ADDIU
    {0x30000000, false}, // ANDI
    {0x34000000, false}, // ORI
    {0x28000000, true }, // SLTI
    {0x2C000000, false}, // SLTIU
    {0x38000000, false}, // XORI
};

static bool
handleIntegerInstructionRRI(void) {
	if (lex_Context->token.id >= T_MIPS_INTEGER_RRI_FIRST && lex_Context->token.id <= T_MIPS_INTEGER_RRI_LAST) {
		SInstructionRRI* instruction = &g_RRIInstructions[lex_Context->token.id - T_MIPS_INTEGER_RRI_FIRST];

		parse_GetToken();

		int rt = expectRegister();
		if (rt == REG_NONE)
			return false;

		if (!parse_ExpectChar(','))
			return false;

		int rs = expectRegister();
		if (rs == REG_NONE)
			return false;

		if (!parse_ExpectChar(','))
			return false;

		SExpression* expression;
		if (instruction->isSigned)
			expression = parse_ExpressionS16();
		else
			expression = parse_ExpressionU16();

		if (expression != NULL) {
			expression = expr_Or(expression, expr_Const((rs << 21) | (rt << 16) | instruction->opcode));

			sect_OutputExpr32(expression);
		}
		return true;
	}

	return false;
}

typedef struct {
	int registerCount;
	uint32_t opcode;
} SInstructionBranch;

static SInstructionBranch g_branchInstructions[T_MIPS_BRANCH_LAST - T_MIPS_BRANCH_FIRST + 1] = {
    {2, 0x10000000}, // BEQ
    {2, 0x50000000}, // BEQL
    {2, 0x14000000}, // BNE
    {2, 0x54000000}, // BNEL

  /* Branch R,addr */

    {1, 0x04010000}, // BGEZ
    {1, 0x04110000}, // BGEZAL
    {1, 0x04130000}, // BGEZALL
    {1, 0x04030000}, // BGEZL
    {1, 0x1C000000}, // BGTZ
    {1, 0x5C000000}, // BGTZL
    {1, 0x18000000}, // BLEZ
    {1, 0x58000000}, // BLEZL
    {1, 0x04000000}, // BLTZ
    {1, 0x04100000}, // BLTZAL
    {1, 0x04120000}, // BLTZALL
    {1, 0x02020000}, // BLTZL

  /* Branch addr */

    {0, 0x10000000}, // B
    {0, 0x04110000}, // BAL
};

static bool
handleBranch(void) {
	if (lex_Context->token.id >= T_MIPS_BRANCH_FIRST && lex_Context->token.id <= T_MIPS_BRANCH_LAST) {
		SInstructionBranch* instruction = &g_branchInstructions[lex_Context->token.id - T_MIPS_BRANCH_FIRST];

		parse_GetToken();

		int rs = 0;
		if (instruction->registerCount >= 1) {
			rs = expectRegister();
			if (rs == REG_NONE)
				return false;

			if (!parse_ExpectChar(','))
				return false;
		}

		int rt = 0;
		if (instruction->registerCount >= 2) {
			rt = expectRegister();
			if (rt == REG_NONE)
				return false;

			if (!parse_ExpectChar(','))
				return false;
		}

		SExpression* expression = parse_Expression(4);
		if (expression == NULL)
			return false;

		expression = expr_PcRelative(expression, -4);
		expression = expr_Asr(expression, expr_Const(2));
		expression = expr_CheckRange(expression, -32768, 32767);
		if (expression != NULL) {
			expression = expr_And(expression, expr_Const(0xFFFF));

			expression = expr_Or(expression, expr_Const(instruction->opcode | (rs << 21) | (rt << 16)));

			sect_OutputExpr32(expression);
		} else {
			err_Error(ERROR_OPERAND_RANGE);
		}

		return true;
	}

	return false;
}

static uint32_t g_shiftInstructions[T_MIPS_SHIFT_LAST - T_MIPS_SHIFT_FIRST + 1] = {
    0x00200002, // ROTR
    0x00000000, // SLL
    0x00000003, // SRA
    0x00000002, // SRL
};

static bool
handleShift(void) {
	if (lex_Context->token.id >= T_MIPS_SHIFT_FIRST && lex_Context->token.id <= T_MIPS_SHIFT_LAST) {
		uint32_t opcode = g_shiftInstructions[lex_Context->token.id - T_MIPS_SHIFT_FIRST];

		parse_GetToken();

		int rd = expectRegister();
		if (rd == REG_NONE)
			return false;

		if (!parse_ExpectChar(','))
			return false;

		int rt = expectRegister();
		if (rt == REG_NONE)
			return false;

		if (!parse_ExpectChar(','))
			return false;

		SExpression* expression = parse_Expression(1);
		if (expression == NULL)
			return false;

		expression = expr_CheckRange(expression, 0, 31);
		if (expression != NULL) {
			expression = expr_Asl(expression, expr_Const(6));
			expression = expr_Or(expression, expr_Const(opcode | (rt << 16) | (rd << 11)));

			sect_OutputExpr32(expression);
		} else {
			err_Error(ERROR_OPERAND_RANGE);
		}

		return true;
	}

	return false;
}

static uint32_t g_loadStoreInstructions[T_MIPS_LOADSTORE_LAST - T_MIPS_LOADSTORE_FIRST + 1] = {
    0x80000000, // LB
    0x90000000, // LBU
    0x84000000, // LH
    0x94000000, // LHU
    0xC0000000, // LL
    0x8C000000, // LW
    0xC4000000, // LWC1
    0xC8000000, // LWC2
    0x88000000, // LWL
    0x98000000, // LWR
    0xA0000000, // SB
    0xE0000000, // SC
    0xA4000000, // SH
    0xAC000000, // SW
    0xE4000000, // SWC1
    0xE8000000, // SWC2
    0xA8000000, // SWL
    0xB8000000, // SWR
};

static bool
handleLoadStore(void) {
	if (lex_Context->token.id >= T_MIPS_LOADSTORE_FIRST && lex_Context->token.id <= T_MIPS_LOADSTORE_LAST) {
		uint32_t opcode = g_loadStoreInstructions[lex_Context->token.id - T_MIPS_LOADSTORE_FIRST];

		parse_GetToken();

		int rt = expectRegister();
		if (rt == REG_NONE)
			return false;

		if (!parse_ExpectChar(','))
			return false;

		SExpression* expression = parse_ExpressionS16();
		if (expression == NULL)
			return false;

		if (!parse_ExpectChar('('))
			return false;

		int base = expectRegister();
		if (base == REG_NONE)
			return false;

		if (parse_ExpectChar(')')) {
			expression = expr_Or(expression, expr_Const(opcode | (base << 21) | (rt << 16)));
			sect_OutputExpr32(expression);
			return true;
		}
	}

	return false;
}

static uint32_t g_RSRTInstructions[T_MIPS_RSRT_LAST - T_MIPS_RSRT_FIRST + 1] = {
    0x0000001A, // DIV
    0x0000001B, // DIVU
    0x70000000, // MADD
    0x70000001, // MADDU
    0x70000004, // MSUB
    0x70000005, // MSUBU
    0x00000018, // MULT
    0x00000019, // MULU
};

static bool
handleIntegerInstructionRSRT(void) {
	if (lex_Context->token.id >= T_MIPS_RSRT_FIRST && lex_Context->token.id <= T_MIPS_RSRT_LAST) {
		uint32_t opcode = g_RSRTInstructions[lex_Context->token.id - T_MIPS_RSRT_FIRST];

		parse_GetToken();

		int rs = expectRegister();
		if (rs == REG_NONE)
			return false;

		if (!parse_ExpectChar(','))
			return false;

		int rt = expectRegister();
		if (rt != REG_NONE) {
			sect_OutputConst32(opcode | (rs << 21) | (rt << 16));
			return true;
		}
	}

	return false;
}

static uint32_t g_RDRTInstructions[T_MIPS_RDRT_LAST - T_MIPS_RDRT_FIRST + 1] = {
    0x41400000, // RDPGPR
    0x7C000420, // SEB
    0x7C000620, // SEH
    0x41C00000, // WRPGPR
    0x7C0000A0, // WSBH
};

static bool
handleIntegerInstructionRDRT(void) {
	if (lex_Context->token.id >= T_MIPS_RDRT_FIRST && lex_Context->token.id <= T_MIPS_RDRT_LAST) {
		uint32_t opcode = g_RDRTInstructions[lex_Context->token.id - T_MIPS_RDRT_FIRST];

		parse_GetToken();

		int rd = expectRegister();
		if (rd == REG_NONE)
			return false;

		if (!parse_ExpectChar(','))
			return false;

		int rt = expectRegister();
		if (rt != REG_NONE) {
			sect_OutputConst32(opcode | (rd << 11) | (rt << 16));
			return true;
		}
	}

	return false;
}

static uint32_t g_RSRTCodeInstructions[T_MIPS_RSRTCODE_LAST - T_MIPS_RSRTCODE_FIRST + 1] = {
    0x00000034, // TEQ
    0x00000030, // TGE
    0x00000031, // TGEU
    0x00000032, // TLT
    0x00000033, // TLTU
    0x00000036, // TNE
};

static bool
handleIntegerInstructionRSRTCode(void) {
	if (lex_Context->token.id >= T_MIPS_RSRTCODE_FIRST && lex_Context->token.id <= T_MIPS_RSRTCODE_LAST) {
		uint32_t opcode = g_RSRTCodeInstructions[lex_Context->token.id - T_MIPS_RSRTCODE_FIRST];

		parse_GetToken();

		int rs = expectRegister();
		if (rs == REG_NONE)
			return false;

		if (!parse_ExpectChar(','))
			return false;

		int rt = expectRegister();
		if (rt != REG_NONE) {
			if (lex_Context->token.id == ',') {
				parse_GetToken();

				SExpression* expression = parse_Expression(2);
				if (expression == NULL) {
					err_Error(ERROR_INVALID_EXPRESSION);
					return true;
				}

				expression = expr_CheckRange(expression, 0, 1023);
				if (expression == NULL) {
					err_Error(ERROR_OPERAND_RANGE);
					return true;
				}

				expression = expr_Asl(expression, expr_Const(6));
				expression = expr_Or(expression, expr_Const(opcode | (rs << 21) | (rt << 16)));
				sect_OutputExpr32(expression);
			} else {
				sect_OutputConst32(opcode | (rs << 21) | (rt << 16));
			}

			return true;
		}
	}

	return false;
}

static SInstructionRRI g_RIInstructions[T_MIPS_INTEGER_RI_LAST - T_MIPS_INTEGER_RI_FIRST + 1] = {
    {0x040C0000, true }, // TEQI
    {0x04080000, true }, // TGEI
    {0x04090000, false}, // TGEIU
    {0x040A0000, true }, // TLTI
    {0x040B0000, false}, // TLTIU
    {0x040E0000, true }, // TNEI
};

static bool
handleIntegerInstructionRI(void) {
	if (lex_Context->token.id >= T_MIPS_INTEGER_RI_FIRST && lex_Context->token.id <= T_MIPS_INTEGER_RI_LAST) {
		SInstructionRRI* instruction = &g_RIInstructions[lex_Context->token.id - T_MIPS_INTEGER_RI_FIRST];

		parse_GetToken();
		int rs = expectRegister();
		if (rs == REG_NONE)
			return false;

		if (!parse_ExpectChar(','))
			return false;

		SExpression* expression = instruction->isSigned ? parse_ExpressionS16() : parse_ExpressionU16();

		if (expression != NULL) {
			expression = expr_Or(expression, expr_Const(instruction->opcode | (rs << 21)));
			sect_OutputExpr32(expression);
			return true;
		}
	}

	return false;
}

static uint32_t g_RDRSRTCopyInstructions[T_MIPS_INTEGER_RDRS_RTCOPY_LAST - T_MIPS_INTEGER_RDRS_RTCOPY_FIRST + 1] = {
    0x70000021, // CLO
    0x70000020  // CLZ
};

static bool
handleIntegerInstructionRDRSRTCopy(void) {
	if (lex_Context->token.id >= T_MIPS_INTEGER_RDRS_RTCOPY_FIRST && lex_Context->token.id <= T_MIPS_INTEGER_RDRS_RTCOPY_LAST) {
		uint32_t opcode = g_RDRSRTCopyInstructions[lex_Context->token.id - T_MIPS_INTEGER_RDRS_RTCOPY_FIRST];

		parse_GetToken();

		int rd = expectRegister();
		if (rd == REG_NONE)
			return false;

		if (!parse_ExpectChar(','))
			return false;

		int rs = expectRegister();
		if (rs != REG_NONE) {
			sect_OutputConst32(opcode | (rd << 11) | (rs << 21) | (rd << 16));

			return true;
		}
	}

	return false;
}

static uint32_t g_noParameterInstructions[T_MIPS_INTEGER_NO_PARAMETER_LAST - T_MIPS_INTEGER_NO_PARAMETER_FIRST + 1] = {
    0x4200001F, // DERET
    0x000000C0, // EHB
    0x42000018, // ERET
    0x00000000, // NOP
    0x00000040, // SSNOP
    0x42000008, // TLBP
    0x42000001, // TLBR
    0x42000002, // TLBWI
    0x42000006, // TLBWR
};

static bool
handleIntegerNoParameter(void) {
	if (lex_Context->token.id >= T_MIPS_INTEGER_NO_PARAMETER_FIRST && lex_Context->token.id <= T_MIPS_INTEGER_NO_PARAMETER_LAST) {
		uint32_t opcode = g_noParameterInstructions[lex_Context->token.id - T_MIPS_INTEGER_NO_PARAMETER_FIRST];

		parse_GetToken();

		sect_OutputConst32(opcode);
		return true;
	}

	return false;
}

static uint32_t g_RTInstructions[T_MIPS_INTEGER_RT_LAST - T_MIPS_INTEGER_RT_FIRST + 1] = {
    0x41606000, // DI
    0x41606020, // EI
};

static bool
handleIntegerRT(void) {
	if (lex_Context->token.id >= T_MIPS_INTEGER_RT_FIRST && lex_Context->token.id <= T_MIPS_INTEGER_RT_LAST) {
		uint32_t opcode = g_RTInstructions[lex_Context->token.id - T_MIPS_INTEGER_RT_FIRST];

		parse_GetToken();

		int rt = getRegister();
		if (rt == REG_NONE)
			rt = 0;

		sect_OutputConst32(opcode | (rt << 16));

		return true;
	}

	return false;
}

static uint32_t g_RDInstructions[T_MIPS_INTEGER_RD_LAST - T_MIPS_INTEGER_RD_FIRST + 1] = {
    0x00000010, // MFHI
    0x00000012, // MFLO
};

static bool
handleIntegerRD(void) {
	if (lex_Context->token.id >= T_MIPS_INTEGER_RD_FIRST && lex_Context->token.id <= T_MIPS_INTEGER_RD_LAST) {
		uint32_t opcode = g_RDInstructions[lex_Context->token.id - T_MIPS_INTEGER_RD_FIRST];

		parse_GetToken();

		int rd = expectRegister();
		if (rd == REG_NONE)
			return false;

		sect_OutputConst32(opcode | (rd << 11));
		return true;
	}

	return false;
}

static uint32_t g_RSInstructions[T_MIPS_INTEGER_RS_LAST - T_MIPS_INTEGER_RS_FIRST + 1] = {
    0x00000008, // JR
    0x00000408, // JR.HB
    0x00000011, // MTHI
    0x00000013, // MTLO
};

static bool
handleIntegerRS(void) {
	if (lex_Context->token.id >= T_MIPS_INTEGER_RS_FIRST && lex_Context->token.id <= T_MIPS_INTEGER_RS_LAST) {
		uint32_t opcode = g_RSInstructions[lex_Context->token.id - T_MIPS_INTEGER_RS_FIRST];

		parse_GetToken();

		int rs = expectRegister();
		if (rs == REG_NONE)
			return false;

		sect_OutputConst32(opcode | (rs << 21));
		return true;
	}

	return false;
}

static uint32_t g_jumpAbsInstructions[T_MIPS_INTEGER_J_ABS_LAST - T_MIPS_INTEGER_J_ABS_FIRST + 1] = {
    0x08000000, // J
    0x0C000000, // JAL
};

static bool
handleIntegerJumpAbs(void) {
	if (lex_Context->token.id >= T_MIPS_INTEGER_J_ABS_FIRST && lex_Context->token.id <= T_MIPS_INTEGER_J_ABS_LAST) {
		uint32_t opcode = g_jumpAbsInstructions[lex_Context->token.id - T_MIPS_INTEGER_J_ABS_FIRST];

		parse_GetToken();

		SExpression* expression = parse_Expression(4);
		if (expression != NULL) {
			expression = expr_Asr(expression, expr_Const(2));
			expression = expr_And(expression, expr_Const(0x03FFFFFF));
			expression = expr_Or(expression, expr_Const(opcode));

			sect_OutputExpr32(expression);
			return true;
		}
	}
	return false;
}

static bool
handleLUI(void) {
	if (lex_Context->token.id == T_MIPS_LUI) {
		parse_GetToken();

		int rd = expectRegister();
		if (rd != REG_NONE) {
			if (parse_ExpectChar(',')) {
				SExpression* expression = parse_ExpressionU16();
				if (expression != NULL) {
					expression = expr_Or(expression, expr_Const((15 << 26) | (rd << 16)));
					sect_OutputExpr32(expression);
				}
			}
		}
		return true;
	}

	return false;
}

typedef bool (*mnemonicHandler)(void);

static mnemonicHandler g_mnemonicHandlers[T_MIPS_LUI - T_MIPS_ADD + 1] = {
    handleIntegerInstructionRRR, //	T_MIPS_ADD = 6000,
    handleIntegerInstructionRRR, //	T_MIPS_ADDU,
    handleIntegerInstructionRRR, //	T_MIPS_AND,
    handleIntegerInstructionRRR, //	T_MIPS_MOVN,
    handleIntegerInstructionRRR, //	T_MIPS_MOVZ,
    handleIntegerInstructionRRR, //	T_MIPS_MUL,
    handleIntegerInstructionRRR, //	T_MIPS_NOR,
    handleIntegerInstructionRRR, //	T_MIPS_OR,
    handleIntegerInstructionRRR, //	T_MIPS_ROTRV,
    handleIntegerInstructionRRR, //	T_MIPS_SLLV,
    handleIntegerInstructionRRR, //	T_MIPS_SLT,
    handleIntegerInstructionRRR, //	T_MIPS_SLTU,
    handleIntegerInstructionRRR, //	T_MIPS_SRAV,
    handleIntegerInstructionRRR, //	T_MIPS_SRLV,
    handleIntegerInstructionRRR, //	T_MIPS_SUB,
    handleIntegerInstructionRRR, //	T_MIPS_SUBU,
    handleIntegerInstructionRRR, //	T_MIPS_XOR,

    handleIntegerInstructionRRI, //	T_MIPS_ADDI,
    handleIntegerInstructionRRI, //	T_MIPS_ADDIU,
    handleIntegerInstructionRRI, //	T_MIPS_ANDI,
    handleIntegerInstructionRRI, //	T_MIPS_ORI,
    handleIntegerInstructionRRI, //	T_MIPS_SLTI,
    handleIntegerInstructionRRI, //	T_MIPS_SLTIU,
    handleIntegerInstructionRRI, //	T_MIPS_XORI,

    handleBranch, //	T_MIPS_BEQ,
    handleBranch, //	T_MIPS_BEQL,
    handleBranch, //	T_MIPS_BNE,
    handleBranch, //	T_MIPS_BNEL,
    handleBranch, //	T_MIPS_BGEZ,
    handleBranch, //	T_MIPS_BGEZAL,
    handleBranch, //	T_MIPS_BGEZALL,
    handleBranch, //	T_MIPS_BGEZL,
    handleBranch, //	T_MIPS_BGTZ,
    handleBranch, //	T_MIPS_BGTZL,
    handleBranch, //	T_MIPS_BLEZ,
    handleBranch, //	T_MIPS_BLEZL,
    handleBranch, //	T_MIPS_BLTZ,
    handleBranch, //	T_MIPS_BLTZAL,
    handleBranch, //	T_MIPS_BLTZALL,
    handleBranch, //	T_MIPS_BLTZL,
    handleBranch, //	T_MIPS_B,
    handleBranch, //	T_MIPS_BAL,

    handleShift, //	T_MIPS_ROTR,
    handleShift, //	T_MIPS_SLL,
    handleShift, //	T_MIPS_SRA,
    handleShift, //	T_MIPS_SRL,

    handleLoadStore, //	T_MIPS_LB,
    handleLoadStore, //	T_MIPS_LBU,
    handleLoadStore, //	T_MIPS_LH,
    handleLoadStore, //	T_MIPS_LHU,
    handleLoadStore, //	T_MIPS_LL,
    handleLoadStore, //	T_MIPS_LW,
    handleLoadStore, //	T_MIPS_LWC1,
    handleLoadStore, //	T_MIPS_LWC2,
    handleLoadStore, //	T_MIPS_LWL,
    handleLoadStore, //	T_MIPS_LWR,
    handleLoadStore, //	T_MIPS_SB,
    handleLoadStore, //	T_MIPS_SC,
    handleLoadStore, //	T_MIPS_SH,
    handleLoadStore, //	T_MIPS_SW,
    handleLoadStore, //	T_MIPS_SWC1,
    handleLoadStore, //	T_MIPS_SWC2,
    handleLoadStore, //	T_MIPS_SWL,
    handleLoadStore, //	T_MIPS_SWR,

    handleIntegerInstructionRSRT, //	T_MIPS_DIV,
    handleIntegerInstructionRSRT, //	T_MIPS_DIVU,
    handleIntegerInstructionRSRT, //	T_MIPS_MADD,
    handleIntegerInstructionRSRT, //	T_MIPS_MADDU,
    handleIntegerInstructionRSRT, //	T_MIPS_MSUB,
    handleIntegerInstructionRSRT, //	T_MIPS_MSUBU,
    handleIntegerInstructionRSRT, //	T_MIPS_MULT,
    handleIntegerInstructionRSRT, //	T_MIPS_MULU,

    handleIntegerInstructionRDRT, //	T_MIPS_RDPGPR,
    handleIntegerInstructionRDRT, //	T_MIPS_SEB,
    handleIntegerInstructionRDRT, //	T_MIPS_SEH,
    handleIntegerInstructionRDRT, //	T_MIPS_WRPGPR,
    handleIntegerInstructionRDRT, //	T_MIPS_WSBH,

    handleIntegerInstructionRSRTCode, //	T_MIPS_TEQ,
    handleIntegerInstructionRSRTCode, //	T_MIPS_TGE,
    handleIntegerInstructionRSRTCode, //	T_MIPS_TGEU,
    handleIntegerInstructionRSRTCode, //	T_MIPS_TLT,
    handleIntegerInstructionRSRTCode, //	T_MIPS_TLTU,
    handleIntegerInstructionRSRTCode, //	T_MIPS_TNE,

    handleIntegerInstructionRI, //	T_MIPS_TEQI,
    handleIntegerInstructionRI, //	T_MIPS_TGEI,
    handleIntegerInstructionRI, //	T_MIPS_TGEIU,
    handleIntegerInstructionRI, //	T_MIPS_TLTI,
    handleIntegerInstructionRI, //	T_MIPS_TLTIU,
    handleIntegerInstructionRI, //	T_MIPS_TNEI,

    handleIntegerInstructionRDRSRTCopy, //	T_MIPS_CLO,
    handleIntegerInstructionRDRSRTCopy, //	T_MIPS_CLZ,

    handleIntegerNoParameter, //	T_MIPS_DERET,
    handleIntegerNoParameter, //	T_MIPS_EHB,
    handleIntegerNoParameter, //	T_MIPS_ERET,
    handleIntegerNoParameter, //	T_MIPS_NOP,
    handleIntegerNoParameter, //	T_MIPS_SSNOP,
    handleIntegerNoParameter, //	T_MIPS_TLBP,
    handleIntegerNoParameter, //	T_MIPS_TLBR,
    handleIntegerNoParameter, //	T_MIPS_TLBWI,
    handleIntegerNoParameter, //	T_MIPS_TLBWR,

    handleIntegerRT, //	T_MIPS_DI,
    handleIntegerRT, //	T_MIPS_EI,

    handleIntegerRD, //	T_MIPS_MFHI,
    handleIntegerRD, //	T_MIPS_MFLO,

    handleIntegerRS, //	T_MIPS_JR,
    handleIntegerRS, //	T_MIPS_JR_HB,
    handleIntegerRS, //	T_MIPS_MTHI,
    handleIntegerRS, //	T_MIPS_MTLO,

    handleIntegerJumpAbs, //	T_MIPS_J,
    handleIntegerJumpAbs, //	T_MIPS_JAL,

    handleLUI //	T_MIPS_LUI
};

bool
mips_ParseIntegerInstruction(void) {
	if (T_MIPS_ADD <= lex_Context->token.id && lex_Context->token.id <= T_MIPS_LUI) {
		return g_mnemonicHandlers[lex_Context->token.id - T_MIPS_ADD]();
	}

	return false;
}
