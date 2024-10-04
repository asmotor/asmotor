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

#ifndef XASM_MOTOR_LINEMAP_H_INCLUDED_
#define XASM_MOTOR_LINEMAP_H_INCLUDED_

#include "lexer_context.h"

#include "section.h"


struct LineMapEntry {
    SFileInfo* fileInfo;
    uint32_t lineNumber;
    uint32_t offset;
};
typedef struct LineMapEntry SLineMapEntry;


struct LineMapSection {
    uint32_t totalEntries;
    uint32_t allocatedEntries;
    SLineMapEntry* entries;
};
typedef struct LineMapSection SLineMapSection;


extern void
linemap_Add(SFileInfo* filename, uint32_t lineNumber, SSection* section, uint32_t offset);

extern void
linemap_AddCurrent(void);

extern void
linemap_Free(SLineMapSection* linemap);

#endif
