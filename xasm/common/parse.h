/*  Copyright 2008 Carsten Sørensen

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

#ifndef	INCLUDE_PARSE_H
#define	INCLUDE_PARSE_H

#include "expr.h"

extern	bool_t	parse_Do(void);
extern	void	parse_GetToken(void);
extern	SExpression* parse_Expression(void);
extern	int32_t	parse_ConstantExpression(void);
extern bool_t	parse_ExpectChar(char ch);
extern bool_t parse_ExpectComma(void);

#endif	/*INCLUDE_PARSE_H*/