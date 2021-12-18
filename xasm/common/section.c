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

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <assert.h>

#include "util.h"
#include "file.h"
#include "mem.h"

#include "xasm.h"
#include "dependency.h"
#include "errors.h"
#include "expression.h"
#include "lexer_context.h"
#include "includes.h"
#include "linemap.h"
#include "options.h"
#include "parse.h"
#include "patch.h"
#include "section.h"
#include "symbol.h"


#define SECTION_GROWTH_AMOUNT 0x4000U

SSection* sect_Current;
SSection* sect_Sections;

typedef struct SectionStackEntry {
	SSection* section;
	SSymbol* scope;
	struct SectionStackEntry* next;
} SSectionStackEntry;

static SSectionStackEntry* g_sectionStack = NULL;

static void
freeSection(SSection* section) {
	if (section->lineMap != NULL)
		linemap_Free(section->lineMap);

	SPatch* patch = section->patches;
	while (patch != NULL) {
		SPatch* next = list_GetNext(patch);
		patch_Free(patch);
		patch = next;
	}

	str_Free(section->name);
	mem_Free(section->data);
	mem_Free(section);

}

static EGroupType
currentSectionType(void) {
	if (sect_Current == NULL)
		internalerror("No SECTION defined");

	if (sect_Current->group == NULL)
		internalerror("No GROUP defined for SECTION");

	if (sect_Current->group->type != SYM_GROUP)
		internalerror("SECTION's GROUP symbol is not of type SYM_GROUP");

	return sect_Current->group->value.groupType;
}

static SSection*
createSection(const string* name) {
	SSection* newSection = mem_Alloc(sizeof(SSection));
	memset(newSection, 0, sizeof(SSection));

	newSection->name = str_Copy(name);
	newSection->freeSpace = xasm_Configuration->maxSectionSize;

	if (sect_Sections != NULL) {
		SSection* lastSection = sect_Sections;
		while (lastSection->pNext)
			lastSection = list_GetNext(lastSection);
		list_InsertAfter(lastSection, newSection);
	} else {
		sect_Sections = newSection;
	}
	return newSection;
}

static SSection*
findSection(const string* name, SSymbol* group) {
	for (SSection* newSection = sect_Sections; newSection != NULL; newSection = list_GetNext(newSection)) {
		if (str_Equal(newSection->name, name)) {
			if (group != NULL) {
				if (newSection->group == group) {
					return newSection;
				} else {
                    err_Error(ERROR_SECT_EXISTS);
					return NULL;
				}
			} else {
				return newSection;
			}
		}
	}

	return NULL;
}

static uint32_t
alignToNext(uint32_t value, uint32_t alignment) {
	uint32_t t = value + alignment - 1;
	t -= t % alignment;
	return t;
}

static void
growCurrentSection(uint32_t count) {
	assert((uint32_t) xasm_Configuration->minimumWordSize <= count);

	if (count + sect_Current->usedSpace > sect_Current->allocatedSpace) {
		uint32_t allocate = alignToNext(count + sect_Current->usedSpace, SECTION_GROWTH_AMOUNT);
		if ((sect_Current->data = mem_Realloc(sect_Current->data, allocate)) != NULL) {
			sect_Current->allocatedSpace = allocate;
		} else {
			internalerror("Out of memory!");
		}
	}
}

static bool
checkAvailableSpace(uint32_t count) {
	assert((uint32_t) xasm_Configuration->minimumWordSize <= count);

	if (sect_Current != NULL) {
		if (count <= sect_Current->freeSpace) {
			if (currentSectionType() == GROUP_TEXT) {
				growCurrentSection(count);
			}
			return true;
		} else {
            err_Error(ERROR_SECTION_FULL);
			return false;
		}
	} else {
        err_Error(ERROR_SECTION_MISSING);
		return false;
	}
}


/* Public functions */

uint32_t
sect_CurrentSize(void) {
	return sect_Current->usedSpace;
}


void
sect_OutputConst16At(uint16_t value, uint32_t offset) {
	if (offset + 2 <= sect_CurrentSize()) {
		switch (currentSectionType()) {
			case GROUP_TEXT: {
				switch (opt_Current->endianness) {
					case ASM_LITTLE_ENDIAN: {
						sect_Current->data[offset++] = (uint8_t) (value);
						sect_Current->data[offset++] = (uint8_t) (value >> 8u);
						break;
					}
					case ASM_BIG_ENDIAN: {
						sect_Current->data[offset++] = (uint8_t) (value >> 8u);
						sect_Current->data[offset++] = (uint8_t) (value);
						break;
					}
					default: {
						internalerror("Unknown endianness");
						break;
					}
				}
				break;
			}
			case GROUP_BSS: {
                err_Error(ERROR_SECTION_DATA);
				break;
			}
			default: {
				internalerror("Unknown GROUP type");
				break;
			}
		}
	}	
}


