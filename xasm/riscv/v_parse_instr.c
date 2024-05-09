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

#include <ctype.h>
#include <stdbool.h>

#include "errors.h"
#include "expression.h"
#include "lexer_context.h"
#include "options.h"
#include "parse.h"
#include "parse_expression.h"

#include "section.h"
#include "tokens.h"
#include "v_errors.h"
#include "v_options.h"
#include "v_tokens.h"


typedef struct {
	int rd, rs1, rs2;
} SAddressingMode;

typedef struct Parser {
	uint32_t baseOpcode;
	bool privileged;
	bool (*parser)(uint32_t opcode);
} SParser;


static uint32_t
getOpcode(ETargetToken token);


static SExpression*
maskAndShift(SExpression* expr, int srcHigh, int srcLow, int destLow) {
	int mask = ((2 << (srcHigh - srcLow)) - 1) << srcLow;
	expr = expr_And(expr, expr_Const(mask));
	if (destLow >= srcLow)
		expr = expr_Asl(expr, expr_Const(destLow - srcLow));
	else
		expr = expr_Asr(expr, expr_Const(srcLow - destLow));

	return expr;
}


static SExpression*
swizzleSFmtImmediate12(SExpression* expr) {
	if (expr == NULL)
		return expr_Const(0);

	if (expr != NULL) {
		SExpression* oldExpr = expr;
		expr =
			expr_Or(
				maskAndShift(expr_Copy(expr), 11, 5, 25),
				maskAndShift(expr_Copy(expr), 4, 0, 7)
			);
		expr_Free(oldExpr);
	}

	return expr;
}


static SExpression*
swizzleBFmtPcRelative13(SExpression* expr) {
	expr = expr_PcRelative(expr, 0);
	expr = expr_CheckRange(expr,-0x1000, 0xFFF);
	expr = expr_And(expr, expr_Const(0x1FFF));

	if (expr != NULL) {
		SExpression* oldExpr = expr;
		expr =
			expr_Or(
				expr_Or(
					maskAndShift(expr_Copy(expr), 12, 12, 31),
					maskAndShift(expr_Copy(expr), 10, 5, 25)
				),
				expr_Or(
					maskAndShift(expr_Copy(expr), 4, 1, 8),
					maskAndShift(expr_Copy(expr), 11, 11, 7)
				)
			);
		expr_Free(oldExpr);
	}

	return expr;
}


static SExpression*
swizzleJFmtPcRelative21(SExpression* expr) {
	expr = expr_PcRelative(expr, 0);
	expr = expr_CheckRange(expr, -0x100000, 0xFFFFF);
	expr = expr_And(expr, expr_Const(0x1FFFFF));

	if (expr != NULL) {
		SExpression* oldExpr = expr;
		expr =
			expr_Or(
				expr_Or(
					maskAndShift(expr_Copy(expr), 20, 20, 31),
					maskAndShift(expr_Copy(expr), 10, 1, 21)
				),
				expr_Or(
					maskAndShift(expr_Copy(expr), 11, 11, 20),
					maskAndShift(expr_Copy(expr), 19, 12, 12)
				)
			);
		expr_Free(oldExpr);
	}

	return expr;
}


static bool
parse_Register(int* reg) {
	if (T_V_REG_X0 <= lex_Context->token.id && lex_Context->token.id <= T_V_REG_X31) {
		*reg = lex_Context->token.id - T_V_REG_X0;
		if (*reg != 0 && opt_Current->machineOptions->reservedRegister == *reg) {
			err_Error(MERROR_REGISTER_RESERVED, *reg);
			return false;
		}
		parse_GetToken();
		return true;
	}

	return false;
}


static SExpression*
parse_Imm20(void) {
	SExpression* expr = parse_Expression(2);
	if (expr != NULL) {
		expr = expr_CheckRange(expr, -0x80000, 0xFFFFF);
		if (expr != NULL)
			return expr_And(expr, expr_Const(0xFFFFF));
	}

	return NULL;
}


static SExpression*
check_Signed12(SExpression* expr) {
	if (expr != NULL) {
		expr = expr_CheckRange(expr, -0x800, 0x7FF);
		if (expr != NULL)
			return expr_And(expr, expr_Const(0xFFF));
	}

	return NULL;
}


static SExpression*
parse_Expr12(uint32_t low, uint32_t high) {
	SExpression* expr = parse_Expression(2);
	return check_Signed12(expr);
}


static SExpression*
parse_Signed12(void) {
	return parse_Expr12(-0x800, 0x7FF);
}


