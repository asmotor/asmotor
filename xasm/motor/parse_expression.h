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

#ifndef XASM_MOTOR_PARSE_EXPRESSION_H_INCLUDED_
#define XASM_MOTOR_PARSE_EXPRESSION_H_INCLUDED_

#include "expression.h"

extern SExpression*
parse_Expression(size_t maxStringConstLength);

extern int32_t
parse_ConstantExpression(void);

extern SExpression*
parse_ExpressionS16(void);

extern SExpression*
parse_ExpressionU16(void);

#endif // XASM_MOTOR_PARSE_EXPRESSION_H_INCLUDED_
