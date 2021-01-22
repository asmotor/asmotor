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

/*
 * xLink - OBJECT.C
 * Copyright 1996-1998 Carsten Sorensen (csorensen@ea.com)
 *
 *	char	ID[4]="XOB\3";
 *	[>=v1] char	MinimumWordSize ; Used for address calculations.
 *							; 1 - A CPU address points to a byte in memory
 *							; 2 - A CPU address points to a 16 bit word in memory (CPU address 0x1000 is the 0x2000th byte)
 *							; 4 - A CPU address points to a 32 bit word in memory (CPU address 0x1000 is the 0x4000th byte)
 *	IF Version >= 2
 *		uint32_t NumberOfFiles
 *		REPT NumberOfFiles
 *			ASCIIZ		Name
 *			uint32_t	CRC32
 *		ENDR
 *	ENDC
 *	uint32_t	NumberOfGroups
 *	REPT	NumberOfGroups
 *			ASCIIZ	Name
 *			uint32_t	Type
 *	ENDR
 *	uint32_t	NumberOfSections
 *	REPT	NumberOfSections
 *			int32_t	GroupID	; -1 = exported EQU symbols
 *			ASCIIZ	Name
 *			int32_t	Bank	; -1 = not bankfixed
 *			int32_t	Position; -1 = not fixed
 *			[>=v1] int32_t BasePC	; -1 = not fixed
 *			[>=v3] int32_t ByteAlign ; -1 = not aligned
 *			[>=v4] uint8_t Rooted ; != 0 is rooted
 *			uint32_t	NumberOfSymbols
 *			REPT	NumberOfSymbols
 *					ASCIIZ	Name
 *					uint32_t	Type	;0=EXPORT
 *									;1=IMPORT
 *									;2=LOCAL
 *									;3=LOCALEXPORT
 *									;4=LOCALIMPORT
 *					IF Type==EXPORT or LOCAL or LOCALEXPORT
 *						int32_t	value
 *					ENDC
 *			ENDR
 *          IF Version >= 2
 *				uint32_t	NumberOfLineMappings
 *				REPT NumberOfLineMappings
 *					uint32_t	FileId
 *					uint32_t	LineNumber
 *					uint32_t	Offset
 *				ENDR
 *			ENDC
 *			uint32_t	Size
 *			IF	SectionCanContainData
 *					uint8_t	Data[Size]
 *					uint32_t	NumberOfPatches
 *					REPT	NumberOfPatches
 *							uint32_t	Offset
 *							uint32_t	Type
 *							uint32_t	ExprSize
 *							uint8_t	Expr[ExprSize]
 *					ENDR
 *			ENDC
 *	ENDR
 */

#include <string.h>

#include "file.h"
#include "mem.h"
#include "str.h"

#include "group.h"
#include "object.h"
#include "patch.h"
#include "section.h"
#include "symbol.h"
#include "xlink.h"

#define MAKE_ID(a, b, c, d) ((uint32_t)(a)|((uint32_t)(b)<<8u)|((uint32_t)(c)<<16u)|((uint32_t)(d)<<24u))

static uint32_t g_fileId = 0;
static uint32_t g_minimumWordSize = 0;

static uint32_t g_fileInfoCount = 0;
static SFileInfo* g_fileInfo = NULL;

static void
readGroup(FILE* fileHandle, Group* group) {
    uint32_t flags;
    uint32_t type;

    fgetsz(group->name, MAX_SYMBOL_NAME_LENGTH, fileHandle);

    type = fgetll(fileHandle);
    flags = type & (GROUP_FLAG_DATA | GROUP_FLAG_SHARED);
    type &= ~flags;

    group->flags = flags;
    group->type = (GroupType) type;

}

static Groups*
allocateGroups(uint32_t totalGroups) {
    Groups* groups = mem_Alloc(sizeof(Groups) + totalGroups * sizeof(Group));

    if (groups != NULL) {
        groups->totalGroups = totalGroups;
    }

    return groups;
}

static Groups*
readGroups(FILE* fileHandle) {
    Groups* groups;
    uint32_t totalGroups;

    totalGroups = fgetll(fileHandle);

    if ((groups = allocateGroups(totalGroups)) != NULL) {
        Group* group = groups->groups;

        for (uint32_t i = 0; i < totalGroups; i += 1)
            readGroup(fileHandle, group++);
    } else {
        error("Out of memory");
    }

    return groups;
}

