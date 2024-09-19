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

#include <stdlib.h>
#include <stdio.h>

#include "xasm.h"

#include "rc8_errors.h"
#include "rc8_options.h"
#include "rc8_parse.h"
#include "rc8_section.h"
#include "rc8_symbols.h"
#include "rc8_tokens.h"

static SConfiguration
g_xasmConfiguration = {
	"motorrc8",
	0x10000,
	ASM_BIG_ENDIAN,
	false,
	false,
	false,
	false,
	MINSIZE_8BIT,
	1,
	"CODE",

	"RB", "RW", NULL, NULL,
	"DB", "DW", NULL, NULL,
	"DS", NULL, NULL, NULL,

	rc8_GetError,
	rc8_DefineTokens,
	rc8_DefineSymbols,

	rc8_AllocOptions,
	rc8_SetDefaultOptions,
	rc8_CopyOptions,
	rc8_ParseOption,
	rc8_OptionsUpdated,
	rc8_PrintOptions,

	rc8_ParseFunction,
	rc8_ParseInstruction,

	rc8_AssignSection,

	rc8_IsValidLocalName
};

extern int
main(int argc, char* argv[]) {
	return xasm_Main(&g_xasmConfiguration, argc, argv);
}
