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

#include <stdbool.h>

#include "expression.h"
#include "lexer.h"
#include "parse.h"
#include "parse_expression.h"

#include "x65_options.h"
#include "x65_parse.h"
#include "x65_tokens.h"

static SExpression* 
parseImmExpression() {
	if (lex_Context->token.id == T_OP_LESS_THAN) {
		parse_GetToken();
		return expr_And(parse_Expression(2), expr_Const(0xFF));
	} else if (lex_Context->token.id == T_OP_GREATER_THAN) {
		parse_GetToken();
		return expr_Asr(parse_Expression(2), expr_Const(8));
	}

	return parse_Expression(2);
}

bool
x65_ParseAddressingMode(SAddressingMode* addrMode, uint32_t allowedModes) {
    SLexerContext bm;
    lex_Bookmark(&bm);

    addrMode->expr = NULL;
    addrMode->expr2 = NULL;
    addrMode->expr3 = NULL;

	if ((allowedModes & MODE_A) && lex_Context->token.id == T_6502_REG_A) {
		parse_GetToken();
		addrMode->mode = MODE_A;
		addrMode->expr = NULL;
		return true;
	}
	
	if ((allowedModes & (MODE_IMM | MODE_IMM_IMM)) && lex_Context->token.id == '#') {
		parse_GetToken();
		addrMode->expr = parseImmExpression();
		if (addrMode->expr != NULL) {
			addrMode->mode = MODE_IMM;
			if ((allowedModes & MODE_IMM_IMM) && lex_Context->token.id == ',') {
				parse_GetToken();
				if (lex_Context->token.id == '#') {
					parse_GetToken();
					addrMode->expr2 = parseImmExpression();
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

    if ((allowedModes & (MODE_IND_ZP_X | MODE_IND_ZP_Y | MODE_IND_ABS_X | MODE_816_IND_DISP_S_Y)) && lex_Context->token.id == '(') {
        parse_GetToken();
        addrMode->expr = parse_Expression(2);

        if (addrMode->expr != NULL) {
            if (lex_Context->token.id == ',') {
                parse_GetToken();
                if (lex_Context->token.id == T_6502_REG_X) {
                    parse_GetToken();
                    if (parse_ExpectChar(')')) {
                        addrMode->mode = allowedModes & (MODE_IND_ZP_X | MODE_IND_ABS_X);	/* only one of these can be allowed at the same time */
                        return true;
                    }
                } if (lex_Context->token.id == T_65816_REG_S) {
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
                    }
                }
            }
        }

        lex_Goto(&bm);
    }

    if ((allowedModes & (MODE_816_LONG_IND_ZP | MODE_816_LONG_IND_ZP_Y)) && (lex_Context->token.id == '[')) {
		parse_GetToken();

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
					}
				} else {
					addrMode->mode = MODE_816_LONG_IND_ZP;
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
				addrMode->mode = allowedModes & (MODE_IND_ZP | MODE_IND_ABS);
				return true;
			}
		}
        lex_Goto(&bm);
    }

    if (allowedModes & (MODE_ZP | MODE_ZP_X | MODE_ZP_Y | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_ZP_ABS | MODE_BIT_ZP_ABS | MODE_BIT_ZP | MODE_816_DISP_S)) {
		bool force_zp = false;
		bool force_abs_2 = false;
		bool force_abs_3 = false;

		if (lex_Context->token.id == T_OP_LESS_THAN) {
			force_zp = true;
			parse_GetToken();
		} else if (opt_Current->machineOptions->cpu == MOPT_CPU_65C816S && lex_Context->token.id == T_OP_BITWISE_OR) {
			force_abs_2 = true;
			parse_GetToken();
		} else if (opt_Current->machineOptions->cpu == MOPT_CPU_65C816S && lex_Context->token.id == T_OP_GREATER_THAN) {
			force_abs_3 = true;
			parse_GetToken();
		}

        addrMode->expr = parse_Expression(2);
        if (addrMode->expr != NULL && addrMode->expr->type != EXPR_PARENS) {
			bool is_zp = false;

			if (force_zp)
				addrMode->expr = expr_CheckRange(addrMode->expr, 0x00, 0xFF);
		
			is_zp = expr_IsConstant(addrMode->expr) && 0 <= addrMode->expr->value.integer && addrMode->expr->value.integer <= 255;

			if (lex_Context->token.id == ',') {
				parse_GetToken();
				if (lex_Context->token.id == T_6502_REG_X) {
					parse_GetToken();
					addrMode->mode = (is_zp || force_zp) && (allowedModes & MODE_ZP_X) ? MODE_ZP_X : MODE_ABS_X;
					return true;
				} else if (lex_Context->token.id == T_6502_REG_Y) {
					parse_GetToken();
					addrMode->mode = (is_zp || force_zp) && (allowedModes & MODE_ZP_Y) ? MODE_ZP_Y : MODE_ABS_Y;
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
					bool is_abs_3 = !force_zp && !force_abs_2 && expr_IsConstant(addrMode->expr) && 0 <= addrMode->expr->value.integer && addrMode->expr->value.integer < (1 << 24);

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