static SExpression*
parse_Unsigned5(void) {
	SExpression* expr = parse_Expression(1);
	return expr_CheckRange(expr, 0x00, 0x1F);
}


static void
emit_U(uint32_t opcode, int rd, SExpression* imm) {
	if (imm != NULL) {
		SExpression* op = 
			expr_Or(
				expr_Asl(imm, expr_Const(12)),
				expr_Const(opcode | rd << 7));

		sect_OutputExpr32(op);
	} else {
		sect_OutputConst32(opcode | rd << 7);
	}
}


static bool
handle_U(uint32_t opcode) {
	int rd;

	if (parse_Register(&rd)
	&&  parse_ExpectComma()) {

		SExpression* imm = parse_Imm20();
		emit_U(opcode, rd, imm);
		return imm != NULL;
	}

	return false;
}


static bool
handle_B_offset(uint32_t opcode, int rs1, int rs2) {
	SExpression* address = parse_Expression(4);
	if (address != NULL) {
		opcode = opcode | rs2 << 20 | rs1 << 15;
		SExpression* op =
			expr_RiscvElf(
				R_RISCV_BRANCH,
				address,
				expr_Or(
					swizzleBFmtPcRelative13(expr_Copy(address)),
					expr_Const(opcode))
			);

		sect_OutputExprConst32(op, opcode);
		return true;
	}

	return false;
}


static bool
handle_B(uint32_t opcode) {
	int rs1, rs2;

	if (parse_Register(&rs1)
	&&  parse_ExpectComma()
	&&  parse_Register(&rs2)
	&&	parse_ExpectComma()) {

		return handle_B_offset(opcode, rs1, rs2);
	}

	return false;
}


static bool
handle_B_r(uint32_t opcode) {
	int rs1, rs2;

	if (parse_Register(&rs2)
	&&  parse_ExpectComma()
	&&  parse_Register(&rs1)
	&&	parse_ExpectComma()) {

		return handle_B_offset(opcode, rs1, rs2);
	}

	return false;
}


static void
emit_I(uint32_t opcode, int rd, int rs1, SExpression* imm) {
	opcode |= rd << 7 | rs1 << 15;

	if (imm == NULL) {
		sect_OutputConst32(opcode);
	} else {
		SExpression* op = 
			expr_Or(
				expr_Asl(imm, expr_Const(20)),
				expr_Const(opcode)
			);

		sect_OutputExpr32(op);
	}
}


static void
emit_S(uint32_t opcode, int rs1, int rs2, SExpression* imm) {
	SExpression* op = 
		expr_Or(
			swizzleSFmtImmediate12(imm),
			expr_Const(opcode | rs2 << 20 | rs1 << 15)
		);

	sect_OutputExpr32(op);
}


static bool
handle_I(uint32_t opcode, SExpression* (*parseImm)(void)) {
	int rd, rs1;

	if (parse_Register(&rd)
	&&  parse_ExpectComma()
	&&  parse_Register(&rs1)
	&&	parse_ExpectComma()) {

		SExpression* imm = parseImm();
		if (imm != NULL) {
			emit_I(opcode, rd, rs1, imm);
			return true;
		}
	}

	return false;
}


static bool
handle_I_2r(uint32_t opcode) {
	int rd, rs1;

	if (parse_Register(&rd)
	&&  parse_ExpectComma()
	&&  parse_Register(&rs1)) {

		emit_I(opcode, rd, rs1, NULL);
		return true;
	}

	return false;
}


static bool
handle_I_S(uint32_t opcode) {
	return handle_I(opcode, parse_Signed12);
}


static bool
handle_I_5(uint32_t opcode) {
	return handle_I(opcode, parse_Unsigned5);
}


static bool
parse_RegisterImmediate(int* reg, SExpression** imm, SExpression* (*checkOffset)(SExpression*)) {
	*imm = NULL;

	if (lex_Context->token.id == '(') {
		parse_GetToken();
		if (parse_Register(reg)) {
			return parse_ExpectChar(')');
		}
	}

	SExpression* expr = parse_Expression(4);
	if (lex_Context->token.id == '(') {
		parse_GetToken();
		if (parse_Register(reg)) {
			parse_ExpectChar(')');
			*imm = checkOffset(expr);
			return true;
		}
		return false;
	}

	*imm = expr;
	*reg = -1;
	return true;
}


