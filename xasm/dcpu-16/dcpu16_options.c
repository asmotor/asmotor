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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"

#include "errors.h"
#include "options.h"
#include "xasm.h"


#include "dcpu16_options.h"

struct MachineOptions*
x10c_AllocOptions(void) {
	return mem_Alloc(sizeof(SMachineOptions));
}

void
x10c_SetDefaults(SMachineOptions* options) {}

void
x10c_CopyOptions(struct MachineOptions* dest, struct MachineOptions* src) {
	*dest = *src;
}

void
x10c_OptionsUpdated(SMachineOptions* options) {}

bool
x10c_ParseOption(const char* option) {
	if (option == NULL || strlen(option) == 0)
		return false;

	switch (option[0]) {
		case 'o':
			if (strlen(&option[1]) == 1) {
				switch (option[1] - '0') {
					case 0:
						opt_Current->machineOptions->optimize = false;
						return true;
					case 1:
						opt_Current->machineOptions->optimize = true;
						return true;
					default:
						break;
				}
			}
			break;
		default:
			break;
	}

	err_Warn(WARN_MACHINE_UNKNOWN_OPTION, option);
	return false;
}

void
x10c_PrintOptions(void) {
	printf("    -mo<X>  Optimization level, 0 or 1\n");
}
