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

#ifndef XASM_6502_OPTIONS_H_INCLUDED_
#define XASM_6502_OPTIONS_H_INCLUDED_

typedef struct MachineOptions {
    int undocumentedInstructions;
} SMachineOptions;

extern SMachineOptions*
x65_AllocOptions(void);

extern void
x65_SetDefault(SMachineOptions* options);

extern void
x65_CopyOptions(SMachineOptions* dest, SMachineOptions* pSrc);

extern bool
x65_ParseOption(const char* s);

extern void
x65_OptionsUpdated(SMachineOptions* options);

extern void
x65_PrintOptions(void);

#endif
