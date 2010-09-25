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

#ifndef ASMOTOR_R68KASM_PSEUDOOP_H_INCLUDED_
#define ASMOTOR_R68KASM_PSEUDOOP_H_INCLUDED_

static BOOL parse_PseudoOp()
{
	switch(g_CurrentToken.ID.TargetToken)
	{
		case T_68K_MC68000:
			pOptions->Machine.nCpu = CPUF_68000;
			parse_GetToken();
			return TRUE;
		case T_68K_MC68010:
			pOptions->Machine.nCpu = CPUF_68010;
			parse_GetToken();
			return TRUE;
		case T_68K_MC68020:
			pOptions->Machine.nCpu = CPUF_68020;
			parse_GetToken();
			return TRUE;
		case T_68K_MC68030:
			pOptions->Machine.nCpu = CPUF_68030;
			parse_GetToken();
			return TRUE;
		case T_68K_MC68040:
			pOptions->Machine.nCpu = CPUF_68040;
			parse_GetToken();
			return TRUE;
		case T_68K_MC68060:
			pOptions->Machine.nCpu = CPUF_68060;
			parse_GetToken();
			return TRUE;
	}

	return FALSE;
}

#endif
