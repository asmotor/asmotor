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

#include <stdlib.h>
#include <string.h>

#include "mem.h"

#include "xasm.h"
#include "lexer_variadics.h"
#include "options.h"
#include "errors.h"

#include "z80_options.h"
#include "z80_tokens.h"

SConfiguration z80_XasmConfiguration;

uint32_t z80_gameboyLiteralId;

SMachineOptions*
z80_AllocOptions(void) {
    return mem_Alloc(sizeof(SMachineOptions));
}

void
z80_CopyOptions(SMachineOptions* dest, SMachineOptions* src) {
    *dest = *src;
}

void
z80_SetDefaultOptions(SMachineOptions* options) {
    options->gameboyLiteralCharacters[0] = '0';
    options->gameboyLiteralCharacters[1] = '1';
    options->gameboyLiteralCharacters[2] = '2';
    options->gameboyLiteralCharacters[3] = '3';
    options->cpu = CPUF_Z80;
}

void
z80_OptionsUpdated(SMachineOptions* options) {
    lex_VariadicRemoveAll(z80_gameboyLiteralId);
    lex_VariadicAddCharRange(z80_gameboyLiteralId, '`', '`', 0);

    for (int i = 0; i <= 3; ++i) {
        lex_VariadicAddCharRangeRepeating(
                z80_gameboyLiteralId,
                (uint8_t) options->gameboyLiteralCharacters[i],
                (uint8_t) options->gameboyLiteralCharacters[i],
                1);
    }
}

bool
z80_ParseOption(const char* s) {
    if (s == NULL || strlen(s) == 0)
        return false;

    switch (s[0]) {
        case 'g':
            if (strlen(&s[1]) == 4) {
                opt_Current->machineOptions->gameboyLiteralCharacters[0] = s[1];
                opt_Current->machineOptions->gameboyLiteralCharacters[1] = s[2];
                opt_Current->machineOptions->gameboyLiteralCharacters[2] = s[3];
                opt_Current->machineOptions->gameboyLiteralCharacters[3] = s[4];
                return true;
            }

            err_Warn(WARN_MACHINE_UNKNOWN_OPTION, s);
            return false;
        case 'c':
            if (strlen(&s[1]) == 1) {
                switch (s[1]) {
                    case 'g':
                        opt_Current->machineOptions->cpu = CPUF_GB;
                        z80_XasmConfiguration.maxSectionSize = 0x4000;
                        return true;
                    case 'z':
                        opt_Current->machineOptions->cpu = CPUF_Z80;
                        z80_XasmConfiguration.maxSectionSize = 0x10000;
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
z80_PrintOptions(void) {
    printf("    -mg<ASCI> Change the four characters used for Gameboy graphics\n"
           "              constants (default is 0123)\n");
    printf("    -mc<x>    Change CPU type:\n"
           "                  g - Gameboy\n"
           "                  z - Z80\n");
}
