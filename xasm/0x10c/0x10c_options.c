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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"

#include "xasm.h"
#include "options.h"
#include "project.h"

#include "0x10c_options.h"

void
locopt_Copy(struct MachineOptions* dest, struct MachineOptions* src) {
    *dest = *src;
}

struct MachineOptions*
locopt_Alloc(void) {
    return mem_Alloc(sizeof(SMachineOptions));
}

void
locopt_Open(SOptions* options) {
    assert(options != NULL);
}

void
locopt_Update(void) {
}

bool
locopt_Parse(const char* s) {
    if (s == NULL || strlen(s) == 0)
        return false;

    switch (s[0]) {
        case 'o':
            if (strlen(&s[1]) == 1) {
                switch (s[1] - '0') {
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

    prj_Warn(WARN_MACHINE_UNKNOWN_OPTION, s);
    return false;
}

void
locopt_PrintOptions(void) {
    printf("    -mo<X>  Optimization level, 0 or 1\n");
}