static void
readSymbol(FILE* fileHandle, SSymbol* symbol) {
    fgetsz(symbol->name, MAX_SYMBOL_NAME_LENGTH, fileHandle);

    symbol->type = (ESymbolType) fgetll(fileHandle);

    if (symbol->type != SYM_IMPORT && symbol->type != SYM_LOCALIMPORT)
        symbol->value = fgetll(fileHandle);
    else
        symbol->value = 0;

    symbol->resolved = false;
    symbol->section = NULL;
}

static uint32_t
readSymbols(FILE* fileHandle, SSymbol** outputSymbols) {
    uint32_t totalSymbols = fgetll(fileHandle);

    if (totalSymbols == 0) {
        *outputSymbols = NULL;
        return 0;
    } else {
        SSymbol* symbol = mem_Alloc(totalSymbols * sizeof(SSymbol));
        if (symbol != NULL) {
            *outputSymbols = symbol;

            for (uint32_t i = 0; i < totalSymbols; i += 1)
                readSymbol(fileHandle, symbol++);

            return totalSymbols;
        }
    }

    error("Out of memory");
}

static void
readPatch(FILE* fileHandle, SPatch* patch) {
    patch->offset = fgetll(fileHandle);
    patch->valueSymbol = NULL;
    patch->valueSection = NULL;
    patch->type = (EPatchType) fgetll(fileHandle);
    patch->expressionSize = fgetll(fileHandle);

    if ((patch->expression = mem_Alloc(patch->expressionSize)) != NULL) {
        if (patch->expressionSize != fread(patch->expression, 1, patch->expressionSize, fileHandle))
            error("File read failed");
    } else {
        error("Out of memory");
    }
}

static SPatches*
readPatches(FILE* fileHandle) {
    SPatches* patches;
    int totalPatches = fgetll(fileHandle);

    if ((patches = patch_Alloc(totalPatches)) != NULL) {
        SPatch* patch = patches->patches;
        int i;

        for (i = 0; i < totalPatches; i += 1)
            readPatch(fileHandle, patch++);

        return patches;
    }

    error("Out of memory");
}

static void
readLineMapping(FILE* fileHandle, SLineMapping* lineMapping, uint32_t fileInfoIndex) {
    lineMapping->fileInfo = &g_fileInfo[fileInfoIndex + fgetll(fileHandle)];
    lineMapping->lineNumber = fgetll(fileHandle);
    lineMapping->offset = fgetll(fileHandle);
}

static uint32_t
readLineMappings(FILE* fileHandle, SLineMapping** lineMappings, uint32_t fileInfoIndex) {
    uint32_t total = fgetll(fileHandle);
    if (total > 0) {
        *lineMappings = (SLineMapping*) mem_Alloc(sizeof(SLineMapping) * total);
        for (uint32_t i = 0; i < total; ++i) {
            readLineMapping(fileHandle, &(*lineMappings)[i], fileInfoIndex);
        }
    } else {
        *lineMappings = NULL;
    }
    return total;
}

static void
readSection(FILE* fileHandle, SSection* section, Groups* groups, int version, uint32_t fileInfoIndex) {
    section->group = groups_GetGroup(groups, fgetll(fileHandle));
    fgetsz(section->name, MAX_SYMBOL_NAME_LENGTH, fileHandle);
    section->cpuBank = fgetll(fileHandle);
    section->cpuByteLocation = fgetll(fileHandle);
    if (version >= 1)
        section->cpuLocation = fgetll(fileHandle);
    else
        section->cpuLocation = section->cpuByteLocation;

    if (version >= 3)
        section->byteAlign = fgetll(fileHandle);
    else
        section->byteAlign = -1;

    if (version >= 4)
        section->root = fgetc(fileHandle) != 0;
	else
		section->root = false;

    section->totalSymbols = readSymbols(fileHandle, &section->symbols);

    if (version >= 2) {
        section->totalLineMappings = readLineMappings(fileHandle, &section->lineMappings, fileInfoIndex);
    } else {
        section->totalLineMappings = 0;
        section->lineMappings = NULL;
    }

    section->size = fgetll(fileHandle);
    if (group_isText(section->group)) {
        if ((section->data = mem_Alloc(section->size)) != NULL) {
            if (section->size != fread(section->data, 1, section->size, fileHandle))
                error("File read failed");
            section->patches = readPatches(fileHandle);
        }
    }
}

