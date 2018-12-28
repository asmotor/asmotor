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

#include "asmotor.h"
#include "xasm.h"
#include "mem.h"
#include "section.h"
#include "symbol.h"
#include "project.h"
#include "expression.h"
#include "patch.h"
#include "parse.h"
#include "options.h"
#include "filestack.h"




//	Private defines

#define CHUNK_SIZE 0x4000U




//	Public variables

SSection* sect_Current;
SSection* sect_Sections;




//	Private routines

static EGroupType sect_GetCurrentType(void) {
	if (sect_Current == NULL)
		internalerror("No SECTION defined");

	if (sect_Current->group == NULL)
		internalerror("No GROUP defined for SECTION");

	if (sect_Current->group->type != SYM_GROUP)
		internalerror("SECTION's GROUP symbol is not of type SYM_GROUP");

	return sect_Current->group->value.groupType;
}

static SSection* sect_Create(const string* name) {
	SSection* sect = mem_Alloc(sizeof(SSection));
	memset(sect, 0, sizeof(SSection));

	sect->name = str_Copy(name);
	sect->freeSpace = g_pConfiguration->nMaxSectionSize;

	if (sect_Sections) {
		SSection* list = sect_Sections;
		while (list->pNext)
			list = list_GetNext(list);
		list_InsertAfter(list, sect);
	} else {
		sect_Sections = sect;
	}
	return sect;
}

static SSection* sect_Find(const string* name, SSymbol* group) {
	SSection* sect;

	sect = sect_Sections;
	while (sect) {
		if (str_Equal(sect->name, name)) {
			if (group) {
				if (sect->group == group) {
					return sect;
				} else {
					prj_Fail(ERROR_SECT_EXISTS);
					return NULL;
				}
			} else {
				return sect;
			}
		}
		sect = list_GetNext(sect);
	}

	return NULL;
}

static void sect_GrowCurrent(uint32_t count) {
	assert(g_pConfiguration->eMinimumWordSize <= count);

	if (count + sect_Current->usedSpace > sect_Current->allocatedSpace) {
		uint32_t allocate = (count + sect_Current->usedSpace + CHUNK_SIZE - 1) & -CHUNK_SIZE;
		if ((sect_Current->data = mem_Realloc(sect_Current->data, allocate)) != NULL) {
			sect_Current->allocatedSpace = allocate;
		} else {
			internalerror("Out of memory!");
		}
	}
}

static bool sect_CheckAvailableSpace(uint32_t count) {
	assert(g_pConfiguration->eMinimumWordSize <= count);

	if (sect_Current) {
		if (count <= sect_Current->freeSpace) {
			if (sect_GetCurrentType() == GROUP_TEXT) {
				sect_GrowCurrent(count);
			}
			return true;
		} else {
			prj_Error(ERROR_SECTION_FULL);
			return false;
		}
	} else {
		prj_Error(ERROR_SECTION_MISSING);
		return false;
	}
}




//	Public routines

void sect_OutputConst8(uint8_t value) {
	assert(g_pConfiguration->eMinimumWordSize <= MINSIZE_8BIT);

	if (sect_CheckAvailableSpace(1)) {
		switch (sect_GetCurrentType()) {
			case GROUP_TEXT: {
				sect_Current->freeSpace -= 1;
				sect_Current->data[sect_Current->usedSpace++] = value;
				sect_Current->cpuProgramCounter += 1;
				break;
			}
			case GROUP_BSS: {
				prj_Error(ERROR_SECTION_DATA);
				break;
			}
			default: {
				internalerror("Unknown GROUP type");
				break;
			}
		}
	}
}

