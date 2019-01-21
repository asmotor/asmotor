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

#include "m68k_errors.h"
#include "m68k_options.h"
#include "m68k_tokens.h"

static SConfiguration
g_680x0Configuration = {
	"motor68k",
	"1.0",
	0x7FFFFFFF,
	ASM_BIG_ENDIAN,
	false,
	true,
	MINSIZE_8BIT,
	8,

	"RS.B", "RS.W", "RS.L",
	"DC.B", "DC.W", "DC.L",
	"DS.B", "DS.W", "DS.L",

	loc_GetError,
	loclexer_Init,
	locopt_Alloc,
	locopt_Open,
};

extern int
main(int argc, char* argv[]) {
	return xasm_Main(&g_680x0Configuration, argc, argv);
}
