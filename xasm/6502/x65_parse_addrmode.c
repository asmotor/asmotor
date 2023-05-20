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

#include "lexer.h"
#include "parse.h"
#include "parse_expression.h"

#include "x65_parse.h"
#include "x65_tokens.h"

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
    } else if ((allowedModes & MODE_IMM) && lex_Context->token.id == '#') {
        parse_GetToken();
        addrMode->mode = MODE_IMM;
        addrMode->expr = x65_ParseExpressionSU8();
        return true;
    }

    if ((allowedModes & (MODE_IND_X | MODE_IND_Y)) && lex_Context->token.id == '(') {
        parse_GetToken();
        addrMode->expr = x65_ParseExpressionSU8();

        if (addrMode->expr != NULL) {
            if (lex_Context->token.id == ',') {
                parse_GetToken();
                if (lex_Context->token.id == T_6502_REG_X) {
                    parse_GetToken();
                    if (parse_ExpectChar(')')) {
                        addrMode->mode = MODE_IND_X;
                        return true;
                    }
                }
            } else if (lex_Context->token.id == ')') {
                parse_GetToken();
                if (lex_Context->token.id == ',') {
                    parse_GetToken();
                    if (lex_Context->token.id == T_6502_REG_Y) {
                        parse_GetToken();
                        addrMode->mode = MODE_IND_Y;
                        return true;
                    }
                }
            }
        }

        lex_Goto(&bm);
    }

    if (allowedModes & MODE_IND) {
        if (lex_Context->token.id == '(') {
            parse_GetToken();

            addrMode->expr = parse_Expression(2);
            if (addrMode->expr != NULL) {
                if (parse_ExpectChar(')')) {
                    addrMode->mode = MODE_IND;
                    return true;
                }
            }
        }
        lex_Goto(&bm);
    }

    if (allowedModes & (MODE_ZP | MODE_ZP_X | MODE_ZP_Y | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_ZP_ABS | MODE_BIT_ZP_ABS | MODE_BIT_ZP)) {
        addrMode->expr = parse_Expression(2);
        if (addrMode->expr != NULL) {
			bool is_zp = expr_IsConstant(addrMode->expr) && 0 <= addrMode->expr->value.integer && addrMode->expr->value.integer <= 255;

			if (lex_Context->token.id == ',') {
				parse_GetToken();
				if (lex_Context->token.id == T_6502_REG_X) {
					parse_GetToken();
					addrMode->mode = is_zp ? MODE_ZP_X : MODE_ABS_X;
					return true;
				} else if (lex_Context->token.id == T_6502_REG_Y) {
					parse_GetToken();
					addrMode->mode = is_zp ? MODE_ZP_Y : MODE_ABS_Y;
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
	            addrMode->mode = (allowedModes & MODE_ZP) && is_zp ? MODE_ZP : MODE_ABS;
    	        return true;
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
