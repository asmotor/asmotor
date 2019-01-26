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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"

#include "xasm.h"
#include "options.h"
#include "errors.h"

#include "schip_options.h"
#include "schip_tokens.h"

SMachineOptions*
locopt_Alloc(void) {
    return mem_Alloc(sizeof(SMachineOptions));
}

void
locopt_Copy(SMachineOptions* dest, SMachineOptions* src) {
    *dest = *src;
}

void
locopt_Open(SMachineOptions* options) {
    options->cpu = CPUF_SCHIP;
}

void
locopt_Update(SMachineOptions* options) {
}

bool
locopt_Parse(const char* s) {
    if (s == NULL || strlen(s) == 0)
        return false;

    switch (s[0]) {
        case 'c':
            if (strlen(&s[1]) == 1) {
                switch (s[1] - '0') {
                    case 0:
                        opt_Current->machineOptions->cpu = CPUF_CHIP8;
                        return true;
                    case 1:
                        opt_Current->machineOptions->cpu = CPUF_SCHIP;
                        return true;
                    default:
                        break;
                }
            }
            err_Warn(WARN_MACHINE_UNKNOWN_OPTION, s);
            return false;
        default:
            err_Warn(WARN_MACHINE_UNKNOWN_OPTION, s);
            return false;
    }
}

void
locopt_PrintOptions(void) {
    printf("    -mc<X>  Enable CPU:\n");
    printf("            0 = CHIP-8\n");
    printf("            1 = SuperCHIP (default)\n");
}
