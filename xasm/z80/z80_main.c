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

#include "z80_errors.h"
#include "z80_options.h"
#include "z80_parse.h"
#include "z80_section.h"
#include "z80_symbols.h"
#include "z80_tokens.h"

SConfiguration
z80_XasmConfiguration = {
	"motorz80", "1.0",

	0x4000,
	ASM_LITTLE_ENDIAN,
	true,
	false,
	false,
	EM_NONE,
	R_NONE,
	MINSIZE_8BIT,
	1,
	"HOME",

	"RB", "RW", NULL, NULL,
	"DB", "DW", NULL, NULL,
	"DS", NULL, NULL, NULL,

	z80_GetError,
	z80_DefineTokens,
	z80_DefineSymbols,

	z80_AllocOptions,
	z80_SetDefaultOptions,
	z80_CopyOptions,
	z80_ParseOption,
	z80_OptionsUpdated,
	z80_PrintOptions,

	z80_ParseFunction,
	z80_ParseInstruction,

	z80_AssignSection,

	z80_IsValidLocalName
};

extern int
main(int argc, char* argv[]) {
	return xasm_Main(&z80_XasmConfiguration, argc, argv);
}
