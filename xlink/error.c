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

#include <assert.h>	
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "xlink.h"
#include "error.h"

NORETURN (void error(const char* fmt, ...));

extern void
error(const char* fmt, ...) {
    va_list list;

    va_start(list, fmt);

    printf("ERROR: ");
    vprintf(fmt, list);
    printf("\n");

    va_end(list);

	remove(g_outputFilename);

    exit(EXIT_FAILURE);
}

