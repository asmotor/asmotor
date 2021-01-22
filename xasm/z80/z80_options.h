/*  Copyright 2008-2021 Carsten Elton Sorensen and contributors

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

#ifndef XASM_Z80_OPTIONS_H_INCLUDED_
#define XASM_Z80_OPTIONS_H_INCLUDED_

#define CPUF_GB  0x01u
#define CPUF_Z80 0x02u

typedef struct MachineOptions {
    uint8_t cpu;
    bool synthesizedInstructions;
    bool undocumentedInstructions;
} SMachineOptions;

extern uint32_t z80_gameboyLiteralId;

extern SMachineOptions*
z80_AllocOptions(void);

extern void
z80_SetDefaultOptions(SMachineOptions* options);

extern void
z80_CopyOptions(SMachineOptions* dest, SMachineOptions* src);

extern bool
z80_ParseOption(const char* s);

extern void
z80_OptionsUpdated(SMachineOptions* options);

extern void
z80_PrintOptions(void);

#endif
