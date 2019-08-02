/*  Copyright 2008-2019 Carsten Elton Sorensen

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

#ifndef XASM_COMMON_LINEMAP_H_INCLUDED_
#define XASM_COMMON_LINEMAP_H_INCLUDED_

#include "str.h"

#include "section.h"


struct LineMapEntry {
    uint32_t lineNumber;
    SSection* section;
    uint32_t offset;
};
typedef struct LineMapEntry SLineMapEntry;


struct LineMapFile {
    struct LineMapFile* next;

    string* filename;
    uint32_t totalEntries;
    uint32_t allocatedEntries;
    SLineMapEntry* entries;
};
typedef struct LineMapFile SLineMapFile;


extern void
linemap_Add(string* filename, uint32_t lineNumber, SSection* section, uint32_t offset);

extern void
linemap_AddCurrent(void);

extern uint32_t
linemap_TotalFiles(void);

extern void
linemap_ForEachFile(void (*callback)(const SLineMapFile*, intptr_t), intptr_t data);

#endif