void
sect_OutputConst8(uint8_t value) {
	assert(xasm_Configuration->minimumWordSize <= MINSIZE_8BIT);

	if (checkAvailableSpace(1)) {
		switch (currentSectionType()) {
			case GROUP_TEXT: {
				linemap_AddCurrent();

				sect_Current->freeSpace -= 1;
				sect_Current->data[sect_Current->usedSpace++] = value;
				sect_Current->cpuProgramCounter += 1;
				break;
			}
			case GROUP_BSS: {
                err_Error(ERROR_SECTION_DATA);
				break;
			}
			default: {
				internalerror("Unknown GROUP type");
				break;
			}
		}
	}
}

void
sect_OutputReloc8(SExpression* expression) {
	assert(xasm_Configuration->minimumWordSize <= MINSIZE_8BIT);

	if (checkAvailableSpace(1)) {
		switch (currentSectionType()) {
			case GROUP_TEXT: {
				linemap_AddCurrent();

				patch_Create(sect_Current, sect_Current->usedSpace, expression, PATCH_8);
				sect_Current->cpuProgramCounter += 1;
				sect_Current->usedSpace += 1;
				sect_Current->freeSpace -= 1;
				break;
			}
			case GROUP_BSS: {
                err_Error(ERROR_SECTION_DATA);
				sect_SkipBytes(1);
				break;
			}
			default: {
				internalerror("Unknown GROUP type");
				break;
			}
		}
	}
}

void sect_OutputExpr8(SExpression* expression) {
	assert(xasm_Configuration->minimumWordSize <= MINSIZE_8BIT);

	if (expression == NULL) {
        err_Error(ERROR_EXPR_BAD);
	} else if (expr_IsConstant(expression)) {
		sect_OutputConst8((uint8_t) expression->value.integer);
		expr_Free(expression);
	} else {
		sect_OutputReloc8(expression);
	}
}

void
sect_OutputConst16(uint16_t value) {
	assert(xasm_Configuration->minimumWordSize <= MINSIZE_16BIT);

	if (checkAvailableSpace(2)) {
		switch (currentSectionType()) {
			case GROUP_TEXT: {
				linemap_AddCurrent();

				switch (opt_Current->endianness) {
					case ASM_LITTLE_ENDIAN: {
						sect_Current->data[sect_Current->usedSpace++] = (uint8_t) (value);
						sect_Current->data[sect_Current->usedSpace++] = (uint8_t) (value >> 8u);
						break;
					}
					case ASM_BIG_ENDIAN: {
						sect_Current->data[sect_Current->usedSpace++] = (uint8_t) (value >> 8u);
						sect_Current->data[sect_Current->usedSpace++] = (uint8_t) (value);
						break;
					}
					default: {
						internalerror("Unknown endianness");
						break;
					}
				}
				sect_Current->freeSpace -= 2;
				sect_Current->cpuProgramCounter += 2 / xasm_Configuration->minimumWordSize;
				break;
			}
			case GROUP_BSS: {
                err_Error(ERROR_SECTION_DATA);
				break;
			}
			default: {
				internalerror("Unknown GROUP type");
				break;
			}
		}
	}
}

void
sect_OutputReloc16(SExpression* expression) {
	assert(xasm_Configuration->minimumWordSize <= MINSIZE_16BIT);

	if (checkAvailableSpace(2)) {
		switch (currentSectionType()) {
			case GROUP_TEXT: {
				linemap_AddCurrent();

				patch_Create(sect_Current, sect_Current->usedSpace, expression,
							 opt_Current->endianness == ASM_LITTLE_ENDIAN ? PATCH_LE_16 : PATCH_BE_16);
				sect_Current->freeSpace -= 2;
				sect_Current->usedSpace += 2;
				sect_Current->cpuProgramCounter += 2 / xasm_Configuration->minimumWordSize;
				break;
			}
			case GROUP_BSS: {
                err_Error(ERROR_SECTION_DATA);
				sect_SkipBytes(2);
				break;
			}
			default: {
				internalerror("Unknown GROUP type");
				break;
			}
		}
	}
}

void
sect_OutputExpr16(SExpression* expression) {
	assert(xasm_Configuration->minimumWordSize <= MINSIZE_16BIT);

	if (expression == NULL) {
        err_Error(ERROR_EXPR_BAD);
	} else if (expr_IsConstant(expression)) {
		sect_OutputConst16((uint16_t) (expression->value.integer));
		expr_Free(expression);
	} else {
		sect_OutputReloc16(expression);
	}
}