static bool
parse_RegistersImmediate(int* reg1, int* reg2, SExpression** imm, SExpression* (*checkOffset)(SExpression* expr)) {
	if (parse_Register(reg1)
	&&  parse_ExpectComma()) {

		return parse_RegisterImmediate(reg2, imm, checkOffset);
	}

	return false;
}


static void
emit_LoadStore32Upper(uint32_t opcode, SExpression* imm, int rd) {
	SExpression* upper = 
		expr_Add(
			expr_And(
				expr_Asr(expr_Copy(imm), expr_Const(12)),
				expr_Const(0xFFFFF)),
			expr_And(
				expr_Asr(expr_Copy(imm), expr_Const(11)),
				expr_Const(1)
			)
		);

	emit_U(opcode, rd, upper);
}


static void
emit_Load32(SExpression* imm, int rd, uint32_t first_opcode, uint32_t second_opcode) {
	emit_LoadStore32Upper(first_opcode, imm, rd);
	emit_I(second_opcode, rd, rd, imm);
}


static void
emit_Store32(SExpression* imm, int rd, int rs, uint32_t first_opcode, uint32_t second_opcode) {
	emit_LoadStore32Upper(first_opcode, imm, rd);
	emit_S(second_opcode, rd, rs, imm);
}


static bool
handle_I_JR(uint32_t opcode) {
	int rs;
	if (parse_Register(&rs)) {
		emit_I(opcode, 0, rs, expr_Const(0));
		return true;
	}

	return false;
}


static bool
handle_I_Load(uint32_t opcode) {
	int rd, rs1;

	SExpression* imm = NULL;
	if (parse_RegistersImmediate(&rd, &rs1, &imm, check_Signed12)) {
		if (rs1 != -1) {
			emit_I(opcode, rd, rs1, imm);
		} else {
			if (expr_IsConstant(imm)) {
				if (imm->value.integer >= -0x800 && imm->value.integer <= 0x7FF)
					emit_I(opcode, rd, 0, imm);
				else
					emit_Load32(imm, rd, getOpcode(T_V_32I_LUI), opcode);
			} else {
				imm = expr_PcRelative(imm, 0);
				emit_Load32(imm, rd, getOpcode(T_V_32I_AUIPC), opcode);
			}
		}
		return true;
	}

	return false;
}


static bool
handle_JALR(uint32_t opcode) {
	int rs;

	if (parse_Register(&rs)) {
		SExpression* imm;
		int rd = 1;

		if (lex_Context->token.id == T_COMMA) {
			rd = rs;

			parse_GetToken();
			if (!parse_RegisterImmediate(&rs, &imm, check_Signed12)) {
				return false;
			}
		} else {
			imm = expr_Const(0);
		}

		emit_I(opcode, rd, rs, imm);
		return true;
	}

	return false;
}


static bool
handle_S(uint32_t opcode) {
	int rs1, rs2;

	SExpression* imm = NULL;
	if (parse_RegistersImmediate(&rs2, &rs1, &imm, check_Signed12)) {
		if (rs1 != -1) {
			emit_S(opcode, rs1, rs2, imm);
			return true;
		} else {
			if (lex_Context->token.id == ',') {
				parse_GetToken();
				if (!parse_Register(&rs1)) {
					return false;
				}
			} else {
				rs1 = 0;
			}
			if (expr_IsConstant(imm)) {
				if (imm->value.integer >= -0x800 && imm->value.integer <= 0x7FF) {
					emit_S(opcode, rs1, rs2, imm);
				} else {
					rs1 = opt_Current->machineOptions->reservedRegister;
					emit_Store32(imm, rs1, rs2, getOpcode(T_V_32I_LUI), opcode);
				}
				return true;
			} else {
				imm = expr_PcRelative(imm, 0);
				emit_Store32(imm, rs1, rs2, getOpcode(T_V_32I_AUIPC), opcode);
				return true;
			}
		}
	}

	return false;
}


static bool
handle_R(uint32_t opcode) {
	int rd, rs1, rs2;
	if (parse_Register(&rd)
	&&  parse_ExpectComma()
	&&  parse_Register(&rs1)
	&&	parse_ExpectComma()
	&&  parse_Register(&rs2)) {

		sect_OutputConst32(opcode | rd << 7 | rs2 << 20 | rs1 << 15);
		return true;
	}

	return false;
}


static bool
handle_R_2r(uint32_t opcode) {
	int rd, rs;
	if (parse_Register(&rd)
	&&  parse_ExpectComma()
	&&  parse_Register(&rs)) {

		sect_OutputConst32(opcode | rd << 7 | rs << 20);
		return true;
	}

	return false;
}


