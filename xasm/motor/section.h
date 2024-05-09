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

#ifndef XASM_MOTOR_SECTION_H_INCLUDED_
#define XASM_MOTOR_SECTION_H_INCLUDED_

#include "str.h"
#include "lists.h"

struct Expression;
struct LineMapSection;
struct Patch;
struct Symbol;

struct Section {
    list_Data(struct Section);

    string* name;
    struct Symbol* group;
    uint32_t flags;

    uint32_t id;                // Assigned when writing object

    uint32_t usedSpace;         // How many bytes are used in the section
    uint32_t freeSpace;         // How many bytes are free
    uint32_t allocatedSpace;    // How big a chunk of memory pData is pointing to

    uint32_t imagePosition;     // Where the section is placed in the final image

    uint32_t cpuOrigin;         // Where the CPU sees the first CPU word of this section
    uint32_t cpuProgramCounter; // The CPU word offset into the section
    uint32_t cpuAdjust;

    uint32_t bank;
	uint32_t align;

    struct LineMapSection* lineMap;

    struct Patch* patches;

    uint8_t* data;
};

typedef struct Section SSection;

#define SECTF_LOADFIXED 0x01u
#define SECTF_BANKFIXED 0x02u
#define SECTF_ORGFIXED  0x04u
#define SECTF_ALIGNED   0x08u
#define SECTF_ROOT      0x10u

extern SSection* sect_Current;
extern SSection* sect_Sections;

extern uint32_t
sect_TotalSections(void);

extern bool
sect_SwitchTo(const string* sectname, struct Symbol* group);

extern bool
sect_SwitchTo_KIND(const string* sectname, struct Symbol* group, uint32_t flags, uint32_t origin, uint32_t bank, uint32_t align);

extern bool
sect_SwitchTo_NAMEONLY(const string* sectname);

extern bool
sect_Init(void);

extern void
sect_Exit(void);

extern void
sect_SetOriginAddress(uint32_t org);

extern void
sect_SkipBytes(uint32_t count);

extern void
sect_Align(uint32_t align);

extern uint32_t
sect_CurrentSize(void);

extern void
sect_OutputExpr8(struct Expression* expr);

extern void
sect_OutputExpr16(struct Expression* expr);

extern void
sect_OutputExpr32(struct Expression* expr);

extern void
sect_OutputExprConst32(struct Expression* expr, uint32_t data);

extern void
sect_OutputBinaryFile(string* pFile);

extern void
sect_OutputConst8(uint8_t value);

extern void
sect_OutputConst16(uint16_t value);

extern void
sect_OutputConst16At(uint16_t value, uint32_t offset);

extern void
sect_OutputConst32(uint32_t value);

extern void
sect_OutputFloat32(long double value);

extern void
sect_OutputFloat64(long double value);

extern bool
sect_Push(void);

extern bool
sect_Pop(void);

#endif /* XASM_MOTOR_SECTION_H_INCLUDED_ */
