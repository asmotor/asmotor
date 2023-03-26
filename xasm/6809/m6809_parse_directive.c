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

#include <assert.h>
#include <stdbool.h>

#include "errors.h"
#include "lexer.h"
#include "parse.h"
#include "parse_expression.h"

#include "m6809_parse.h"
#include "m6809_tokens.h"

typedef bool (*handler_t)(ETargetToken token);

static bool
handleSETDP(ETargetToken token) {
	SExpression* expr = parse_Expression(2);
	if (expr == NULL) {
		g_dp_base = DP_BASE_UNKNOWN;
		return true;
	}

	if (expr_IsConstant(expr)) {
		if (expr->value.integer >= 0 && expr->value.integer <= 0xFF) {
			g_dp_base = expr->value.integer << 8;
			return true;
		} else {
			err_Error(ERROR_OPERAND_RANGE);
		}
	} else {
		err_Error(ERROR_EXPR_CONST);
	}

	return false;
}


static handler_t g_directiveHandlers[T_6809_SETDP - T_6809_SETDP + 1] = {
	handleSETDP
};


bool
m6809_ParseDirective(void) {
    if (T_6809_SETDP <= lex_Context->token.id && lex_Context->token.id <= T_6809_SETDP) {
        ETargetToken token = (ETargetToken) lex_Context->token.id;
        handler_t handler = g_directiveHandlers[token - T_6809_SETDP];

        parse_GetToken();
		return handler(token);
    }

    return false;
}
