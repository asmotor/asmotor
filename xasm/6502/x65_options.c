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

#include "mem.h"

#include "xasm.h"
#include "lexer_constants.h"
#include "options.h"
#include "errors.h"

#include "x65_errors.h"
#include "x65_options.h"
#include "x65_tokens.h"

static int g_previousUndocumented = 0;

void
x65_CopyOptions(struct MachineOptions* dest, struct MachineOptions* pSrc) {
    *dest = *pSrc;
}

struct MachineOptions*
x65_AllocOptions(void) {
    return mem_Alloc(sizeof(SMachineOptions));
}

void
x65_SetDefault(SMachineOptions* options) {
    options->undocumentedInstructions = 0;
	options->cpu = MOPT_CPU_6502;
	options->bits16 = false;
}

void
x65_OptionsUpdated(SMachineOptions* options) {
    int newSet = options->undocumentedInstructions;
    if (g_previousUndocumented != newSet) {
        SLexConstantsWord* prev = x65_GetUndocumentedInstructions(g_previousUndocumented);
        if (prev)
            lex_ConstantsUndefineWords(prev);

        SLexConstantsWord* next = x65_GetUndocumentedInstructions(newSet);
        if (next)
            lex_ConstantsDefineWords(next);

        g_previousUndocumented = newSet;
    }
}

bool
x65_ParseOption(const char* s) {
    if (s == NULL || strlen(s) == 0)
        return false;

    switch (s[0]) {
        case 'u': {
			long n = strtol(&s[1], NULL, 10);
			if (n >= 0 && n <= 3) {
				opt_Current->machineOptions->undocumentedInstructions = n;
			} else {
				err_Error(MERROR_UNDOCUMENTED_RANGE);
				return false;
			}
            break;
		}
        case 'c': {
			long n = strtol(&s[1], NULL, 10);
			if (n >= 0 && n <= 3) {
				ECpu6502 cpu = 1 << n;
				opt_Current->machineOptions->cpu = cpu;
				opt_Current->machineOptions->bits16 = cpu == MOPT_CPU_65C816S;
			} else {
				err_Error(MERROR_CPU_RANGE);
				return false;
			}
			break;
		}
        default:
			err_Warn(WARN_MACHINE_UNKNOWN_OPTION, s);
			return false;
    }

	if (opt_Current->machineOptions->cpu != MOPT_CPU_6502 && opt_Current->machineOptions->undocumentedInstructions != 0) {
		err_Error(MERROR_UNDOCUMENTED_NOT_SUPPORTED);
		return false;
	}

	return true;
}

void
x65_PrintOptions(void) {
    printf(
		"    -mu<x>  Enable undocumented instructions, name set x (0, 1 or 2)\n"
		"    -mc<x>  Enable 6502 instruction set level\n"
		"              0 - 6502\n"
		"              1 - 65C02\n"
		"              2 - 65C02S\n"
	);
}
