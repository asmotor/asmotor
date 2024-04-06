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

#include "errors.h"
#include "parse.h"
#include "parse_expression.h"

#include "section.h"
#include "v_tokens.h"


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


#define OP_R(funct7, funct3, opcode) ((funct7) << 25 | (funct3) << 12 | (opcode)),FMT_R
#define OP_I(funct3, opcode)         ((funct3) << 12 | (opcode)),FMT_I
#define OP_U(opcode)                 (opcode),FMT_U


static SParser
g_Parsers[T_V_LAST - T_V_32I_ADD + 1] = {
	{ OP_R(0x00, 0x00, 0x33), handle_R   },	/* ADD */
	{ OP_I(      0x00, 0x13), handle_I_S },	/* ADDI */
	{ OP_R(0x00, 0x07, 0x33), handle_R   },	/* AND */
	{ OP_I(      0x07, 0x13), handle_I_U },	/* ANDI */
	{ OP_U(            0x17), handle_U   },	/* AUIPC */
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
