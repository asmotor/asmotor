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

#include <stdlib.h>
#include <string.h>

#include "mem.h"

#include "errors.h"
#include "options.h"
#include "xasm.h"

#include "z80_options.h"
#include "z80_tokens.h"

extern SConfiguration z80_XasmConfiguration;

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
	options->cpu = CPUF_Z80;
	options->synthesizedInstructions = false;
}

void
z80_OptionsUpdated(SMachineOptions* options) {}

bool
z80_ParseOption(const char* s) {
	if (s == NULL || strlen(s) == 0)
		return false;

	switch (s[0]) {
		case 'g':
			if (strlen(&s[1]) == 4) {
				opt_Current->gameboyLiteralCharacters[0] = s[1];
				opt_Current->gameboyLiteralCharacters[1] = s[2];
				opt_Current->gameboyLiteralCharacters[2] = s[3];
				opt_Current->gameboyLiteralCharacters[3] = s[4];
				return true;
			}

			err_Warn(WARN_MACHINE_UNKNOWN_OPTION, s);
			return false;
		case 'c':
			if (strlen(&s[1]) == 1) {
				switch (s[1]) {
					case 'g':
						opt_Current->machineOptions->cpu = CPUF_GB;
						return true;
					case 'z':
						opt_Current->machineOptions->cpu = CPUF_Z80;
						return true;
					default:
						break;
				}
			}
			err_Warn(WARN_MACHINE_UNKNOWN_OPTION, s);
			return false;
		case 's':
			if (strlen(&s[1]) == 1) {
				opt_Current->machineOptions->synthesizedInstructions = s[1] == '1';
				return true;
			}
			err_Warn(WARN_MACHINE_UNKNOWN_OPTION, s);
			return false;
		case 'u':
			if (strlen(&s[1]) == 1) {
				opt_Current->machineOptions->undocumentedInstructions = s[1] == '1';
				return true;
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
	       "              constants (default is 0123)\n"
	       "    -mc<x>    Change CPU type:\n"
	       "                  g - Gameboy\n"
	       "                  z - Z80\n"
	       "    -ms<x>    Synthesized instructions:\n"
	       "                  0 - Disabled (default)\n"
	       "                  1 - Enabled\n"
	       "    -mu<x>    Undocumented instructions:\n"
	       "                  0 - Disabled (default)\n"
	       "                  1 - Enabled\n");
}
