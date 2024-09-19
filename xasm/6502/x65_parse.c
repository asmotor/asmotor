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
#include <stddef.h>

#include "expression.h"
#include "lexer_context.h"
#include "options.h"
#include "parse.h"
#include "parse_expression.h"
#include "errors.h"

#include "x65_errors.h"
#include "x65_options.h"
#include "x65_parse.h"
#include "x65_tokens.h"


static bool
parseBitsWidth(SExpression* expr) {
	if (!expr_IsConstant(expr)) {
		err_Error(ERROR_EXPR_CONST);
		return false;
	}

	switch (expr->value.integer) {
		case 8:
			return false;
		case 16:
			return true;
		default:
			err_Error(MERROR_BIT_WIDTH);
			return false;
	}
}

static bool
parseBITS(void) {
	SExpression* m = NULL;
	SExpression* x = NULL;
	
	m = parse_Expression(2);
	if (lex_Context->token.id == ',') {
		parse_GetToken();
		x = parse_Expression(2);
	}

	if (m != NULL) {
		opt_Current->machineOptions->m16 = parseBitsWidth(m);
	}
	if (x != NULL) {
		opt_Current->machineOptions->x16 = parseBitsWidth(x);
	}

	return true;
}

SExpression*
x65_ParseExpressionSU8(void) {
    SExpression* expression = parse_Expression(1);
    if (expression == NULL)
        return NULL;

    expression = expr_CheckRange(expression, -128, 255);
    if (expression == NULL)
        err_Error(ERROR_OPERAND_RANGE);

    return expr_And(expression, expr_Const(0xFF));
}

SExpression*
x65_ParseFunction(void) {
    switch (lex_Context->token.id) {
        default:
            return NULL;
    }
}

bool
x65_ParseInstruction(void) {
    if (x65_ParseIntegerInstruction()) {
        return true;
	} else if (x65_Parse65816Instruction()) {
        return true;
	} else if (x65_Parse4510Instruction()) {
        return true;
	} else if (lex_Context->token.id == T_65816_BITS) {
		parse_GetToken();
		if (opt_Current->machineOptions->cpu & MOPT_CPU_65C816S) {
			return parseBITS();
		} else {
			err_Error(MERROR_16BIT_REQUIRED);
		}
	} else if (lex_Context->token.id == T_45GS02_SETBP) {
		parse_GetToken();
		if (opt_Current->machineOptions->cpu & (MOPT_CPU_4510 | MOPT_CPU_45GS02)) {
			opt_Current->machineOptions->bp_base = parse_ConstantExpression();
			if ((opt_Current->machineOptions->bp_base & 0xFF) != 0) {
				err_Error(MERROR_4510_BP_ALIGNMENT);
				return false;
			}
			return true;
		} else {
			err_Error(MERROR_4510_REQUIRED);
		}
	}

    return false;
}

