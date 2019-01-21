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
locopt_Alloc(void);

extern void
locopt_Init(SMachineOptions* options);

extern void
locopt_Copy(SMachineOptions* dest, SMachineOptions* pSrc);

extern bool
locopt_Parse(const char* s);

extern void
locopt_Update(SMachineOptions* options);

extern void
locopt_PrintOptions(void);

#endif
