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
#include <stddef.h>

#include "errors.h"
#include "expression.h"
#include "lexer.h"
#include "lexer_context.h"
#include "options.h"
#include "parse.h"
#include "parse_expression.h"
#include "tokens.h"

#include "x65_errors.h"
#include "x65_options.h"
#include "x65_parse.h"
#include "x65_tokens.h"

static SExpression*
maskExpression(SExpression* expr, bool imm16bit) {
	return expr_And(expr, expr_Const(imm16bit ? 0xFFFF : 0xFF));
}

static SExpression*
parseImmExpression(bool imm16bit) {
	if (lex_Context->token.id == T_OP_LESS_THAN) {
		SExpression* expr;
		parse_GetToken();
		if (lex_Context->token.id == T_OP_GREATER_THAN) {
			parse_GetToken();

			expr = parse_Expression(2);
			SExpression* high = expr_And(expr_Copy(expr), expr_Const(0xFF00));
			SExpression* low = expr;
			expr = expr_Or(                   //
			    expr_Asl(low, expr_Const(8)), //
			    expr_Asr(high, expr_Const(8)));
		} else {
			expr = parse_Expression(2);
		}
		return maskExpression(expr, imm16bit);
	} else if (lex_Context->token.id == T_OP_GREATER_THAN) {
		parse_GetToken();
		return maskExpression(expr_Asr(parse_Expression(2), expr_Const(8)), imm16bit);
	} else if (lex_Context->token.id == T_OP_BITWISE_XOR) {
		parse_GetToken();
		return maskExpression(expr_Asr(parse_Expression(2), expr_Const(16)), imm16bit);
	}

	return parse_Expression(2);
}

