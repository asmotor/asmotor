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

#include "filestack.h"
#include "linemap.h"

#define INITIAL_ALLOCATION 32

static SLineMapFile* g_lineMapFiles = NULL;


static SLineMapFile*
createLineMapFile(string* filename) {
    SLineMapFile* file = mem_Alloc(sizeof(SLineMapFile));
    file->next = g_lineMapFiles;
    file->filename = str_Copy(filename);
    file->totalEntries = 0;
    file->allocatedEntries = INITIAL_ALLOCATION;
    file->entries = mem_Alloc(INITIAL_ALLOCATION * sizeof(SLineMapEntry));

    g_lineMapFiles = file;
    return g_lineMapFiles;
}


static SLineMapFile*
findLineMapFile(string* filename) {
    static SLineMapFile* cached = NULL;

    if (cached == NULL || !str_Equal(filename, cached->filename)) {
        for (cached = g_lineMapFiles; cached != NULL; cached = cached->next) {
            if (str_Equal(filename, cached->filename))
                break;
        }

        cached = createLineMapFile(filename);
    }

    return cached;
}


static SLineMapEntry*
allocLineMapEntry(SLineMapFile* file) {
    if (file->totalEntries == file->allocatedEntries) {
        file->allocatedEntries *= 2;
        file->entries = (SLineMapEntry*) mem_Realloc(file->entries, file->allocatedEntries * sizeof(SLineMapEntry));
    }

    return &file->entries[file->totalEntries++];
}


static SLineMapEntry*
mostRecentLineMapEntry(SLineMapFile* file) {
    if (file->totalEntries > 0)
        return &file->entries[file->totalEntries - 1];
    else
        return NULL;
}
    

static void
addEntry(SLineMapFile* file, uint32_t lineNumber, SSection* section, uint32_t offset) {
    SLineMapEntry* mostRecentEntry = mostRecentLineMapEntry(file);
    if (mostRecentEntry == NULL || mostRecentEntry->section != section || mostRecentEntry->lineNumber != lineNumber) {
        SLineMapEntry* entry = allocLineMapEntry(file);
        entry->lineNumber = lineNumber;
        entry->section = section;
        entry->offset = offset;
    }
}


extern void
linemap_Add(string* filename, uint32_t lineNumber, SSection* section, uint32_t offset) {
    addEntry(findLineMapFile(filename), lineNumber, section, offset);
}


extern void
linemap_AddCurrent(void) {
    linemap_Add(fstk_CurrentFilename(), fstk_CurrentLineNumber(), sect_Current, sect_Current->cpuProgramCounter);
}


extern uint32_t
linemap_TotalFiles(void) {
    uint32_t result = 0;
    for (SLineMapFile *file = g_lineMapFiles; file != NULL; file = file->next)
        result += 1;
    return result;
}


extern void
linemap_ForEachFile(void (*callback)(const SLineMapFile*, intptr_t data), intptr_t data) {
    for (SLineMapFile *file = g_lineMapFiles; file != NULL; file = file->next)
        callback(file, data);
}
