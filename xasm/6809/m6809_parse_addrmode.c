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
#include "lexer.h"
#include "parse.h"
#include "parse_expression.h"

#include "m6809_parse.h"
#include "m6809_tokens.h"

static bool
parseExpressionMode(SAddressingMode* addrMode, uint32_t mode) {
	parse_GetToken();
	addrMode->mode = mode;
	if ((addrMode->expr = parse_Expression(2)) == NULL) {
		err_Error(ERROR_EXPECT_EXPR);
		return false;
	}
	return true;
}

static bool
parseRegisterIndexed(SAddressingMode* addrMode, uint32_t mode, uint8_t postbyte) {
	parse_GetToken();
	if (lex_Context->token.id == ',') {
		parse_GetToken();
		int r = lex_Context->token.id - T_6809_REG_X;
		if (r >= 0 && r <= 3) {
			parse_GetToken();

			r <<= 5;
			addrMode->mode = mode;
			addrMode->indexed_post_byte = postbyte | r;
			return true;
		}
	}
	return false;
}

uint8_t
parseRegisterListBit(ETargetToken allowedStack, ETargetToken disallowedStack) {
	ETargetToken t = lex_Context->token.id;
	switch (t) {
		case T_6809_REG_CCR:
			return 0x01;
		case T_6809_REG_A:
			return 0x02;
		case T_6809_REG_B:
			return 0x04;
		case T_6809_REG_DPR:
			return 0x08;
		case T_6809_REG_X:
			return 0x10;
		case T_6809_REG_Y:
			return 0x20;
		default:
			if (t == allowedStack) {
				return 0x40;
			} else if (t == disallowedStack) {
				err_Error(ERROR_OPERAND);
			}
			break;
		case T_6809_REG_PCR:
			return 0x80;
	}

	return 0;
}