static bool
handle_R_2r2(uint32_t opcode) {
	int rd, rs;
	if (parse_Register(&rd)
	&&  parse_ExpectComma()
	&&  parse_Register(&rs)) {

		sect_OutputConst32(opcode | rd << 7 | rs << 15);
		return true;
	}

	return false;
}


static bool
tokenHasStringContent(void) {
	int id = lex_Context->token.id;
	return id != T_NUMBER && id != T_FLOAT && id != T_STRING;
}


static bool
parse_FenceSpec(uint32_t* spec) {
	bool r = false;
	const char* specString = "WROI";

	*spec = 0;
	while (tokenHasStringContent() && lex_Context->token.id != T_COMMA && lex_Context->token.id != '\n') {
		for (uint32_t i = 0; i < lex_Context->token.length; ++i) {
			char ch = toupper(lex_Context->token.value.string[i]);
			char* pspec = strchr(specString, ch);
			if (pspec != NULL) {
				uint32_t bit = 1 << (pspec - specString);
				if ((bit & *spec) == 0) {
					r = true;
					*spec |= bit;
					continue;
				}
			}
			parse_GetToken();
			err_Error(MERROR_ILLEGAL_FENCE);
			return false;
		}
		parse_GetToken();
	}

	return r;
}


static bool
handle_FENCE(uint32_t opcode) {
	uint32_t pred, succ;

	if (parse_FenceSpec(&succ)) {
		if (!parse_ExpectComma()
		||  !parse_FenceSpec(&pred)) {

			return false;
		}
	} else {
		pred = 0xF;
		succ = 0xF;
	}

	sect_OutputConst32(opcode | succ << 24 | pred << 20);
	return true;
}


static bool
handle_J_OFFSET(uint32_t opcode) {
	SExpression* address = parse_Expression(4);
	if (address != NULL) {
		SExpression* op = 
			expr_Or(
				swizzleJFmtPcRelative21(address),
				expr_Const(opcode)
			);

		sect_OutputExpr32(op);
		return true;
	}

	return false;
}


static bool
handle_JAL(uint32_t opcode) {
	int rd;
	if (parse_Register(&rd)) {
		if (!parse_ExpectComma())
			return false;
	} else {
		rd = 1;
	} 

	return handle_J_OFFSET(opcode | rd << 7);
}


static bool
handle_Implicit(uint32_t opcode) {
	sect_OutputConst32(opcode);
	return true;
}


static bool
handle_BZ(uint32_t opcode) {
	int rs;

	if (parse_Register(&rs)
	&&  parse_ExpectComma()) {

		return handle_B_offset(opcode, rs, 0);
	}

	return false;
}


static bool
handle_BZ_r(uint32_t opcode) {
	int rs;

	if (parse_Register(&rs)
	&&  parse_ExpectComma()) {

		return handle_B_offset(opcode, 0, rs);
	}

	return false;
}


static void
emit_LI(int rd, SExpression* imm) {
	if (expr_IsConstant(imm)) {
		if (imm->value.integer >= -2048 && imm->value.integer <= 2047) {
			emit_I(getOpcode(T_V_32I_ADDI), rd, 0, imm);
			return;
		} else if ((imm->value.integer & 0xFFF) == 0) {
			uint32_t opcode = getOpcode(T_V_32I_LUI);
			opcode |= rd << 7 | imm->value.integer;

			sect_OutputConst32(opcode);
			return;
		}
	}

	emit_Load32(imm, rd, getOpcode(T_V_32I_LUI), getOpcode(T_V_32I_ADDI));
}


static bool
handle_LI(uint32_t opcode) {
	int rd;

	if (parse_Register(&rd)
	&&  parse_ExpectComma()) {

		SExpression* imm = parse_Expression(4);
		if (imm != NULL) {
			emit_LI(rd, imm);
			return true;
		}
	}

	return false;
}


static bool
emit_LLA(int rd, SExpression* expr) {
	emit_LI(rd, expr);
	return true;
}


static bool
handle_LA(uint32_t opcode) {
	int rd;

	if (parse_Register(&rd)
	&&  parse_ExpectComma()) {

		SExpression* expr = parse_Expression(4);
		if (expr != NULL) {
			if (opt_Current->machineOptions->pic) {
				/* PIC mode */
				err_Error(ERROR_NOT_IMPLEMENTED, "LA (PIC mode)");
			} else {
				/* PIC mode disabled */
				return emit_LLA(rd, expr);
			}
			return true;
		}
	}

	return false;
}


