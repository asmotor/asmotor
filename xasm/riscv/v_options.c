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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errors.h"
#include "mem.h"
#include "options.h"

#include "v_options.h"

struct MachineOptions* 
v_AllocOptions(void) {
	return mem_Alloc(sizeof(SMachineOptions));
}

void 
v_CopyOptions(struct MachineOptions* destination, struct MachineOptions* source) {
	*destination = *source;
}

void 
v_SetDefaultOptions(SMachineOptions* options) {
    options->architecture = 0;
}

void
v_OptionsUpdated(SMachineOptions* options) {
}

bool
v_ParseOption(const char* s) {
	if (s == NULL || strlen(s) == 0)
		return false;

	err_Warn(WARN_MACHINE_UNKNOWN_OPTION, s);
	return false;
}

void
v_PrintOptions(void) {
}
