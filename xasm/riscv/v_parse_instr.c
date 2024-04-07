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
#include "parse.h"
#include "parse_expression.h"

#include "section.h"
#include "v_tokens.h"
#include "v_errors.h"


typedef enum InstructionFormat {
	FMT_R,
	FMT_I,
	FMT_S,
	FMT_B,
	FMT_U,
	FMT_J,
} EInstructionFormat;


typedef struct {
	int rd, rs1, rs2;
} SAddressingMode;

typedef struct Parser {
	uint32_t baseOpcode;
	EInstructionFormat instructionFormat;
	bool (*parser)(uint32_t opcode, EInstructionFormat fmt);
} SParser;


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
shufflePcRelative13(SExpression* expr) {
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
shufflePcRelative21(SExpression* expr) {
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
		parse_GetToken();
		return true;
	}

	return false;
}


static SExpression*
parse_Unsigned20(void) {
	SExpression* expr = parse_Expression(2);
	if (expr != NULL) {
		uint32_t high = (1 << 20) - 1;
		expr = expr_CheckRange(expr, 0, high);
		if (expr != NULL)
			return expr_And(expr, expr_Const(high));
	}

	return NULL;
}


static SExpression*
parse_Expr12(uint32_t low, uint32_t high) {
	SExpression* expr = parse_Expression(2);
	if (expr != NULL) {
		expr = expr_CheckRange(expr, low, high);
		if (expr != NULL)
			return expr_And(expr, expr_Const(0x3FFF));
	}

	return NULL;
}


static SExpression*
parse_Signed12(void) {
	return parse_Expr12(-2048, 2047);
}


static SExpression*
parse_Unsigned12(void) {
	return parse_Expr12(0, 4095);
}


static bool
handle_U(uint32_t opcode, EInstructionFormat fmt) {
	int rd;

	if (parse_Register(&rd)
	&&  parse_ExpectComma()) {

		SExpression* imm = parse_Unsigned20();
		if (imm != NULL) {
			SExpression* op = 
				expr_Or(
					expr_Asl(imm, expr_Const(12)),
					expr_Const(opcode | rd << 7));

			sect_OutputExpr32(op);
		}
		return true;
	}

	return false;
}


static bool
handle_B(uint32_t opcode, EInstructionFormat fmt) {
	int rs1, rs2;

	if (parse_Register(&rs1)
	&&  parse_ExpectComma()
	&&  parse_Register(&rs2)
	&&	parse_ExpectComma()) {

		SExpression* address = parse_Expression(4);
		if (address != NULL) {
			SExpression* op = 
				expr_Or(
					shufflePcRelative13(address),
					expr_Const(opcode | rs2 << 20 | rs1 << 15)
				);

			sect_OutputExpr32(op);
		}
		return true;
	}

	return false;
}


static bool
handle_I(uint32_t opcode, EInstructionFormat fmt, SExpression* (*parseImm)(void)) {
	int rd, rs1;

	if (parse_Register(&rd)
	&&  parse_ExpectComma()
	&&  parse_Register(&rs1)
	&&	parse_ExpectComma()) {

		SExpression* imm = parseImm();
		if (imm != NULL) {
			SExpression* op = 
				expr_Or(
					expr_Asl(imm, expr_Const(20)),
					expr_Const(opcode | rd << 7 | rs1 << 15)
				);

			sect_OutputExpr32(op);
		}
		return true;
	}

	return false;
}


static bool
handle_I_S(uint32_t opcode, EInstructionFormat fmt) {
	return handle_I(opcode, fmt, parse_Signed12);
}


static bool
handle_I_U(uint32_t opcode, EInstructionFormat fmt) {
	return handle_I(opcode, fmt, parse_Unsigned12);
}


static bool
handle_R(uint32_t opcode, EInstructionFormat fmt) {
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
tokenHasStringContent(void) {
	int id = lex_Context->token.id;
	return id != T_NUMBER && id != T_FLOAT && id != T_STRING;
}


static bool
parse_FenceSpec(uint32_t* spec) {
	*spec = 0;
	const char* specString = "WROI";
	while (tokenHasStringContent() && lex_Context->token.id != T_COMMA && lex_Context->token.id != '\n') {
		for (uint32_t i = 0; i < lex_Context->token.length; ++i) {
			char ch = toupper(lex_Context->token.value.string[i]);
			char* pspec = strchr(specString, ch);
			if (pspec != NULL) {
				uint32_t bit = 1 << (pspec - specString);
				if ((bit & *spec) == 0) {
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
	return true;
}


static bool
handle_FENCE(uint32_t opcode, EInstructionFormat fmt) {
	uint32_t pred, succ;
	if (parse_FenceSpec(&succ)
	&&  parse_ExpectComma()
	&&  parse_FenceSpec(&pred)) {

		sect_OutputConst32(opcode | succ << 24 | pred << 20);
		return true;
	}

	return false;
}


static bool
handle_J(uint32_t opcode, EInstructionFormat fmt) {
	int rd;
	if (parse_Register(&rd)
	&&  parse_ExpectComma()) {

		SExpression* address = parse_Expression(4);
		if (address != NULL) {
			SExpression* op = 
				expr_Or(
					shufflePcRelative21(address),
					expr_Const(opcode | rd << 7)
				);

			sect_OutputExpr32(op);
			return true;
		}
	}

	return false;
}


#define OP_R(funct7, funct3, opcode) ((funct7) << 25 | (funct3) << 12 | (opcode)),FMT_R
#define OP_I(funct3, opcode)         ((funct3) << 12 | (opcode)),FMT_I
#define OP_B(funct3, opcode)         ((funct3) << 12 | (opcode)),FMT_B
#define OP_U(opcode)                 (opcode),FMT_U
#define OP_J(opcode)                 (opcode),FMT_J


static SParser
g_Parsers[T_V_LAST - T_V_32I_ADD + 1] = {
	{ OP_R(0x00, 0x00, 0x33), handle_R   },	/* ADD */
	{ OP_I(      0x00, 0x13), handle_I_S },	/* ADDI */
	{ OP_R(0x00, 0x07, 0x33), handle_R   },	/* AND */
	{ OP_I(      0x07, 0x13), handle_I_U },	/* ANDI */
	{ OP_U(            0x17), handle_U   },	/* AUIPC */
	{ OP_B(      0x00, 0x63), handle_B   },	/* BEQ */
	{ OP_B(      0x05, 0x63), handle_B   },	/* BGE */
	{ OP_B(      0x07, 0x63), handle_B   },	/* BGEU */
	{ OP_B(      0x04, 0x63), handle_B   },	/* BLT */
	{ OP_B(      0x06, 0x63), handle_B   },	/* BLTU */
	{ OP_B(      0x01, 0x63), handle_B   },	/* BNE */
	{ OP_I(      0x00, 0x0F), handle_FENCE   },	/* FENCE */
	{ OP_J(            0x6F), handle_J   },	/* JAL */
};


bool
v_ParseIntegerInstruction(void) {
	if (T_V_32I_ADD <= lex_Context->token.id && lex_Context->token.id <= T_V_LAST) {
		ETargetToken token = lex_Context->token.id;
		SParser* parser = &g_Parsers[token - T_V_32I_ADD];

		parse_GetToken();

		if (!parser->parser(parser->baseOpcode, parser->instructionFormat)) {
			return err_Error(ERROR_OPERAND);
		}

		return true;
	}

	return false;
}
