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

#include "xasm.h"

static char* g_errors[] = {
	"Illegal addressing mode",
	"Undocumented instruction set must be 0, 1, 2 or 3",
	"CPU type must be 0, 1, 2 or 3",
	"Selected CPU does not support undocumented instructions",
	"Selected CPU does not support instruction",
	"65816 required"
};

const char*
x65_GetError(size_t errorNumber) {
	if (errorNumber < 1000)
		return NULL;

	return g_errors[errorNumber - 1000];
}
