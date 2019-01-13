/*  Copyright 2008-2017 Carsten Elton Sorensen

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

bool parse_AddressingMode(SAddressingMode* pAddrMode, uint32_t nAllowedModes)
{
	SLexerBookmark bm;
	lex_Bookmark(&bm);

	if((nAllowedModes & MODE_A) && lex_Current.token == T_6502_REG_A)
	{
		parse_GetToken();
		pAddrMode->nMode = MODE_A;
		pAddrMode->expr = NULL;
		return true;
	}
	else if((nAllowedModes & MODE_IMM) && lex_Current.token == '#')
	{
		parse_GetToken();
		pAddrMode->nMode = MODE_IMM;
		pAddrMode->expr = parse_ExpressionSU8();
		return true;
	}

	if((nAllowedModes & (MODE_IND_X | MODE_IND_Y)) && lex_Current.token == '(')
	{
		parse_GetToken();
		pAddrMode->expr = parse_ExpressionSU8();

		if(pAddrMode->expr != NULL)
		{
			if(lex_Current.token == ',')
			{
				parse_GetToken();
				if(lex_Current.token == T_6502_REG_X)
				{
					parse_GetToken();
					if(parse_ExpectChar(')'))
					{
						pAddrMode->nMode = MODE_IND_X;
						return true;
					}
				}
			}
			else if(lex_Current.token == ')')
			{
				parse_GetToken();
				if(lex_Current.token == ',')
				{
					parse_GetToken();
					if(lex_Current.token == T_6502_REG_Y)
					{
						parse_GetToken();
						pAddrMode->nMode = MODE_IND_Y;
						return true;
					}
				}
			}
		}

		lex_Goto(&bm);
	}

	if(nAllowedModes & MODE_IND)
	{
		if(lex_Current.token == '(')
		{
			parse_GetToken();

			pAddrMode->expr = parse_Expression(2);
			if(pAddrMode->expr != NULL)
			{
				if(parse_ExpectChar(')'))
				{
					pAddrMode->nMode = MODE_IND;
					return true;
				}
			}
		}
		lex_Goto(&bm);
	}

	if(nAllowedModes & (MODE_ZP | MODE_ZP_X | MODE_ZP_Y | MODE_ABS | MODE_ABS_X | MODE_ABS_Y))
	{
		pAddrMode->expr = parse_Expression(2);

		if(pAddrMode->expr != NULL)
		{
			if(expr_IsConstant(pAddrMode->expr)
			&& 0 <= pAddrMode->expr->value.integer && pAddrMode->expr->value.integer <= 255)
			{
				if(lex_Current.token == ',')
				{
					parse_GetToken();
					if(lex_Current.token == T_6502_REG_X)
					{
						parse_GetToken();
						pAddrMode->nMode = MODE_ZP_X;
						return true;
					}
					else if(lex_Current.token == T_6502_REG_Y)
					{
						parse_GetToken();
						pAddrMode->nMode = MODE_ZP_Y;
						return true;
					}
				}
				pAddrMode->nMode = MODE_ZP;
				return true;
			}

			if(lex_Current.token == ',')
			{
				parse_GetToken();

				if(lex_Current.token == T_6502_REG_X)
				{
					parse_GetToken();
					pAddrMode->nMode = MODE_ABS_X;
					return true;
				}
				else if(lex_Current.token == T_6502_REG_Y)
				{
					parse_GetToken();
					pAddrMode->nMode = MODE_ABS_Y;
					return true;
				}
			}

			pAddrMode->nMode = MODE_ABS;
			return true;
		}

		lex_Goto(&bm);
	}

	if((nAllowedModes == 0) || (nAllowedModes & MODE_NONE))
	{
		pAddrMode->nMode = MODE_NONE;
		pAddrMode->expr = NULL;
		return true;
	}

	if(nAllowedModes & MODE_A)
	{
		pAddrMode->nMode = MODE_A;
		pAddrMode->expr = NULL;
		return true;
	}

	return false;
}