static bool
x65_ParseAddressingModeCore(SAddressingMode* addrMode, uint32_t allowedModes, EImmediateSize immSize) {
	bool imm16bit = false;

	switch (immSize) {
		case IMM_16_BIT:
			imm16bit = true;
			break;
		case IMM_ACC:
			imm16bit = opt_Current->machineOptions->m16;
			break;
		case IMM_INDEX:
			imm16bit = opt_Current->machineOptions->x16;
			break;
		default:
			imm16bit = false;
			break;
	}

	SLexerContext bm;
	lex_Bookmark(&bm);

	addrMode->expr = NULL;
	addrMode->expr2 = NULL;
	addrMode->expr3 = NULL;
	addrMode->size_forced = false;

	if ((allowedModes & MODE_A) && lex_Context->token.id == T_6502_REG_A) {
		parse_GetToken();
		addrMode->mode = MODE_A;
		addrMode->expr = NULL;
		return true;
	}

	if ((allowedModes & MODE_45GS02_Q) && lex_Context->token.id == T_45GS02_REG_Q) {
		parse_GetToken();
		addrMode->mode = MODE_45GS02_Q;
		addrMode->expr = NULL;
		return true;
	}

	if ((allowedModes & (MODE_IMM | MODE_IMM_IMM)) && lex_Context->token.id == '#') {
		parse_GetToken();
		addrMode->expr = parseImmExpression(imm16bit);
		if (addrMode->expr != NULL) {
			addrMode->mode = MODE_IMM;
			if ((allowedModes & MODE_IMM_IMM) && lex_Context->token.id == ',') {
				parse_GetToken();
				if (lex_Context->token.id == '#') {
					parse_GetToken();
					addrMode->expr2 = parseImmExpression(imm16bit);
					if (addrMode->expr2 != NULL) {
						addrMode->mode = MODE_IMM_IMM;
						return true;
					}
				}
			} else {
				return true;
			}
		}

		return false;
	}

	if ((allowedModes & (MODE_IND_ZP_X | MODE_IND_ZP_Y | MODE_4510_IND_ZP_Z | MODE_IND_ABS_X | MODE_816_IND_DISP_S_Y)) &&
	    lex_Context->token.id == '(') {
		parse_GetToken();
		addrMode->expr = parse_Expression(2);

		if (addrMode->expr != NULL) {
			if (lex_Context->token.id == ',') {
				parse_GetToken();
				if (lex_Context->token.id == T_6502_REG_X) {
					parse_GetToken();
					if (parse_ExpectChar(')')) {
						addrMode->mode =
						    allowedModes & (MODE_IND_ZP_X | MODE_IND_ABS_X); /* only one of these can be allowed at the same time */
						return true;
					}
				}
				if (lex_Context->token.id == T_65816_REG_S) {
					parse_GetToken();
					if (parse_ExpectChar(')') && parse_ExpectChar(',')) {
						if (lex_Context->token.id == T_6502_REG_Y) {
							parse_GetToken();
							addrMode->mode = MODE_816_IND_DISP_S_Y;
							return true;
						}
					}
				}
			} else if (lex_Context->token.id == ')') {
				parse_GetToken();
				if (lex_Context->token.id == ',') {
					parse_GetToken();
					if (lex_Context->token.id == T_6502_REG_Y) {
						parse_GetToken();
						addrMode->mode = MODE_IND_ZP_Y;
						return true;
					} else if ((allowedModes & MODE_4510_IND_ZP_Z) && lex_Context->token.id == T_4510_REG_Z) {
						parse_GetToken();
						addrMode->mode = MODE_4510_IND_ZP_Z;
						return true;
					}
				}
			}
		}

		lex_Goto(&bm);
	}

	if ((allowedModes & (MODE_816_LONG_IND_ZP | MODE_816_LONG_IND_ZP_Y | MODE_816_LONG_IND_ABS | MODE_45GS02_IND_ZP_QUAD |
	                     MODE_45GS02_IND_ZP_Z_QUAD)) &&
	    (lex_Context->token.id == '[')) {
		parse_GetToken();

		bool force_zp = false;
		bool force_abs_2 = false;

		if (lex_Context->token.id == T_OP_LESS_THAN) {
			addrMode->size_forced = true;
			force_zp = true;
			parse_GetToken();
		} else if (lex_Context->token.id == T_OP_BITWISE_OR) {
			addrMode->size_forced = true;
			force_abs_2 = true;
			parse_GetToken();
		}

		addrMode->expr = parse_Expression(2);
		if (addrMode->expr != NULL) {
			if (lex_Context->token.id == ']') {
				parse_GetToken();
				if (lex_Context->token.id == ',') {
					parse_GetToken();
					if (lex_Context->token.id == T_6502_REG_Y) {
						parse_GetToken();
						addrMode->mode = MODE_816_LONG_IND_ZP_Y;
						return true;
					} else if (lex_Context->token.id == T_4510_REG_Z) {
						parse_GetToken();
						addrMode->mode = MODE_45GS02_IND_ZP_Z_QUAD;
						return true;
					}
				} else {
					bool is_zp = !force_abs_2 && expr_IsConstant(addrMode->expr) &&
					             opt_Current->machineOptions->bp_base <= addrMode->expr->value.integer &&
					             addrMode->expr->value.integer <= opt_Current->machineOptions->bp_base + 255;
					addrMode->mode =
					    ((force_zp || is_zp) ? (MODE_45GS02_IND_ZP_QUAD | MODE_816_LONG_IND_ZP) : MODE_816_LONG_IND_ABS) &
					    allowedModes;
					return true;
				}
			}
		}
		lex_Goto(&bm);
	}

	if ((allowedModes & (MODE_IND_ABS | MODE_IND_ZP)) && (lex_Context->token.id == '(')) {
		parse_GetToken();

		addrMode->expr = parse_Expression(2);
		if (addrMode->expr != NULL) {
			if (lex_Context->token.id == ')') {
				parse_GetToken();
				addrMode->mode = MODE_IND_ABS;
				return true;
			}
		}
		lex_Goto(&bm);
	}

	if (allowedModes & (MODE_ZP | MODE_ZP_X | MODE_ZP_Y | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_ZP_ABS | MODE_BIT_ZP_ABS |
	                    MODE_BIT_ZP | MODE_816_DISP_S | MODE_816_LONG_ABS_X | MODE_4510_ABS_X | MODE_4510_ABS_Y)) {
		bool force_zp = false;
		bool force_abs_2 = false;
		bool force_abs_3 = false;

		if (lex_Context->token.id == T_OP_LESS_THAN) {
			addrMode->size_forced = true;
			force_zp = true;
			parse_GetToken();
		} else if (opt_Current->machineOptions->cpu == MOPT_CPU_65C816S && lex_Context->token.id == T_OP_BITWISE_OR) {
			addrMode->size_forced = true;
			force_abs_2 = true;
			parse_GetToken();
		} else if (opt_Current->machineOptions->cpu == MOPT_CPU_65C816S && lex_Context->token.id == T_OP_GREATER_THAN) {
			addrMode->size_forced = true;
			force_abs_3 = true;
			parse_GetToken();
		}

		addrMode->expr = parse_Expression(2);
		if (addrMode->expr != NULL && addrMode->expr->type != EXPR_PARENS) {
			if (force_zp)
				addrMode->expr = expr_CheckRange(addrMode->expr, 0x00, 0xFF);

			bool is_zp = expr_IsConstant(addrMode->expr) && opt_Current->machineOptions->bp_base <= addrMode->expr->value.integer &&
			             addrMode->expr->value.integer <= opt_Current->machineOptions->bp_base + 255;
			bool is_abs_3 = !force_zp && !force_abs_2 && expr_IsConstant(addrMode->expr) && 0 <= addrMode->expr->value.integer &&
			                addrMode->expr->value.integer < (1 << 24);

			if (lex_Context->token.id == ',') {
				parse_GetToken();
				if (lex_Context->token.id == T_6502_REG_X) {
					parse_GetToken();
					if ((is_zp || force_zp) && (allowedModes & MODE_ZP_X)) {
						addrMode->mode = MODE_ZP_X;
					} else if ((is_abs_3 || force_abs_3) && (allowedModes & MODE_816_LONG_ABS_X)) {
						addrMode->mode = MODE_816_LONG_ABS_X;
					} else {
						addrMode->mode = (MODE_ABS_X | MODE_4510_ABS_X) & allowedModes;
					}
					return true;
				} else if (lex_Context->token.id == T_6502_REG_Y) {
					parse_GetToken();
					addrMode->mode = (is_zp || force_zp) && (allowedModes & MODE_ZP_Y)
					                     ? MODE_ZP_Y
					                     : (MODE_ABS_Y | MODE_4510_ABS_Y) & allowedModes;
					return true;
				} else if (lex_Context->token.id == T_65816_REG_S) {
					parse_GetToken();
					addrMode->mode = MODE_816_DISP_S;
					return true;
				}
				addrMode->expr2 = parse_Expression(2);
				if (addrMode->expr2 != NULL) {
					if (lex_Context->token.id == ',') {
						parse_GetToken();
						addrMode->expr3 = parse_Expression(2);
						if (addrMode->expr3 != NULL) {
							addrMode->mode = MODE_BIT_ZP_ABS;
							return true;
						}
					} else {
						addrMode->mode = allowedModes & (MODE_BIT_ZP | MODE_ZP_ABS);
						return true;
					}
				}
			} else {
				if (addrMode->expr != NULL && addrMode->expr->type != EXPR_PARENS) {
					if ((allowedModes & MODE_ZP) && (is_zp || force_zp)) {
						addrMode->mode = MODE_ZP;
						return true;
					} else if ((allowedModes & MODE_816_LONG_ABS) && (is_abs_3 || force_abs_3)) {
						addrMode->mode = MODE_816_LONG_ABS;
						return true;
					} else if (allowedModes & MODE_ABS) {
						addrMode->mode = MODE_ABS;
						return true;
					}
				}
			}
		}

		lex_Goto(&bm);
	}

	if ((allowedModes == 0) || (allowedModes & MODE_NONE)) {
		addrMode->mode = MODE_NONE;
		addrMode->expr = NULL;
		return true;
	}

	if (allowedModes & MODE_A) {
		addrMode->mode = MODE_A;
		addrMode->expr = NULL;
		return true;
	}

	return false;
}