static SSection**
readSections(Groups* groups, FILE* fileHandle, int version, uint32_t fileInfoIndex) {
    uint32_t totalSections = fgetll(fileHandle);
    SSection** sections = mem_Alloc(sizeof(SSection*) * totalSections);

    for (uint32_t i = 0; i < totalSections; ++i) {
        SSection* section = sect_CreateNew();
        sections[i] = section;

        section->minimumWordSize = g_minimumWordSize;
        section->fileId = g_fileId;

        readSection(fileHandle, section, groups, version, fileInfoIndex);

        if (group_isText(section->group) && strcmp(section->group->name, "HOME") == 0) {
            section->cpuBank = 0;
        }

    }

    g_fileId += 1;

    return sections;
}

static SFileInfo* findFileInfo(string* filename, uint32_t crc32) {
    for (uint32_t i = 0; i < g_fileInfoCount; ++i) {
        if (str_Equal(g_fileInfo[i].fileName, filename) && g_fileInfo[i].crc32 == crc32) {
            return &g_fileInfo[i];
        }
    }
    return NULL;
}

static uint32_t
readFileInfo(FILE* fileHandle) {
    uint32_t fileInfoIndex = g_fileInfoCount;
    uint32_t fileInfoInObject = fgetll(fileHandle);

    g_fileInfoCount += fileInfoInObject;
    if (g_fileInfoCount > 0) {
        g_fileInfo = mem_Realloc(g_fileInfo, sizeof(SFileInfo) * g_fileInfoCount);

        for (uint32_t i = 0; i < fileInfoInObject; ++i) {
            uint32_t index = i + fileInfoIndex;
            g_fileInfo[index].fileName = fgetstr(fileHandle);
            g_fileInfo[index].crc32 = fgetll(fileHandle);

            SFileInfo* fileInfo = findFileInfo(g_fileInfo[i].fileName, g_fileInfo[i].crc32);
            if (fileInfo != NULL) {
                g_fileInfo[index].index = fileInfo->index;
            } else {
                g_fileInfo[index].index = index;

            }
        } 
    }

    return fileInfoIndex;
}

static void
readXOB0(FILE* fileHandle) {
    g_minimumWordSize = 1;
    SSection** sections = readSections(readGroups(fileHandle), fileHandle, 0, 0);
    mem_Free(sections);
}

static void
readXOB1(FILE* fileHandle) {
    g_minimumWordSize = fgetc(fileHandle);
    SSection** sections = readSections(readGroups(fileHandle), fileHandle, 1, 0);
    mem_Free(sections);
}

static void
readXOBn(FILE* fileHandle, int32_t version) {
    g_minimumWordSize = fgetc(fileHandle);
    uint32_t fileInfoIndex = readFileInfo(fileHandle);
    SSection** sections = readSections(readGroups(fileHandle), fileHandle, version, fileInfoIndex);
    mem_Free(sections);
}

static void
readChunk(FILE* fileHandle);

static void
readXLB0(FILE* fileHandle) {
    uint32_t count = fgetll(fileHandle);

    while (count--) {
        while (fgetc(fileHandle)) {
        }  // Skip name
        fgetll(fileHandle);           // Skip length

        readChunk(fileHandle);
    }
}

static void
readChunk(FILE* fileHandle) {
    uint32_t id = fgetll(fileHandle);

    switch (id) {
        case MAKE_ID('X', 'O', 'B', 0): {
            readXOB0(fileHandle);
            break;
        }

        case MAKE_ID('X', 'O', 'B', 1): {
            readXOB1(fileHandle);
            break;
        }

        case MAKE_ID('X', 'O', 'B', 2): {
            readXOBn(fileHandle, 2);
            break;
        }

        case MAKE_ID('X', 'O', 'B', 3): {
            readXOBn(fileHandle, 3);
            break;
        }

        case MAKE_ID('X', 'O', 'B', 4): {
            readXOBn(fileHandle, 4);
            break;
        }

        case MAKE_ID('X', 'L', 'B', 0): {
            readXLB0(fileHandle);
            break;
        }

        default: {
            error("Unknown file type");
        }
    }
}

void
obj_Read(char* fileName) {
    FILE* fileHandle;

    if ((fileHandle = fopen(fileName, "rb")) != NULL) {
        size_t size = fsize(fileHandle);

        while ((size_t) ftell(fileHandle) < size)
            readChunk(fileHandle);

        fclose(fileHandle);
    } else {
        error("File \"%s\" not found", fileName);
    }
}
