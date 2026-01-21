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

#include <stdio.h>
#include <stdlib.h>

#include "xasm.h"

#include "x65_errors.h"
#include "x65_options.h"
#include "x65_parse.h"
#include "x65_section.h"
#include "x65_symbols.h"
#include "x65_tokens.h"

static SConfiguration x65_xasmConfiguration = {
    "motor6502",
    ASM_LITTLE_ENDIAN,
    true,
    false,
    false,
    false,
    MINSIZE_8BIT,
    1,
    "CODE",

    // clang-format off
	"RB", "RW", "RL", NULL,
	"DB", "DW", "DL", NULL,
	"DS", NULL, NULL, NULL,
    // clang-format on

    x65_GetError,
    x65_DefineTokens,
    x65_DefineSymbols,

    x65_AllocOptions,
    x65_SetDefault,
    x65_CopyOptions,
    x65_ParseOption,
    x65_OptionsUpdated,
    x65_PrintOptions,

    x65_ParseFunction,
    x65_ParseInstruction,

    x65_AssignSection,

    x65_IsValidLocalName,
};

extern int
main(int argc, char* argv[]) {
	return xasm_Main(&x65_xasmConfiguration, argc, argv);
}