void
sect_OutputConst32(uint32_t value) {
	assert(xasm_Configuration->minimumWordSize <= MINSIZE_32BIT);

	if (checkAvailableSpace(4)) {
		switch (currentSectionType()) {
			case GROUP_TEXT: {
				linemap_AddCurrent();

				switch (opt_Current->endianness) {
					case ASM_LITTLE_ENDIAN: {
						sect_Current->data[sect_Current->usedSpace++] = (uint8_t) (value);
						sect_Current->data[sect_Current->usedSpace++] = (uint8_t) (value >> 8u);
						sect_Current->data[sect_Current->usedSpace++] = (uint8_t) (value >> 16u);
						sect_Current->data[sect_Current->usedSpace++] = (uint8_t) (value >> 24u);
						break;
					}
					case ASM_BIG_ENDIAN: {
						sect_Current->data[sect_Current->usedSpace++] = (uint8_t) (value >> 24u);
						sect_Current->data[sect_Current->usedSpace++] = (uint8_t) (value >> 16u);
						sect_Current->data[sect_Current->usedSpace++] = (uint8_t) (value >> 8u);
						sect_Current->data[sect_Current->usedSpace++] = (uint8_t) (value);
						break;
					}
					default: {
						internalerror("Unknown endianness");
						break;
					}
				}
				sect_Current->freeSpace -= 4;
				sect_Current->cpuProgramCounter += 4 / xasm_Configuration->minimumWordSize;
				break;
			}
			case GROUP_BSS: {
                err_Error(ERROR_SECTION_DATA);
				break;
			}
			default: {
				internalerror("Unknown GROUP type");
				break;
			}
		}
	}
}

void
sect_OutputRel32(SExpression* expression) {
	assert(xasm_Configuration->minimumWordSize <= MINSIZE_32BIT);

	if (checkAvailableSpace(4)) {
		switch (currentSectionType()) {
			case GROUP_TEXT: {
				linemap_AddCurrent();

				patch_Create(sect_Current, sect_Current->usedSpace, expression,
							 opt_Current->endianness == ASM_LITTLE_ENDIAN ? PATCH_LE_32 : PATCH_BE_32);
				sect_Current->freeSpace -= 4;
				sect_Current->cpuProgramCounter += 4 / xasm_Configuration->minimumWordSize;
				sect_Current->usedSpace += 4;
				break;
			}
			case GROUP_BSS: {
                err_Error(ERROR_SECTION_DATA);
				sect_SkipBytes(4);
				break;
			}
			default: {
				internalerror("Unknown GROUP type");
				break;
			}
		}
	}
}

extern void
sect_OutputExpr32(SExpression* expression) {
	assert(xasm_Configuration->minimumWordSize <= MINSIZE_32BIT);

	if (expression == NULL) {
        err_Error(ERROR_EXPR_BAD);
	} else if (expr_IsConstant(expression)) {
		sect_OutputConst32(expression->value.integer);
		expr_Free(expression);
	} else {
		sect_OutputRel32(expression);
	}
}


void
sect_OutputFloat32(long double value) {
	float floatValue = (float) value;
	uint32_t intValue;
	assert(sizeof(floatValue) == sizeof(intValue));
	memcpy(&intValue, &floatValue, sizeof(uint32_t));
	sect_OutputConst32(intValue);
}


void
sect_OutputFloat64(long double value) {
	double floatValue = (double) value;
	uint64_t intValue;
	assert(sizeof(floatValue) == sizeof(intValue));
	memcpy(&intValue, &floatValue, sizeof(uint64_t));
	sect_OutputConst32((uint32_t) intValue);
	sect_OutputConst32((uint32_t) (intValue >> 32));
}


void
sect_OutputBinaryFile(string* filename) {
	FILE* fileHandle;

	if ((filename = inc_FindFile(filename)) != NULL && (fileHandle = fopen(str_String(filename), "rb")) != NULL) {
		dep_AddDependency(filename);

		fseek(fileHandle, 0, SEEK_END);
		uint32_t size = ftell(fileHandle);
		fseek(fileHandle, 0, SEEK_SET);

		if (checkAvailableSpace(size)) {
			switch (currentSectionType()) {
				case GROUP_TEXT: {
					linemap_AddCurrent();

					size_t read = fread(&sect_Current->data[sect_Current->usedSpace], sizeof(uint8_t), size, fileHandle);
					sect_Current->freeSpace -= size;
					sect_Current->usedSpace += size;
					sect_Current->cpuProgramCounter += size / xasm_Configuration->minimumWordSize;
					if (read != size) {
                        err_Fail(ERROR_READ);
					}
					break;
				}
				case GROUP_BSS: {
                    err_Error(ERROR_SECTION_DATA);
					break;
				}
				default: {
					internalerror("Unknown GROUP type");
					break;
				}
			}
		}

		fclose(fileHandle);
	} else {
        err_Error(ERROR_NO_FILE);
	}

	str_Free(filename);
}

