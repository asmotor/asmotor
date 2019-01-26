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

#include <stdlib.h>
#include <stdio.h>

#include "xasm.h"

#include "0x10c_errors.h"
#include "0x10c_options.h"
#include "0x10c_parse.h"
#include "0x10c_symbols.h"
#include "0x10c_tokens.h"

static SConfiguration x10c_XasmConfiguration = {
	"motor0x10c",
	"1.0",
	0x10000,
	ASM_BIG_ENDIAN,
	false,
	false,
	MINSIZE_16BIT,
	2,
	
	NULL, "RW", "RL",
	NULL, "DW", "DL",
	NULL, "DSW", NULL,

	x10c_GetError,
	x10c_DefineTokens,
	x10c_DefineSymbols,

	x10c_AllocOptions,
	x10c_SetDefaults,
	x10c_CopyOptions,
	x10c_ParseOption,
	x10c_OptionsUpdated,
	x10c_PrintOptions,

	x10c_ParseFunction,
	x10c_ParseInstruction
};

extern int
main(int argc, char* argv[]) {
	return xasm_Main(&x10c_XasmConfiguration, argc, argv);
}
