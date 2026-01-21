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

#ifndef XASM_MIPS_OPTIONS_H_INCLUDED_
#define XASM_MIPS_OPTIONS_H_INCLUDED_

#define CPUF_MIPS32R1 0x01
#define CPUF_MIPS32R2 0x02
#define CPUF_ALL      0x03

#include <stdbool.h>
#include <stdint.h>

typedef struct MachineOptions {
	uint8_t cpu;
} SMachineOptions;

extern SMachineOptions*
mips_AllocOptions(void);

extern void
mips_SetDefaultOptions(SMachineOptions* options);

extern void
mips_CopyOptions(SMachineOptions* dest, SMachineOptions* src);

extern bool
mips_ParseOption(const char* s);

extern void
mips_OptionsUpdated(SMachineOptions* options);

extern void
mips_PrintOptions(void);

#endif