void
sect_Align(uint32_t alignment) {
	assert((uint32_t) xasm_Configuration->minimumWordSize <= alignment);

	uint32_t t = alignToNext(sect_Current->usedSpace, alignment);
	sect_SkipBytes(t - sect_Current->usedSpace);
}

void
sect_SkipBytes(uint32_t count) {
	if (count == 0)
		return;

	assert((uint32_t) xasm_Configuration->minimumWordSize <= count);

	if (checkAvailableSpace(count)) {
		linemap_AddCurrent();

		switch (currentSectionType()) {
			case GROUP_TEXT: {
				while (count >= 2) {
					sect_OutputConst16((opt_Current->uninitializedValue << 8) | opt_Current->uninitializedValue);
					count -= 2;
				}
				while (count--) {
					sect_OutputConst8((uint8_t) opt_Current->uninitializedValue);
				}
				break;
			}
			case GROUP_BSS: {
				sect_Current->freeSpace -= count;
				sect_Current->usedSpace += count;
				sect_Current->cpuProgramCounter += count / xasm_Configuration->minimumWordSize;
				break;
			}
			default: {
				internalerror("Unknown GROUP type");
			}
		}
	}
}

bool
sect_SwitchTo(const string* sectname, SSymbol* group) {
	SSection* newSection = findSection(sectname, group);
	if (newSection) {
		sect_Current = newSection;
		return true;
	} else {
		newSection = createSection(sectname);
		if (newSection) {
			newSection->group = group;
			newSection->flags = 0;
		}
		sect_Current = newSection;
		return newSection != NULL;
	}
}

bool
sect_SwitchTo_KIND(const string* sectname, SSymbol* group, uint32_t flags, uint32_t origin, uint32_t bank, uint32_t align) {
	SSection* newSection;

	if ((flags & SECTF_BANKFIXED) && !xasm_Configuration->supportBanks)
		internalerror("Banks not supported");

	if ((newSection = findSection(sectname, group)) != NULL) {
		if (newSection->flags == flags 
		&& ((flags & SECTF_BANKFIXED) == 0 || newSection->bank == bank)
		&& ((flags & SECTF_LOADFIXED) == 0 || newSection->cpuOrigin == origin)
		&& ((flags & SECTF_ALIGNED) == 0 || newSection->align == align)) {
			sect_Current = newSection;
			return true;
		}

        err_Error(ERROR_SECT_EXISTS_DIFFERENT_KIND);
		return false;
	}

	if ((newSection = createSection(sectname)) != NULL) {
		newSection->group = group;
		newSection->flags = flags;
		newSection->bank = (flags & SECTF_BANKFIXED) ? bank : 0;
		newSection->align = (flags & SECTF_ALIGNED) ? align : 0;
		newSection->cpuOrigin = (flags & SECTF_LOADFIXED) ? origin : 0;
		newSection->imagePosition = newSection->cpuOrigin * xasm_Configuration->minimumWordSize;
	}

	sect_Current = newSection;
	return newSection != NULL;
}

bool
sect_SwitchTo_NAMEONLY(const string* sectname) {
	if ((sect_Current = findSection(sectname, NULL)) != NULL) {
		return true;
	} else {
        err_Error(ERROR_NO_SECT);
		return false;
	}
}

bool
sect_Init(void) {
	sect_Current = NULL;
	sect_Sections = NULL;
	return true;
}

void
sect_Exit(void) {
	SSection* section = sect_Sections;
	while (section != NULL) {
		SSection* next = list_GetNext(section);
		freeSection(section);
		section = next;
	}
}

void
sect_SetOriginAddress(uint32_t org) {
	if (sect_Current == NULL) {
        err_Error(ERROR_SECTION_MISSING);
	} else {
		sect_Current->flags |= SECTF_ORGFIXED;
		sect_Current->cpuAdjust = org - (sect_Current->cpuProgramCounter + sect_Current->cpuOrigin);
	}
}

uint32_t 
sect_TotalSections(void) {
	uint32_t totalSections = 0;
	for (const SSection* section = sect_Sections; section != NULL; section = list_GetNext(section)) {
		++totalSections;
	}
	return totalSections;
}

bool
sect_Push(void) {
	SSectionStackEntry* entry = (SSectionStackEntry*) mem_Alloc(sizeof(SSectionStackEntry));
	if (entry != NULL) {
		entry->section = sect_Current;
		entry->scope = sym_CurrentScope;
		entry->next = g_sectionStack;
		g_sectionStack = entry;
		return true;
	}

	return false;
}

bool
sect_Pop(void) {
	SSectionStackEntry* entry = g_sectionStack;
	if (entry != NULL) {
		sect_Current = entry->section;
		sym_CurrentScope = entry->scope;
		g_sectionStack = entry->next;
		mem_Free(entry);
		return true;
	}

	return false;
}

