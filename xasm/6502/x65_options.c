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
#include "lexer_constants.h"
#include "options.h"
#include "project.h"

#include "x65_errors.h"
#include "x65_options.h"
#include "x65_tokens.h"

static int g_previousInstructionSet = 0;

void
locopt_Copy(struct MachineOptions* dest, struct MachineOptions* pSrc) {
    *dest = *pSrc;
}

struct MachineOptions*
locopt_Alloc(void) {
    return mem_Alloc(sizeof(SMachineOptions));
}

void
locopt_Init(SMachineOptions* options) {
    options->undocumentedInstructions = 0;
}

void
locopt_Update(SMachineOptions* options) {
    int newSet = options->undocumentedInstructions;
    if (g_previousInstructionSet != newSet) {
        SLexConstantsWord* prev = loclexer_GetUndocumentedInstructions(g_previousInstructionSet);
        if (prev)
            lex_ConstantsUndefineWords(prev);

        SLexConstantsWord* next = loclexer_GetUndocumentedInstructions(newSet);
        if (next)
            lex_ConstantsDefineWords(next);

        g_previousInstructionSet = newSet;
    }
}

bool
locopt_Parse(const char* s) {
    if (s == NULL || strlen(s) == 0)
        return false;

    switch (s[0]) {
        case 'u':
            if (strlen(&s[0]) >= 2) {
                int n = atoi(&s[1]);
                if (n >= 0 && n <= 3) {
                    opt_Current->machineOptions->undocumentedInstructions = n;
                    return true;
                }
                prj_Error(ERROR_MACHINE_OPTION_UNDOCUMENTED_RANGE);
                return false;
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
    printf("    -mu<x>  Enable undocumented opcodes, name set x (0, 1 or 2)\n");
}