#define HANDLE_MODES                                                                                                             \
	(MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_ABS | MODE_IND_ABS_X | MODE_4510_IND_ZP_Z | MODE_45GS02_IND_ZP_Z_QUAD |       \
	 MODE_45GS02_IND_ZP_QUAD | MODE_IND_ZP_X | MODE_IND_ZP_Y | MODE_ZP_ABS | MODE_BIT_ZP_ABS | MODE_ZP | MODE_ZP_X | MODE_ZP_Y | \
	 MODE_816_LONG_IND_ZP | MODE_816_LONG_IND_ABS)
#define ZP_MODES                                                                                                              \
	(MODE_4510_IND_ZP_Z | MODE_45GS02_IND_ZP_Z_QUAD | MODE_45GS02_IND_ZP_QUAD | MODE_IND_ZP_X | MODE_IND_ZP_Y | MODE_ZP_ABS | \
	 MODE_BIT_ZP_ABS | MODE_ZP | MODE_ZP_X | MODE_ZP_Y | MODE_45GS02_IND_ZP_QUAD | MODE_816_LONG_IND_ZP)

extern bool
x65_ParseAddressingMode(SAddressingMode* addrMode, uint32_t allowedModes, EImmediateSize immSize) {
	if (x65_ParseAddressingModeCore(addrMode, allowedModes, immSize)) {
		if (addrMode->size_forced || (addrMode->mode & HANDLE_MODES) == 0 || !expr_IsConstant(addrMode->expr))
			return addrMode->mode != 0;

		SExpression** zpExpr = NULL;
		switch (addrMode->mode) {
			case MODE_BIT_ZP:
			case MODE_BIT_ZP_ABS:
				zpExpr = &addrMode->expr2;
				break;
			default:
				zpExpr = &addrMode->expr;
				break;
		}

		int32_t address = (*zpExpr)->value.integer;

		bool is_zp = address >= opt_Current->machineOptions->bp_base && address <= opt_Current->machineOptions->bp_base + 255;
		if (!is_zp) {
			if (addrMode->mode & ZP_MODES) {
				err_Error(MERROR_NEEDS_ZP);
			}
			return true;
		}

		switch (addrMode->mode) {
			case MODE_ABS:
				if ((allowedModes & MODE_ZP) == 0)
					return true;
				addrMode->mode = MODE_ZP;
				break;
			case MODE_ABS_X:
				if ((allowedModes & MODE_ZP_X) == 0)
					return true;
				addrMode->mode = MODE_ZP_X;
				break;
			case MODE_ABS_Y:
				if ((allowedModes & MODE_ZP_Y) == 0)
					return true;
				addrMode->mode = MODE_ZP_Y;
				break;
			case MODE_IND_ABS:
				if ((allowedModes & MODE_IND_ZP) == 0)
					return true;
				addrMode->mode = MODE_IND_ZP;
				break;
			case MODE_816_LONG_IND_ABS:
				if ((allowedModes & MODE_816_LONG_IND_ZP) == 0)
					return true;
				addrMode->mode = MODE_816_LONG_IND_ZP;
				break;
			case MODE_4510_IND_ZP_Z:
			case MODE_45GS02_IND_ZP_Z_QUAD:
			case MODE_45GS02_IND_ZP_QUAD:
			case MODE_IND_ZP_X:
			case MODE_IND_ZP_Y:
			case MODE_ZP_ABS:
			case MODE_BIT_ZP_ABS:
			case MODE_ZP:
			case MODE_ZP_X:
			case MODE_ZP_Y:
				break;
			default:
				return true;
		}

		*zpExpr = expr_And(*zpExpr, expr_Const(0xFF));

		return true;
	}

	return false;
}