static bool
handle_LLA(uint32_t opcode) {
	int rd;

	if (parse_Register(&rd)
	&&  parse_ExpectComma()) {

		SExpression* expr = parse_SymbolExpression();
		emit_LLA(rd, expr);
		return true;
	}

	return false;
}


static bool
handle_CALL(uint32_t opcode) {
	SExpression* expr = parse_SymbolExpression();
	emit_Load32(expr, 1 /* x1 */, getOpcode(T_V_32I_AUIPC), getOpcode(T_V_32I_JALR));

	return true;
}


static bool
handle_TAIL(uint32_t opcode) {
	SExpression* expr = parse_SymbolExpression();
	emit_LoadStore32Upper(getOpcode(T_V_32I_AUIPC), expr, 6 /* x6 */);
	emit_I(getOpcode(T_V_32I_JALR), 0, 6, expr);

	return true;
}


#define OP_R(funct7, funct3, opcode) ((funct7) << 25 | (funct3) << 12 | (opcode))
#define OP_I(funct3, opcode)         ((funct3) << 12 | (opcode))
#define OP_I_rs(rs, funct3, opcode)  ((rs << 15) | (funct3) << 12 | (opcode))
#define OP_I_regs(imm, funct3, opcode) ((((uint32_t)(imm) << 20) | (funct3) << 12 | (opcode)))
#define OP_B(funct3, opcode)         ((funct3) << 12 | (opcode))
#define OP_S(funct3, opcode)         ((funct3) << 12 | (opcode))
#define OP_U(opcode)                 (opcode)
#define OP_J(opcode)                 (opcode)
#define OP_UNKNOWN()                 0


