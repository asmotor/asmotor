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

#include "mem.h"
#include "strcoll.h"

#include "lexer_context.h"
#include "linemap.h"

#define INITIAL_ALLOCATION 32


/* Internal functions */

static SLineMapSection*
createLineMapSection(SSection* section) {
    SLineMapSection* mapSection = mem_Alloc(sizeof(SLineMapSection));
    mapSection->totalEntries = 0;
    mapSection->allocatedEntries = INITIAL_ALLOCATION;
    mapSection->entries = mem_Alloc(INITIAL_ALLOCATION * sizeof(SLineMapEntry));

    section->lineMap = mapSection;

    return mapSection;
}


static SLineMapSection*
findLineMapSection(SSection* section) {
    SLineMapSection* lineMap = section->lineMap;
    if (lineMap != NULL) {
        return lineMap;
    }

    return section->lineMap = createLineMapSection(section);
}


static SLineMapEntry*
allocLineMapEntry(SLineMapSection* file) {
    if (file->totalEntries == file->allocatedEntries) {
        file->allocatedEntries *= 2;
        file->entries = (SLineMapEntry*) mem_Realloc(file->entries, file->allocatedEntries * sizeof(SLineMapEntry));
    }

    return &file->entries[file->totalEntries++];
}


static SLineMapEntry*
mostRecentLineMapEntry(SLineMapSection* sectionMap) {
    if (sectionMap->totalEntries > 0)
        return &sectionMap->entries[sectionMap->totalEntries - 1];
    else
        return NULL;
}
    

static void
addEntry(SLineMapSection* sectionMap, SFileInfo* fileInfo, uint32_t lineNumber, uint32_t offset) {
    SLineMapEntry* mostRecentEntry = mostRecentLineMapEntry(sectionMap);
    if (mostRecentEntry == NULL || mostRecentEntry->fileInfo != fileInfo || mostRecentEntry->lineNumber != lineNumber) {
        SLineMapEntry* entry = allocLineMapEntry(sectionMap);
        entry->fileInfo = fileInfo;
        entry->lineNumber = lineNumber;
        entry->offset = offset;
    }
}


/* Exported functions */

extern void
linemap_Add(SFileInfo* fileInfo, uint32_t lineNumber, SSection* section, uint32_t offset) {
    addEntry(findLineMapSection(section), fileInfo, lineNumber, offset);
}


extern void
linemap_AddCurrent(void) {
    linemap_Add(lexctx_TokenFileInfo(), lexctx_TokenLineNumber(), sect_Current, sect_Current->cpuProgramCounter);
}
