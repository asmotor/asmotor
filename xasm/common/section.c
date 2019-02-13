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

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <assert.h>

#include "mem.h"

#include "asmotor.h"

#include "xasm.h"
#include "errors.h"
#include "expression.h"
#include "filestack.h"
#include "options.h"
#include "parse.h"
#include "patch.h"
#include "section.h"
#include "symbol.h"


#define SECTION_GROWTH_AMOUNT 0x4000U

SSection* sect_Current;
SSection* sect_Sections;


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
	SSection* newSection = sect_Sections;
	while (newSection) {
		if (str_Equal(newSection->name, name)) {
			if (group) {
				if (newSection->group == group) {
					return newSection;
				} else {
                    err_Fail(ERROR_SECT_EXISTS);
					return NULL;
				}
			} else {
				return newSection;
			}
		}
		newSection = list_GetNext(newSection);
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
	assert(xasm_Configuration->minimumWordSize <= count);

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
	assert(xasm_Configuration->minimumWordSize <= count);

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

void
sect_OutputConst8(uint8_t value) {
	assert(xasm_Configuration->minimumWordSize <= MINSIZE_8BIT);

	if (checkAvailableSpace(1)) {
		switch (currentSectionType()) {
			case GROUP_TEXT: {
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

void
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
sect_OutputBinaryFile(string* pFile) {
	/* TODO: Handle minimum word size.
	 * Pad file if necessary.
	 * Read words and output in chosen endianness
	 */

	FILE* f;

	if ((pFile = fstk_FindFile(pFile)) != NULL && (f = fopen(str_String(pFile), "rb")) != NULL) {
		uint32_t size;

		fseek(f, 0, SEEK_END);
		size = ftell(f);
		fseek(f, 0, SEEK_SET);

		if (checkAvailableSpace(size)) {
			switch (currentSectionType()) {
				case GROUP_TEXT: {
					size_t read;

					read = fread(&sect_Current->data[sect_Current->usedSpace], sizeof(uint8_t), size, f);
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

		fclose(f);
	} else {
        err_Fail(ERROR_NO_FILE);
	}

	str_Free(pFile);
}

void
sect_Align(uint32_t alignment) {
	assert(xasm_Configuration->minimumWordSize <= alignment);

	uint32_t t = alignToNext(sect_Current->usedSpace, alignment);
	sect_SkipBytes(t - sect_Current->usedSpace);
}

void
sect_SkipBytes(uint32_t count) {
	if (count == 0)
		return;

	assert(xasm_Configuration->minimumWordSize <= count);

	if (checkAvailableSpace(count)) {
		//printf("*DEBUG* skipping %d bytes\n", count);
		switch (currentSectionType()) {
			case GROUP_TEXT: {
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
sect_SwitchTo_LOAD(const string* sectname, SSymbol* group, uint32_t load) {
	SSection* newSection;

	if ((newSection = findSection(sectname, group)) != NULL) {
		if (newSection->flags == SECTF_LOADFIXED && newSection->cpuOrigin == load) {
			sect_Current = newSection;
			return true;
		} else {
            err_Fail(ERROR_SECT_EXISTS_LOAD);
			return false;
		}
	} else {
		if ((newSection = createSection(sectname)) != NULL) {
			newSection->group = group;
			newSection->flags = SECTF_LOADFIXED;
			newSection->cpuOrigin = load;
			newSection->imagePosition = load * xasm_Configuration->minimumWordSize;
		}
		sect_Current = newSection;
		return newSection != NULL;
	}
}

bool
sect_SwitchTo_BANK(const string* sectname, SSymbol* group, uint32_t bank) {
	SSection* newSection;

	if (!xasm_Configuration->supportBanks)
		internalerror("Banks not supported");

	newSection = findSection(sectname, group);
	if (newSection) {
		if (newSection->flags == SECTF_BANKFIXED && newSection->bank == bank) {
			sect_Current = newSection;
			return true;
		}

        err_Fail(ERROR_SECT_EXISTS_BANK);
		return false;
	}

	newSection = createSection(sectname);
	if (newSection) {
		newSection->group = group;
		newSection->flags = SECTF_BANKFIXED;
		newSection->bank = bank;
	}
	sect_Current = newSection;
	return newSection != NULL;
}

bool
sect_SwitchTo_LOAD_BANK(const string* sectname, SSymbol* group, uint32_t origin, uint32_t bank) {
	SSection* newSection;

	if (!xasm_Configuration->supportBanks)
		internalerror("Banks not supported");

	if ((newSection = findSection(sectname, group)) != NULL) {
		if (newSection->flags == (SECTF_BANKFIXED | SECTF_LOADFIXED) && newSection->bank == bank && newSection->cpuOrigin == origin) {
			sect_Current = newSection;
			return true;
		}

        err_Fail(ERROR_SECT_EXISTS_BANK_LOAD);
		return false;
	}

	if ((newSection = createSection(sectname)) != NULL) {
		newSection->group = group;
		newSection->flags = SECTF_BANKFIXED | SECTF_LOADFIXED;
		newSection->bank = bank;
		newSection->cpuOrigin = origin;
		newSection->imagePosition = origin * xasm_Configuration->minimumWordSize;
	}

	sect_Current = newSection;
	return newSection != NULL;
}

bool
sect_SwitchTo_NAMEONLY(const string* sectname) {
	if ((sect_Current = findSection(sectname, NULL)) != NULL) {
		return true;
	} else {
        err_Fail(ERROR_NO_SECT);
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
