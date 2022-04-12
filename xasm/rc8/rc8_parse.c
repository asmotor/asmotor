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

#include "errors.h"
#include "expression.h"
#include "lexer.h"
#include "parse.h"
#include "parse_expression.h"
#include "section.h"

#include "rc8_parse.h"

SExpression*
rc8_ExpressionSU16(void) {
	SExpression* expr = parse_Expression(2);
	if (expr == NULL)
		return NULL;
		
	expr = expr_CheckRange(expr, -32768, 65535);
	if (expr == NULL)
		err_Error(ERROR_OPERAND_RANGE);

	return expr;
}


SExpression*
rc8_ParseFunction(void) {
	switch (lex_Context->token.id) {
		default:
			return NULL;
	}
}


bool
rc8_ParseInstruction(void) {
	if (rc8_ParseIntegerInstruction())
		return true;

	return false;
}
