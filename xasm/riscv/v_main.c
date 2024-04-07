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

#include "v_errors.h"
#include "v_options.h"
#include "v_parse.h"
#include "v_section.h"
#include "v_symbols.h"
#include "v_tokens.h"

static SConfiguration
g_xasmConfiguration = {
	"motorv",
	"1.0",
	0x10000,
	ASM_LITTLE_ENDIAN,
	false,
	false,
	false,
	EM_RISCV,
	MINSIZE_8BIT,
	4,
	"CODE",

	"RB", "RW", "RL", NULL,
	"DB", "DW", "DL", NULL,
	"DSB", "DSW", "DSL", NULL,

	v_GetError,
	v_DefineTokens,
	v_DefineSymbols,

	v_AllocOptions,
	v_SetDefaultOptions,
	v_CopyOptions,
	v_ParseOption,
	v_OptionsUpdated,
	v_PrintOptions,

	v_ParseFunction,
	v_ParseInstruction,

	v_AssignSection,

	v_IsValidLocalName
};

extern int
main(int argc, char* argv[]) {
	return xasm_Main(&g_xasmConfiguration, argc, argv);
}