static SParser
g_Parsers[T_V_LAST - T_V_32I_ADD + 1] = {
	{ OP_R(0x00, 0x00, 0x33), false, handle_R   },	/* ADD */
	{ OP_I(      0x00, 0x13), false, handle_I_S },	/* ADDI */
	{ OP_R(0x00, 0x07, 0x33), false, handle_R   },	/* AND */
	{ OP_I(      0x07, 0x13), false, handle_I_S },	/* ANDI */
	{ OP_U(            0x17), false, handle_U   },	/* AUIPC */
	{ OP_B(      0x00, 0x63), false, handle_B   },	/* BEQ */
	{ OP_B(      0x05, 0x63), false, handle_B   },	/* BGE */
	{ OP_B(      0x07, 0x63), false, handle_B   },	/* BGEU */
	{ OP_B(      0x04, 0x63), false, handle_B   },	/* BLT */
	{ OP_B(      0x06, 0x63), false, handle_B   },	/* BLTU */
	{ OP_B(      0x01, 0x63), false, handle_B    },	/* BNE */
	{ OP_I(      0x00, 0x0F), false, handle_FENCE   },	/* FENCE */
	{ OP_J(            0x6F), false, handle_JAL  },	/* JAL */
	{ OP_I(      0x00, 0x67), false, handle_JALR },	/* JALR */
	{ OP_I(      0x00, 0x03), false, handle_I_Load },	/* LB */
	{ OP_I(      0x04, 0x03), false, handle_I_Load },	/* LBU */
	{ OP_I(      0x01, 0x03), false, handle_I_Load },	/* LH */
	{ OP_I(      0x05, 0x03), false, handle_I_Load },	/* LHU */
	{ OP_U(            0x37), false, handle_U   },	/* LUI */
	{ OP_I(      0x02, 0x03), false, handle_I_Load },	/* LW */
	{ OP_R(0x00, 0x06, 0x33), false, handle_R   },	/* OR */
	{ OP_I(      0x06, 0x13), false, handle_I_S },	/* ORI */
	{ OP_S(      0x00, 0x23), false, handle_S   },	/* SB */
	{ OP_S(      0x01, 0x23), false, handle_S   },	/* SH */
	{ OP_R(0x00, 0x01, 0x33), false, handle_R   },	/* SLL */
	{ OP_R(0x00, 0x01, 0x13), false, handle_I_5 },	/* SLLI */
	{ OP_R(0x00, 0x02, 0x33), false, handle_R   },	/* SLT */
	{ OP_I(      0x02, 0x13), false, handle_I_S },	/* SLTI */
	{ OP_I(      0x03, 0x13), false, handle_I_S },	/* SLTIU */
	{ OP_R(0x00, 0x03, 0x33), false, handle_R   },	/* SLTU */
	{ OP_R(0x20, 0x05, 0x33), false, handle_R   },	/* SRA */
	{ OP_R(0x20, 0x05, 0x13), false, handle_I_5 },	/* SRAI */
	{ OP_R(0x00, 0x05, 0x33), false, handle_R   },	/* SRL */
	{ OP_R(0x00, 0x05, 0x13), false, handle_I_5 },	/* SRLI */
	{ OP_R(0x20, 0x00, 0x33), false, handle_R   },	/* SUB */
	{ OP_S(      0x02, 0x23), false, handle_S   },	/* SW */
	{ OP_R(0x00, 0x04, 0x33), false, handle_R   },	/* XOR */
	{ OP_I(      0x04, 0x13), false, handle_I_S },	/* XORI */

	/* Pseudo instructions */
	{ OP_J   (            0x6F), false, handle_J_OFFSET    },	/* J */
	{ OP_I   (      0x00, 0x67), false, handle_I_JR        },	/* JALR */
	{ OP_I_rs(0x01, 0x00, 0x67), false, handle_Implicit },	/* RET */

	{ OP_B(      0x00, 0x63), false, handle_BZ   },	/* BEQZ */
	{ OP_B(      0x01, 0x63), false, handle_BZ   },	/* BNEZ */
	{ OP_B(      0x05, 0x63), false, handle_BZ_r },	/* BLEZ */
	{ OP_B(      0x05, 0x63), false, handle_BZ   },	/* BGEZ */
	{ OP_B(      0x04, 0x63), false, handle_BZ   },	/* BLTZ */
	{ OP_B(      0x04, 0x63), false, handle_BZ_r },	/* BGTZ */

	{ OP_B(      0x04, 0x63), false, handle_B_r  },	/* BGT */
	{ OP_B(      0x05, 0x63), false, handle_B_r  },	/* BLE */
	{ OP_B(      0x06, 0x63), false, handle_B_r  },	/* BLTU */
	{ OP_B(      0x07, 0x63), false, handle_B_r  },	/* BLEU */

	{ OP_I_regs(0x000, 0x00, 0x13), false, handle_I_2r },	/* MV */
	{ OP_R     ( 0x20, 0x00, 0x33), false, handle_R_2r   },	/* NEG */
	{ OP_I_regs(0xFFF, 0x04, 0x13), false, handle_I_2r },	/* NOT */
	{ OP_I     (       0x00, 0x13), false, handle_Implicit },	/* NOP */

	{ OP_I_regs(0x001, 0x03, 0x13), false, handle_I_2r },	/* SEQZ */
	{ OP_R     ( 0x00, 0x03, 0x33), false, handle_R_2r   },	/* SNEZ */
	{ OP_R     ( 0x00, 0x02, 0x33), false, handle_R_2r2   },	/* SLTZ */
	{ OP_R     ( 0x00, 0x02, 0x33), false, handle_R_2r   },	/* SGTZ */

	{ OP_I_regs(0x001, 0x00, 0x73), false, handle_Implicit },	/* EBREAK */
	{ OP_I_regs(0x000, 0x00, 0x73), false, handle_Implicit },	/* ECALL */

	{ OP_UNKNOWN(), false, handle_LI   },	/* LI */
	{ OP_UNKNOWN(), false, handle_LA   },	/* LA */
	{ OP_UNKNOWN(), false, handle_LLA  },	/* LLA */
	{ OP_UNKNOWN(), false, handle_CALL  },	/* CALL */
	{ OP_UNKNOWN(), false, handle_TAIL  },	/* TAIL */

	{ 0x10200073, true, handle_Implicit },	/* SRET */
	{ 0x30200073, true, handle_Implicit },	/* MRET */
	{ 0x10500073, true, handle_Implicit },	/* WFI */


};


static uint32_t
getOpcode(ETargetToken token) {
	return g_Parsers[token - T_V_32I_ADD].baseOpcode;
}


bool
v_ParseIntegerInstruction(void) {
	if (T_V_32I_ADD <= lex_Context->token.id && lex_Context->token.id <= T_V_LAST) {
		ETargetToken token = lex_Context->token.id;
		SParser* parser = &g_Parsers[token - T_V_32I_ADD];

		parse_GetToken();

		if (parser->privileged && !opt_Current->machineOptions->privileged) {
			err_Error(MERROR_PRIVILEGED);
		} else {
			if (!parser->parser(parser->baseOpcode)) {
				return err_Error(ERROR_OPERAND);
			}
		}

		return true;
	}

	return false;
}
