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

#include <stdlib.h>
#include <string.h>

#include "xasm.h"
#include "asmotor.h"
#include "parse.h"
#include "project.h"
#include "localasm.h"
#include "expr.h"
#include "lexer.h"
#include "options.h"
#include "section.h"

#include "locopt.h"
#include "loccpu.h"
#include "addrmode.h"
#include "intinstr.h"
#include "pseudoop.h"

SExpression* parse_TargetFunction(void)
{
	switch(g_CurrentToken.ID.TargetToken)
	{
		case T_68K_REGMASK:
		{
			uint32_t regs;
			parse_GetToken();
			if(!parse_ExpectChar('('))
				return NULL;
			regs = parse_RegisterList();
			if(regs == REGLIST_FAIL)
				return NULL;
			if(!parse_ExpectChar(')'))
				return NULL;
			return expr_Const(regs);
		}
		default:
			return NULL;
	}
}

bool_t parse_TargetSpecific(void)
{
	if(parse_IntegerInstruction())
		return true;
	else if(parse_PseudoOp())
		return false;

	return false;
}
