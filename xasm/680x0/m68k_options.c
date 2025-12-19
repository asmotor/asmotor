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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"

#include "errors.h"
#include "options.h"

#include "m68k_errors.h"
#include "m68k_options.h"
#include "m68k_symbols.h"

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
	options->platform = PLATFORM_GENERIC;
	options->trackMovem = false;
}

void
m68k_OptionsUpdated(SMachineOptions* options) {
    if ((options->fpu & FPUF_6888X) != 0 && (options->cpu & (CPUF_68020|CPUF_68030)) == 0) {
        err_Error(MERROR_FPU_NEEDS_020_030);
    }
    if ((options->fpu & FPUF_68040) != 0 && (options->cpu & CPUF_68040) == 0) {
        err_Error(MERROR_FPU_NEEDS_040);
    }
    if ((options->fpu & FPUF_68060) != 0 && (options->cpu & CPUF_68060) == 0) {
        err_Error(MERROR_FPU_NEEDS_060);
    }

	m68k_DefineMachineGroups(options->platform);
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
                    case 8:
                        opt_Current->machineOptions->cpu = CPUF_68080;
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
                        opt_Current->machineOptions->fpu = FPUF_6888X;
                        return true;
                    case '4':
                        opt_Current->machineOptions->fpu = FPUF_68040;
                        return true;
                    case '6':
                        opt_Current->machineOptions->fpu = FPUF_68060;
                        return true;
                    case '8':
                        opt_Current->machineOptions->fpu = FPUF_68080;
                        return true;
                    default:
                        break;
                }
            }
            err_Warn(WARN_MACHINE_UNKNOWN_OPTION, option);
            return false;
        case 'g':
            if (strlen(&option[1]) == 1) {
                switch (option[1]) {
                    case 'a':
                        opt_Current->machineOptions->platform = PLATFORM_AMIGA;
                        return true;
                    case 'f':
                        opt_Current->machineOptions->platform = PLATFORM_A2560K;
                        return true;
                    case 'g':
                        opt_Current->machineOptions->platform = PLATFORM_GENERIC;
                        return true;
                    case 's':
                        opt_Current->machineOptions->platform = PLATFORM_GENESIS;
                        return true;
                    default:
                        break;
                }
            }
            err_Warn(WARN_MACHINE_UNKNOWN_OPTION, option);
            return false;
        case 'm':
            if (strlen(&option[1]) == 1) {
                switch (option[1]) {
                    case 'y':
                    case 'Y':
                        opt_Current->machineOptions->trackMovem = true;
                        return true;
                    case 'n':
                    case 'N':
                        opt_Current->machineOptions->trackMovem = false;
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
    printf(
		"    -mc<X>  Enable CPU 680x0\n"
		"    -mf<X>  Enable FPU 6888x (1, 2), 68040 (4), 68060 (6), 68080 (8)\n"
		"    -mg<X>  Enable platform specific groups\n"
		"                a - Amiga\n"
		"                f - Foenix A2560K/X\n"
		"                g - Generic (default)\n"
		"                s - Sega Genesis/Mega Drive\n"
		"    -mm<X>  MOVEM updates regmask, <X> is y(es) or n(o) (default)\n"
	);
}
