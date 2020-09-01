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

static const char* g_errors[] = {
	"Result of operation is undefined",
	"Instruction is unsized, ignoring size",
	"Scale out of range (must be 1, 2, 4 or 8)",
	"Invalid index register size",
	"Invalid displacement size",
	"Invalid instruction size",
	"Invalid instruction for selected CPU",
	"Bitfield expected",
	"Instruction is privileged",
	"MOVEM instruction skipped due to empty register list",
	"CAS misaligned word or long access is unimplemented on 68060",
	"FPU register expected",
	"Instruction not supported by selected FPU",
	"Selected FPU 6888x needs CPU 68020 or 68030",
	"Selected FPU 68040 needs CPU 68040",
	"Selected FPU 68060 needs CPU 68060",
	"Not an allowed 68080 mode"
};

const char*
m68k_GetError(size_t errorNumber)
{
	if (errorNumber < 1000)
		return NULL;

	return g_errors[errorNumber - 1000];
}
