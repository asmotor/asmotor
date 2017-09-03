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

#include "xasm.h"
#include <stdlib.h>
#include <stdio.h>

static SConfiguration s_sConfiguration =
{
	"motorschip",
	"1.0",
	0x1000,
	ASM_BIG_ENDIAN,
	false,
	false,
	MINSIZE_8BIT,
	1

	"rb", "rh", "rw",
	"db", "dh", "dw",
	"dsb", "dsh", "dsl"
};

SConfiguration* g_pConfiguration = &s_sConfiguration;

extern int main(int argc, char* argv[])
{
	return xasm_Main(argc, argv);
}
