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

#ifndef XLINK_SECTION_H_INCLUDED_
#define XLINK_SECTION_H_INCLUDED_

#include "str.h"
#include "types.h"

#include "group.h"
#include "object.h"
#include "patch.h"
#include "symbol.h"

typedef struct FileInfo {
    string* fileName;
    uint32_t crc32;
    uint32_t index;
} SFileInfo;

typedef struct LineMapping {
    SFileInfo* fileInfo;
	uint32_t lineNumber;
	uint32_t offset;
} SLineMapping;

typedef struct Section {
    uint32_t fileId;
    uint32_t sectionId;

    Group* group;

    // Before they are assigned, bank, byteLocation and basePC reflect the programmer's wish.
    // After, they point to where this section actually is
    int32_t cpuByteLocation; // Where the CPU sees this section, in bytes
    int32_t cpuBank;
    int32_t cpuLocation;
    int32_t imageLocation;
    int32_t minimumWordSize;
    int32_t byteAlign;
	bool root;

    char name[MAX_SYMBOL_NAME_LENGTH];

    uint32_t totalSymbols;
    SSymbol* symbols;

    uint32_t totalLineMappings;
    SLineMapping* lineMappings;

    uint32_t size;
    uint8_t* data;

    SPatches* patches;

    bool used;
    bool assigned;

    struct Section* nextSection;
} SSection;

extern SSection*
sect_Sections;

extern SSection*
sect_CreateNew(void);

extern SSymbol*
sect_GetSymbol(SSection* section, uint32_t symbolId, bool allowImports);

extern bool
sect_GetConstantSymbolBank(SSection* section, uint32_t symbolId, int32_t* outValue);

extern char*
sect_GetSymbolName(SSection* section, uint32_t symbolId);

extern void
sect_ForEachUsedSection(void (* function)(SSection*, intptr_t), intptr_t data);

extern uint32_t
sect_TotalSections(void);

extern void
sect_SortSections(void);

extern SSection*
sect_FindSectionWithExportedSymbol(const char* symbol);

extern SSection*
sect_FindSectionWithLocallyExportedSymbol(const char* symbolName, uint32_t fileId);

extern bool
sect_IsEquSection(SSection* section);

extern void
sect_ResolveUnresolved(void);

#endif
