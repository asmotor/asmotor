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
#include <string.h>

#include "mem.h"

#include "errors.h"
#include "options.h"

#include "mips_options.h"

SMachineOptions*
mips_AllocOptions(void) {
	return mem_Alloc(sizeof(SMachineOptions));
}

void
mips_CopyOptions(SMachineOptions* dest, SMachineOptions* src) {
	*dest = *src;
}

void
mips_SetDefaultOptions(SMachineOptions* options) {
	options->cpu = CPUF_MIPS32R2;
}

void
mips_OptionsUpdated(SMachineOptions* options) {}

bool
mips_ParseOption(const char* s) {
	if (s == NULL || strlen(s) == 0)
		return false;

	switch (s[0]) {
		case 'c':
			if (strlen(&s[1]) == 1) {
				switch (s[1] - '0') {
					case 0:
						opt_Current->machineOptions->cpu = CPUF_MIPS32R1;
						return true;
					case 1:
						opt_Current->machineOptions->cpu = CPUF_MIPS32R2;
						return true;
					default:
						err_Warn(WARN_MACHINE_UNKNOWN_OPTION, s);
						return false;
				}
			} else {
				err_Warn(WARN_MACHINE_UNKNOWN_OPTION, s);
				return false;
			}
		default:
			err_Warn(WARN_MACHINE_UNKNOWN_OPTION, s);
			return false;
	}
}

void
mips_PrintOptions(void) {
	printf("    -mc<X>  Enable CPU:\n");
	printf("            0 = MIPS32 R1\n");
	printf("            1 = MIPS32 R2 (default)\n");
}
