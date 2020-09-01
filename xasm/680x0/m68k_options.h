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

#ifndef XASM_M68K_OPTIONS_H_INCLUDED_
#define XASM_M68K_OPTIONS_H_INCLUDED_

#define CPUF_68000 0x01
#define CPUF_68010 0x02
#define CPUF_68020 0x04
#define CPUF_68030 0x08
#define CPUF_68040 0x10
#define CPUF_68060 0x20
#define CPUF_68080 0x40

#define CPUF_ALL 0x7F

#define FPUF_6888X   0x01
#define FPUF_68040   0x02
#define FPUF_68040SP 0x04
#define FPUF_68060   0x08
#define FPUF_68060SP 0x10
#define FPUF_68080   0x20
#define FPUF_68080SP 0x40

#define FPUF_ALL 0x07

typedef struct MachineOptions {
    uint8_t cpu;
    uint8_t fpu;
} SMachineOptions;

extern SMachineOptions*
m68k_AllocOptions(void);

extern void
m68k_SetDefaults(SMachineOptions*);

extern void
m68k_CopyOptions(SMachineOptions* dest, SMachineOptions* src);

extern bool
m68k_ParseOption(const char* option);

extern void
m68k_OptionsUpdated(SMachineOptions* options);

extern void
m68k_PrintOptions(void);

#endif
