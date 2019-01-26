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
#include "errors.h"
#include "options.h"

#include "m68k_options.h"

void
m68k_CopyOptions(SMachineOptions* dest, SMachineOptions* src) {
    *dest = *src;
}

SMachineOptions*
m68k_AllocOptions(void) {
    return mem_Alloc(sizeof(SMachineOptions));
}

void
m68k_SetDefaults(SMachineOptions* options) {
    options->cpu = CPUF_68000;
    options->fpu = 0;
}

void
m68k_OptionsUpdated(SMachineOptions* options) {
}

bool
m68k_ParseOption(const char* option) {
    if (option == NULL || strlen(option) == 0)
        return false;

    switch (option[0]) {
        case 'c':
            if (strlen(&option[1]) == 1) {
                switch (option[1] - '0') {
                    case 0:
                        opt_Current->machineOptions->cpu = CPUF_68000;
                        return true;
                    case 1:
                        opt_Current->machineOptions->cpu = CPUF_68010;
                        return true;
                    case 2:
                        opt_Current->machineOptions->cpu = CPUF_68020;
                        return true;
                    case 3:
                        opt_Current->machineOptions->cpu = CPUF_68030;
                        return true;
                    case 4:
                        opt_Current->machineOptions->cpu = CPUF_68040;
                        return true;
                    case 6:
                        opt_Current->machineOptions->cpu = CPUF_68060;
                        return true;
                    default:
                        break;
                }
            }
            err_Warn(WARN_MACHINE_UNKNOWN_OPTION, option);
            return false;
        case 'f':
            if (strlen(&option[1]) == 1) {
                switch (option[1]) {
                    case '1':
                    case '2':
                    case 'x':
                        opt_Current->machineOptions->cpu = FPUF_6888X;
                        return true;
                    case '4':
                        opt_Current->machineOptions->cpu = FPUF_68040;
                        return true;
                    case '6':
                        opt_Current->machineOptions->cpu = FPUF_68060;
                        return true;
                    default:
                        break;
                }
            }
            err_Warn(WARN_MACHINE_UNKNOWN_OPTION, option);
            return false;
        default:
            err_Warn(WARN_MACHINE_UNKNOWN_OPTION, option);
            return false;
    }
}

void
m68k_PrintOptions(void) {
    printf("    -mc<X>  Enable CPU 680X0\n");
}
