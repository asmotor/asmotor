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

#include <stdbool.h>

#include "lexer_context.h"
#include "options.h"
#include "parse.h"
#include "parse_expression.h"

#include "m68k_options.h"
#include "m68k_parse.h"
#include "m68k_symbols.h"
#include "m68k_tokens.h"

bool
m68k_ParseDirective(void) {
	switch (lex_Context->token.id) {
		case T_68K_MC68000:
			opt_Current->machineOptions->cpu = CPUF_68000;
			parse_GetToken();
			return true;
		case T_68K_MC68010:
			opt_Current->machineOptions->cpu = CPUF_68010;
			parse_GetToken();
			return true;
		case T_68K_MC68020:
			opt_Current->machineOptions->cpu = CPUF_68020;
			parse_GetToken();
			return true;
		case T_68K_MC68030:
			opt_Current->machineOptions->cpu = CPUF_68030;
			parse_GetToken();
			return true;
		case T_68K_MC68040:
			opt_Current->machineOptions->cpu = CPUF_68040;
			parse_GetToken();
			return true;
		case T_68K_MC68060:
			opt_Current->machineOptions->cpu = CPUF_68060;
			parse_GetToken();
			return true;
		case T_68K_MC68080:
			opt_Current->machineOptions->cpu = CPUF_68080;
			parse_GetToken();
			return true;
		case T_68K_FPU6888X:
			opt_Current->machineOptions->fpu = FPUF_6888X;
			parse_GetToken();
			return true;
		case T_68K_FPU68040:
			opt_Current->machineOptions->fpu = FPUF_68040;
			parse_GetToken();
			return true;
		case T_68K_FPU68060:
			opt_Current->machineOptions->fpu = FPUF_68060;
			parse_GetToken();
			return true;
		case T_68K_FPU68080:
			opt_Current->machineOptions->fpu = FPUF_68080;
			parse_GetToken();
			return true;
		case T_68K_REGMASKADD:
			parse_GetToken();
			m68k_AddRegmask(parse_ConstantExpression());
			return true;
		case T_68K_REGMASKRESET:
			m68k_ResetRegmask();
			parse_GetToken();
			return true;
		default:
			return false;
	}
}
