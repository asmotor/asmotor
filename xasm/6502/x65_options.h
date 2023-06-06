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

#ifndef XASM_6502_OPTIONS_H_INCLUDED_
#define XASM_6502_OPTIONS_H_INCLUDED_

typedef enum {
	/* If this enum is changed, parsing must be changed in x65_ParseOption */
	MOPT_CPU_6502	= 0x01,
	MOPT_CPU_65C02	= 0x02,
	MOPT_CPU_65C02S	= 0x04,	/* + bit instructions */
	MOPT_CPU_65C816S = 0x08
} ECpu6502;

typedef struct MachineOptions {
    int undocumentedInstructions;
	ECpu6502 cpu;
	bool m16;	/* 16 bit accumulator immediate */
	bool x16;	/* 16 bit index immediate */
	uint32_t allowedModes;
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
