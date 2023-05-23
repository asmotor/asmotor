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

#include "xasm.h"
#include "expression.h"
#include "lexer.h"
#include "parse.h"
#include "parse_expression.h"
#include "errors.h"
#include "section.h"

#include "x65_errors.h"
#include "x65_options.h"
#include "x65_parse.h"
#include "x65_tokens.h"

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
	} else if (lex_Context->token.id == T_65816_BITS) {
		if (opt_Current->machineOptions->cpu & MOPT_CPU_65C816S) {
			opt_Current->machineOptions->bits16 = !opt_Current->machineOptions->bits16;
			return true;
		} else {
			err_Error(MERROR_16BIT_REQUIRED);
		}
	}

    return false;
}

