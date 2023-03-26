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

#include "m6809_parse.h"

uint32_t g_dp_base = DP_BASE_UNKNOWN;

SExpression*
m6809_ParseFunction(void) {
    switch (lex_Context->token.id) {
        default:
            return NULL;
    }
}

bool
m6809_ParseInstruction(void) {
    if (m6809_ParseIntegerInstruction())
        return true;
	else if (m6809_ParseDirective())
		return true;

    return false;
}
