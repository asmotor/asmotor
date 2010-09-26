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

#ifndef	INCLUDE_EXPR_H
#define	INCLUDE_EXPR_H

struct Symbol;

typedef	enum
{
	EXPR_OPERATOR,
	EXPR_PCREL,
	EXPR_CONSTANT,
	EXPR_SYMBOL
} EExprType;

#define EXPRF_isCONSTANT	0x01
#define EXPRF_isRELOC		0x02

struct Expression
{
	struct Expression*	pLeft;
	struct Expression*	pRight;
	EExprType Type;
	ULONG	Flags;
	ULONG	Operator;
	union
	{
		SLONG	Value;
		struct	Symbol*	pSymbol;
	} Value;
};
typedef	struct Expression SExpression;

#endif	/*INCLUDE_EXPR_H*/
