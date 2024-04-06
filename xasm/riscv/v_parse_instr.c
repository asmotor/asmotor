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

static SParser
g_Parsers[T_V_ADD - T_V_ADD + 1] = {
	{ OP_R(0x00, 0x00, 0x33), handle_R },	/* ADD */
};


bool
v_ParseIntegerInstruction(void) {
	if (T_V_ADD <= lex_Context->token.id && lex_Context->token.id <= T_V_ADD) {
		ETargetToken token = lex_Context->token.id;
		SParser* parser = &g_Parsers[token - T_V_ADD];

		parse_GetToken();

		if (!parser->parser(parser->baseOpcode, parser->instructionFormat)) {
			return err_Error(ERROR_OPERAND);
		}

		return true;
	}

	return false;
}
