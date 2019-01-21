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

#include "mips_errors.h"
#include "mips_options.h"
#include "mips_tokens.h"

static SConfiguration xasm_MIPSConfiguration = {
	"motormips",
	"1.0",
	0x7FFFFFFF,
	ASM_LITTLE_ENDIAN,
	false,
	false,
	MINSIZE_8BIT,
	8,

	"RB", "RH", "RW",
	"DB", "DH", "DW",
	"DSB", "DSH", "DSL",

	loc_GetError,
	loclexer_Init,
	locopt_Alloc,
	locopt_Open,
};

extern int
main(int argc, char* argv[]) {
	return xasm_Main(&xasm_MIPSConfiguration, argc, argv);
}