bool
m6809_ParseAddressingMode(SAddressingMode* addrMode, uint32_t allowedModes) {
	SLexerContext bm;
	lex_Bookmark(&bm);

	if ((allowedModes & MODE_IMMEDIATE) && lex_Context->token.id == '#')
		return parseExpressionMode(addrMode, MODE_IMMEDIATE);

	if ((allowedModes & MODE_INDIRECT_MODIFIER) && lex_Context->token.id == '[') {
		parse_GetToken();
		if (m6809_ParseAddressingMode(addrMode, MODE_ALLOWED_INDIRECT)) {
			if (parse_ExpectChar(']')) {
				if (addrMode->mode & (MODE_ADDRESS | MODE_EXTENDED)) {
					addrMode->mode = MODE_EXTENDED_INDIRECT | MODE_INDIRECT_MODIFIER;
					addrMode->indexed_post_byte = 0x9F;
				} else {
					addrMode->indexed_post_byte |= 0x10;
				}
				addrMode->mode |= MODE_INDIRECT_MODIFIER;
				return true;
			}
		}
		return false;
	}

	if (allowedModes & (MODE_REGISTER_LIST_S | MODE_REGISTER_LIST_U)) {
		ETargetToken allowedStack = allowedModes & MODE_REGISTER_LIST_S ? T_6809_REG_S : T_6809_REG_U;
		ETargetToken disallowedStack = allowedModes & MODE_REGISTER_LIST_S ? T_6809_REG_U : T_6809_REG_S;
		uint8_t bits = 0;
		uint8_t bit;
		while ((bit = parseRegisterListBit(allowedStack, disallowedStack)) != 0) {
			parse_GetToken();
			bits |= bit;
			if (lex_Context->token.id != T_COMMA) {
				break;
			}
			parse_GetToken();
		}
		addrMode->indexed_post_byte = bits;
		addrMode->mode = allowedModes & (MODE_REGISTER_LIST_S | MODE_REGISTER_LIST_U);
		return true;
	}

	if ((allowedModes & (MODE_INDEXED_R_INC1 | MODE_INDEXED_R_INC2 | MODE_INDEXED_R_DEC1 | MODE_INDEXED_R_DEC2 | MODE_INDEXED_R)) &&
	    lex_Context->token.id == ',') {
		parse_GetToken();

		int minus = 0;
		while (lex_Context->token.id == T_OP_SUBTRACT) {
			++minus;
			parse_GetToken();
		}

		int r = lex_Context->token.id - T_6809_REG_X;
		if (r >= 0 && r <= 3) {
			parse_GetToken();

			int plus = 0;
			while (lex_Context->token.id == T_OP_ADD) {
				++plus;
				parse_GetToken();
			}

			r <<= 5;

			if (minus == 0 && plus == 1 && (allowedModes & MODE_INDEXED_R_INC1)) {
				addrMode->mode = MODE_INDEXED_R_INC1;
				addrMode->indexed_post_byte = 0x80 | r;
				return true;
			} else if (minus == 0 && plus == 2 && (allowedModes & MODE_INDEXED_R_INC2)) {
				addrMode->mode = MODE_INDEXED_R_INC2;
				addrMode->indexed_post_byte = 0x81 | r;
				return true;
			} else if (minus == 1 && plus == 0 && (allowedModes & MODE_INDEXED_R_DEC1)) {
				addrMode->mode = MODE_INDEXED_R_DEC1;
				addrMode->indexed_post_byte = 0x82 | r;
				return true;
			} else if (minus == 2 && plus == 0 && (allowedModes & MODE_INDEXED_R_DEC2)) {
				addrMode->mode = MODE_INDEXED_R_DEC2;
				addrMode->indexed_post_byte = 0x83 | r;
				return true;
			} else if (minus == 0 && plus == 0 && (allowedModes & MODE_INDEXED_R)) {
				addrMode->mode = MODE_INDEXED_R;
				addrMode->indexed_post_byte = 0x84 | r;
				return true;
			}
		}

		return false;
	}

	if ((allowedModes & MODE_INDEXED_R_A) && lex_Context->token.id == T_6809_REG_A) {
		return parseRegisterIndexed(addrMode, MODE_INDEXED_R_A, 0x86);
	}

	if ((allowedModes & MODE_INDEXED_R_B) && lex_Context->token.id == T_6809_REG_B) {
		return parseRegisterIndexed(addrMode, MODE_INDEXED_R_B, 0x85);
	}

	if ((allowedModes & MODE_INDEXED_R_D) && lex_Context->token.id == T_6809_REG_D) {
		return parseRegisterIndexed(addrMode, MODE_INDEXED_R_D, 0x8B);
	}

	bool force_extended = false;
	if (lex_Context->token.id == T_OP_GREATER_THAN) {
		force_extended = true;
		parse_GetToken();
	}

	bool force_direct = false;
	if (lex_Context->token.id == T_OP_LESS_THAN) {
		force_direct = true;
		parse_GetToken();
	}

	addrMode->expr = parse_Expression(2);
	if (allowedModes &
	    (MODE_INDEXED_R_5BIT | MODE_INDEXED_R_8BIT | MODE_INDEXED_R_16BIT | MODE_INDEXED_PC_8BIT | MODE_INDEXED_PC_16BIT)) {
		if (lex_Context->token.id == ',') {
			parse_GetToken();

			if (lex_Context->token.id == T_6809_REG_PCR) {
				if ((allowedModes & MODE_INDEXED_PC_8BIT) && force_direct) {
					parse_GetToken();
					addrMode->mode = MODE_INDEXED_PC_8BIT;
					addrMode->indexed_post_byte = 0x8C;
					return true;
				}
				if (allowedModes & MODE_INDEXED_PC_16BIT) {
					parse_GetToken();
					addrMode->mode = MODE_INDEXED_PC_16BIT;
					addrMode->indexed_post_byte = 0x8D;
					return true;
				}
				return false;
			}

			bool force_5bit = false;
			bool force_8bit = force_direct;
			bool force_16bit = force_extended;

			if (!force_8bit && !force_16bit && expr_IsConstant(addrMode->expr)) {
				if (addrMode->expr->value.integer >= -16 && addrMode->expr->value.integer <= 15) {
					force_5bit = true;
				} else if (addrMode->expr->value.integer >= -128 && addrMode->expr->value.integer <= 127) {
					force_8bit = true;
				} else if (addrMode->expr->value.integer >= -32768 && addrMode->expr->value.integer <= 32767) {
					force_16bit = true;
				}
			}

			int r = lex_Context->token.id - T_6809_REG_X;
			if (r >= 0 && r <= 3) {
				r <<= 5;

				if ((allowedModes & MODE_INDEXED_R_5BIT) && force_5bit) {
					parse_GetToken();

					if (addrMode->expr->value.integer == 0) {
						addrMode->mode = MODE_INDEXED_R;
						addrMode->indexed_post_byte = r | 0x84;
					} else {
						addrMode->mode = MODE_INDEXED_R_5BIT;
						addrMode->indexed_post_byte = r | (addrMode->expr->value.integer & 0x1F);
					}
					return true;
				}
				if ((allowedModes & MODE_INDEXED_R_8BIT) && (force_8bit || force_5bit)) {
					parse_GetToken();
					addrMode->mode = MODE_INDEXED_R_8BIT;
					addrMode->indexed_post_byte = 0x88;
					return true;
				}
				if ((allowedModes & MODE_INDEXED_R_16BIT) && (force_16bit || force_8bit || force_5bit)) {
					parse_GetToken();
					addrMode->mode = MODE_INDEXED_R_16BIT;
					addrMode->indexed_post_byte = 0x89;
					return true;
				}
			}
		}
	}

	if ((allowedModes & MODE_EXTENDED) && force_extended) {
		addrMode->mode = MODE_EXTENDED;
		return true;
	}

	if ((allowedModes & MODE_DIRECT) && force_direct) {
		addrMode->mode = MODE_DIRECT;
		return true;
	}

	if ((allowedModes & MODE_ADDRESS) && addrMode->expr != NULL) {
		addrMode->mode = MODE_ADDRESS;
		return true;
	}

	lex_Goto(&bm);

	return (allowedModes & MODE_NONE);
}
