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

#include "x65_errors.h"
#include "x65_options.h"
#include "x65_tokens.h"

static SConfiguration
xasm_6502Configuration = {
	"motor6502",
	"1.0",
	0x10000,
	ASM_LITTLE_ENDIAN,
	false,
	false,
	MINSIZE_8BIT,
	1,

	"RB", "RW", "RL",
	"DB", "DW", "DL",
	"DS", NULL, NULL,

	loc_GetError,
	loclexer_Init,
	locopt_Alloc,
	locopt_Init,
	locopt_Copy,
	locopt_Parse,
};

extern int
main(int argc, char* argv[]) {
	return xasm_Main(&xasm_6502Configuration, argc, argv);
}
