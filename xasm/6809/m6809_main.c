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
#include <stdio.h>
#include <stdlib.h>

#include "options.h"
#include "xasm.h"

#include "m6809_errors.h"
#include "m6809_options.h"
#include "m6809_parse.h"
#include "m6809_section.h"
#include "m6809_symbols.h"
#include "m6809_tokens.h"

static SConfiguration m6809_xasmConfiguration = {
    "motor6809",
    ASM_BIG_ENDIAN,
    true, // banks
    false,
    false,
    false,
    MINSIZE_8BIT,
    1,
    "CODE",

    // clang-format off
	"RB", "RW", NULL, NULL,
	"DB", "DW", NULL, NULL,
	"DS", NULL, NULL, NULL,
    // clang-format on

    m6809_GetError,
    m6809_DefineTokens,
    m6809_DefineSymbols,

    m6809_AllocOptions,
    m6809_SetDefault,
    m6809_CopyOptions,
    m6809_ParseOption,
    m6809_OptionsUpdated,
    m6809_PrintOptions,

    m6809_ParseFunction,
    m6809_ParseInstruction,

    m6809_AssignSection,

    m6809_IsValidLocalName,
};

extern int
main(int argc, char* argv[]) {
	return xasm_Main(&m6809_xasmConfiguration, argc, argv);
}