void sect_OutputReloc8(SExpression* expr) {
	assert(g_pConfiguration->eMinimumWordSize <= MINSIZE_8BIT);

	if (sect_CheckAvailableSpace(1)) {
		switch (sect_GetCurrentType()) {
			case GROUP_TEXT: {
				patch_Create(sect_Current, sect_Current->usedSpace, expr, PATCH_8);
				sect_Current->cpuProgramCounter += 1;
				sect_Current->usedSpace += 1;
				sect_Current->freeSpace -= 1;
				break;
			}
			case GROUP_BSS: {
				prj_Error(ERROR_SECTION_DATA);
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

void sect_OutputExpr8(SExpression* expr) {
	assert(g_pConfiguration->eMinimumWordSize <= MINSIZE_8BIT);

	if (expr == NULL) {
		prj_Error(ERROR_EXPR_BAD);
	} else if (expr_IsConstant(expr)) {
		sect_OutputConst8((uint8_t) expr->value.integer);
		expr_Free(expr);
	} else {
		sect_OutputReloc8(expr);
	}
}

void sect_OutputConst16(uint16_t value) {
	assert(g_pConfiguration->eMinimumWordSize <= MINSIZE_16BIT);

	if (sect_CheckAvailableSpace(2)) {
		switch (sect_GetCurrentType()) {
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
				sect_Current->cpuProgramCounter += 2 / g_pConfiguration->eMinimumWordSize;
				break;
			}
			case GROUP_BSS: {
				prj_Error(ERROR_SECTION_DATA);
				break;
			}
			default: {
				internalerror("Unknown GROUP type");
				break;
			}
		}
	}
}

void sect_OutputReloc16(SExpression* expr) {
	assert(g_pConfiguration->eMinimumWordSize <= MINSIZE_16BIT);

	if (sect_CheckAvailableSpace(2)) {
		switch (sect_GetCurrentType()) {
			case GROUP_TEXT: {
				patch_Create(sect_Current, sect_Current->usedSpace, expr,
							 opt_Current->endianness == ASM_LITTLE_ENDIAN ? PATCH_LE_16 : PATCH_BE_16);
				sect_Current->freeSpace -= 2;
				sect_Current->usedSpace += 2;
				sect_Current->cpuProgramCounter += 2 / g_pConfiguration->eMinimumWordSize;
				break;
			}
			case GROUP_BSS: {
				prj_Error(ERROR_SECTION_DATA);
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

void sect_OutputExpr16(SExpression* expr) {
	assert(g_pConfiguration->eMinimumWordSize <= MINSIZE_16BIT);

	if (expr == NULL) {
		prj_Error(ERROR_EXPR_BAD);
	} else if (expr_IsConstant(expr)) {
		sect_OutputConst16((uint16_t) (expr->value.integer));
		expr_Free(expr);
	} else {
		sect_OutputReloc16(expr);
	}
}

void sect_OutputConst32(uint32_t value) {
	assert(g_pConfiguration->eMinimumWordSize <= MINSIZE_32BIT);

	if (sect_CheckAvailableSpace(4)) {
		switch (sect_GetCurrentType()) {
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
				sect_Current->cpuProgramCounter += 4 / g_pConfiguration->eMinimumWordSize;
				break;
			}
			case GROUP_BSS: {
				prj_Error(ERROR_SECTION_DATA);
				break;
			}
			default: {
				internalerror("Unknown GROUP type");
				break;
			}
		}
	}
}

void sect_OutputRel32(SExpression* expr) {
	assert(g_pConfiguration->eMinimumWordSize <= MINSIZE_32BIT);

	if (sect_CheckAvailableSpace(4)) {
		switch (sect_GetCurrentType()) {
			case GROUP_TEXT: {
				patch_Create(sect_Current, sect_Current->usedSpace, expr,
							 opt_Current->endianness == ASM_LITTLE_ENDIAN ? PATCH_LE_32 : PATCH_BE_32);
				sect_Current->freeSpace -= 4;
				sect_Current->cpuProgramCounter += 4 / g_pConfiguration->eMinimumWordSize;
				sect_Current->usedSpace += 4;
				break;
			}
			case GROUP_BSS: {
				prj_Error(ERROR_SECTION_DATA);
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

void sect_OutputExpr32(SExpression* expr) {
	assert(g_pConfiguration->eMinimumWordSize <= MINSIZE_32BIT);

	if (expr == NULL) {
		prj_Error(ERROR_EXPR_BAD);
	} else if (expr_IsConstant(expr)) {
		sect_OutputConst32(expr->value.integer);
		expr_Free(expr);
	} else {
		sect_OutputRel32(expr);
	}
}

void sect_OutputBinaryFile(string* pFile) {
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

		if (sect_CheckAvailableSpace(size)) {
			switch (sect_GetCurrentType()) {
				case GROUP_TEXT: {
					size_t read;

					read = fread(&sect_Current->data[sect_Current->usedSpace], sizeof(uint8_t), size, f);
					sect_Current->freeSpace -= size;
					sect_Current->usedSpace += size;
					sect_Current->cpuProgramCounter += size / g_pConfiguration->eMinimumWordSize;
					if (read != size) {
						prj_Fail(ERROR_READ);
					}
					break;
				}
				case GROUP_BSS: {
					prj_Error(ERROR_SECTION_DATA);
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
		prj_Fail(ERROR_NO_FILE);
	}

	str_Free(pFile);
}

void sect_Align(uint32_t align) {
	assert(g_pConfiguration->eMinimumWordSize <= align);

	uint32_t t = sect_Current->usedSpace + align - 1;
	t -= t % align;
	sect_SkipBytes(t - sect_Current->usedSpace);
}

void sect_SkipBytes(uint32_t count) {
	if (count == 0)
		return;

	assert(g_pConfiguration->eMinimumWordSize <= count);

	if (sect_CheckAvailableSpace(count)) {
		//printf("*DEBUG* skipping %d bytes\n", count);
		switch (sect_GetCurrentType()) {
			case GROUP_TEXT: {
				while (count--) {
					sect_OutputConst8((uint8_t) opt_Current->uninitializedValue);
				}
				break;
			}
			case GROUP_BSS: {
				sect_Current->freeSpace -= count;
				sect_Current->usedSpace += count;
				sect_Current->cpuProgramCounter += count / g_pConfiguration->eMinimumWordSize;
				break;
			}
			default: {
				internalerror("Unknown GROUP type");
			}
		}
	}
}

bool sect_SwitchTo(const string* sectname, SSymbol* group) {
	SSection* sect;

	sect = sect_Find(sectname, group);
	if (sect) {
		sect_Current = sect;
		return true;
	} else {
		sect = sect_Create(sectname);
		if (sect) {
			sect->group = group;
			sect->flags = 0;
		}
		sect_Current = sect;
		return sect != NULL;
	}
}

bool sect_SwitchTo_LOAD(const string* sectname, SSymbol* group, uint32_t load) {
	SSection* sect;

	if ((sect = sect_Find(sectname, group)) != NULL) {
		if (sect->flags == SECTF_LOADFIXED && sect->cpuOrigin == load) {
			sect_Current = sect;
			return true;
		} else {
			prj_Fail(ERROR_SECT_EXISTS_LOAD);
			return false;
		}
	} else {
		if ((sect = sect_Create(sectname)) != NULL) {
			sect->group = group;
			sect->flags = SECTF_LOADFIXED;
			sect->cpuOrigin = load;
			sect->imagePosition = load * g_pConfiguration->eMinimumWordSize;
		}
		sect_Current = sect;
		return sect != NULL;
	}
}

bool sect_SwitchTo_BANK(const string* sectname, SSymbol* group, uint32_t bank) {
	SSection* sect;

	if (!g_pConfiguration->bSupportBanks)
		internalerror("Banks not supported");

	sect = sect_Find(sectname, group);
	if (sect) {
		if (sect->flags == SECTF_BANKFIXED && sect->bank == bank) {
			sect_Current = sect;
			return true;
		}

		prj_Fail(ERROR_SECT_EXISTS_BANK);
		return false;
	}

	sect = sect_Create(sectname);
	if (sect) {
		sect->group = group;
		sect->flags = SECTF_BANKFIXED;
		sect->bank = bank;
	}
	sect_Current = sect;
	return sect != NULL;
}

bool sect_SwitchTo_LOAD_BANK(const string* sectname, SSymbol* group, uint32_t origin, uint32_t bank) {
	SSection* sect;

	if (!g_pConfiguration->bSupportBanks)
		internalerror("Banks not supported");

	if ((sect = sect_Find(sectname, group)) != NULL) {
		if (sect->flags == (SECTF_BANKFIXED | SECTF_LOADFIXED) && sect->bank == bank && sect->cpuOrigin == origin) {
			sect_Current = sect;
			return true;
		}

		prj_Fail(ERROR_SECT_EXISTS_BANK_LOAD);
		return false;
	}

	if ((sect = sect_Create(sectname)) != NULL) {
		sect->group = group;
		sect->flags = SECTF_BANKFIXED | SECTF_LOADFIXED;
		sect->bank = bank;
		sect->cpuOrigin = origin;
		sect->imagePosition = origin * g_pConfiguration->eMinimumWordSize;
	}

	sect_Current = sect;
	return sect != NULL;
}

bool sect_SwitchTo_NAMEONLY(const string* sectname) {
	if ((sect_Current = sect_Find(sectname, NULL)) != NULL) {
		return true;
	} else {
		prj_Fail(ERROR_NO_SECT);
		return false;
	}
}

bool sect_Init(void) {
	sect_Current = NULL;
	sect_Sections = NULL;
	return true;
}

void sect_SetOrgAddress(uint32_t org) {
	if (sect_Current == NULL) {
		prj_Error(ERROR_SECTION_MISSING);
	} else {
		sect_Current->flags |= SECTF_ORGFIXED;
		sect_Current->cpuAdjust = org - (sect_Current->cpuProgramCounter + sect_Current->cpuOrigin);
	}
}

uint32_t sect_TotalSections(void) {
	uint32_t totalSections = 0;
	for (const SSection* pSect = sect_Sections; pSect != NULL; pSect = list_GetNext(pSect)) {
		++totalSections;
	}
	return totalSections;
}
