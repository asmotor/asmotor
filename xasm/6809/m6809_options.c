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

#include "mem.h"

#include "errors.h"
#include "options.h"

#include "m6809_options.h"

void
m6809_CopyOptions(struct MachineOptions* dest, struct MachineOptions* pSrc) {
	*dest = *pSrc;
}

struct MachineOptions*
m6809_AllocOptions(void) {
	return mem_Alloc(sizeof(SMachineOptions));
}

void
m6809_SetDefault(SMachineOptions* options) {}

void
m6809_OptionsUpdated(SMachineOptions* options) {}

bool
m6809_ParseOption(const char* s) {
	err_Warn(WARN_MACHINE_UNKNOWN_OPTION, s);
	return false;
}

void
m6809_PrintOptions(void) {}
