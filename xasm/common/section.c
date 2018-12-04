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

SSection* g_pCurrentSection;
SSection* g_pSectionList;




//	Private routines

static EGroupType sect_GetCurrentType(void) {
	if (g_pCurrentSection == NULL)
		internalerror("No SECTION defined");

	if (g_pCurrentSection->pGroup == NULL)
		internalerror("No GROUP defined for SECTION");

	if (g_pCurrentSection->pGroup->eType != SYM_GROUP)
		internalerror("SECTION's GROUP symbol is not of type SYM_GROUP");

	return g_pCurrentSection->pGroup->Value.GroupType;
}

static SSection* sect_Create(const char* name) {
	SSection* sect = mem_Alloc(sizeof(SSection));
	memset(sect, 0, sizeof(SSection));

	sect->Name = str_Create(name);
	sect->FreeSpace = g_pConfiguration->nMaxSectionSize;

	if (g_pSectionList) {
		SSection* list = g_pSectionList;
		while (list->pNext)
			list = list_GetNext(list);
		list_InsertAfter(list, sect);
	} else {
		g_pSectionList = sect;
	}
	return sect;
}

static SSection* sect_Find(const char* name, SSymbol* group) {
	SSection* sect;

	sect = g_pSectionList;
	while (sect) {
		if (str_EqualConst(sect->Name, name)) {
			if (group) {
				if (sect->pGroup == group) {
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

	if (count + g_pCurrentSection->UsedSpace > g_pCurrentSection->AllocatedSpace) {
		uint32_t allocate = (count + g_pCurrentSection->UsedSpace + CHUNK_SIZE - 1) & -CHUNK_SIZE;
		if ((g_pCurrentSection->pData = mem_Realloc(g_pCurrentSection->pData, allocate)) != NULL) {
			g_pCurrentSection->AllocatedSpace = allocate;
		} else {
			internalerror("Out of memory!");
		}
	}
}

static bool sect_CheckAvailableSpace(uint32_t count) {
	assert(g_pConfiguration->eMinimumWordSize <= count);

	if (g_pCurrentSection) {
		if (count <= g_pCurrentSection->FreeSpace) {
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
				g_pCurrentSection->FreeSpace -= 1;
				g_pCurrentSection->pData[g_pCurrentSection->UsedSpace++] = value;
				g_pCurrentSection->PC += 1;
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
				patch_Create(g_pCurrentSection, g_pCurrentSection->UsedSpace, expr, PATCH_BYTE);
				g_pCurrentSection->PC += 1;
				g_pCurrentSection->UsedSpace += 1;
				g_pCurrentSection->FreeSpace -= 1;
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

	if (expr == NULL)
		prj_Error(ERROR_EXPR_BAD);
	else if (expr_IsConstant(expr)) {
		sect_OutputConst8((uint8_t) expr->Value.Value);
		expr_Free(expr);
	} else if (expr_IsRelocatable(expr))
		sect_OutputReloc8(expr);
	else
		prj_Error(ERROR_EXPR_CONST_RELOC);
}

void sect_OutputConst16(uint16_t value) {
	assert(g_pConfiguration->eMinimumWordSize <= MINSIZE_16BIT);

	if (sect_CheckAvailableSpace(2)) {
		switch (sect_GetCurrentType()) {
			case GROUP_TEXT: {
				switch (g_pOptions->Endian) {
					case ASM_LITTLE_ENDIAN: {
						g_pCurrentSection->pData[g_pCurrentSection->UsedSpace++] = (uint8_t) (value);
						g_pCurrentSection->pData[g_pCurrentSection->UsedSpace++] = (uint8_t) (value >> 8u);
						break;
					}
					case ASM_BIG_ENDIAN: {
						g_pCurrentSection->pData[g_pCurrentSection->UsedSpace++] = (uint8_t) (value >> 8u);
						g_pCurrentSection->pData[g_pCurrentSection->UsedSpace++] = (uint8_t) (value);
						break;
					}
					default: {
						internalerror("Unknown endianness");
						break;
					}
				}
				g_pCurrentSection->FreeSpace -= 2;
				g_pCurrentSection->PC += 2 / g_pConfiguration->eMinimumWordSize;
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
				patch_Create(g_pCurrentSection, g_pCurrentSection->UsedSpace, expr,
							 g_pOptions->Endian == ASM_LITTLE_ENDIAN ? PATCH_LWORD : PATCH_BWORD);
				g_pCurrentSection->FreeSpace -= 2;
				g_pCurrentSection->UsedSpace += 2;
				g_pCurrentSection->PC += 2 / g_pConfiguration->eMinimumWordSize;
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

	if (expr == NULL)
		prj_Error(ERROR_EXPR_BAD);
	else if (expr_IsConstant(expr)) {
		sect_OutputConst16((uint16_t) (expr->Value.Value));
		expr_Free(expr);
	} else if (expr_IsRelocatable(expr))
		sect_OutputReloc16(expr);
	else
		prj_Error(ERROR_EXPR_CONST_RELOC);
}

void sect_OutputConst32(uint32_t value) {
	assert(g_pConfiguration->eMinimumWordSize <= MINSIZE_32BIT);

	if (sect_CheckAvailableSpace(4)) {
		switch (sect_GetCurrentType()) {
			case GROUP_TEXT: {
				switch (g_pOptions->Endian) {
					case ASM_LITTLE_ENDIAN: {
						g_pCurrentSection->pData[g_pCurrentSection->UsedSpace++] = (uint8_t) (value);
						g_pCurrentSection->pData[g_pCurrentSection->UsedSpace++] = (uint8_t) (value >> 8u);
						g_pCurrentSection->pData[g_pCurrentSection->UsedSpace++] = (uint8_t) (value >> 16u);
						g_pCurrentSection->pData[g_pCurrentSection->UsedSpace++] = (uint8_t) (value >> 24u);
						break;
					}
					case ASM_BIG_ENDIAN: {
						g_pCurrentSection->pData[g_pCurrentSection->UsedSpace++] = (uint8_t) (value >> 24u);
						g_pCurrentSection->pData[g_pCurrentSection->UsedSpace++] = (uint8_t) (value >> 16u);
						g_pCurrentSection->pData[g_pCurrentSection->UsedSpace++] = (uint8_t) (value >> 8u);
						g_pCurrentSection->pData[g_pCurrentSection->UsedSpace++] = (uint8_t) (value);
						break;
					}
					default: {
						internalerror("Unknown endianness");
						break;
					}
				}
				g_pCurrentSection->FreeSpace -= 4;
				g_pCurrentSection->PC += 4 / g_pConfiguration->eMinimumWordSize;
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
				patch_Create(g_pCurrentSection, g_pCurrentSection->UsedSpace, expr,
							 g_pOptions->Endian == ASM_LITTLE_ENDIAN ? PATCH_LLONG : PATCH_BLONG);
				g_pCurrentSection->FreeSpace -= 4;
				g_pCurrentSection->PC += 4 / g_pConfiguration->eMinimumWordSize;
				g_pCurrentSection->UsedSpace += 4;
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

	if (expr == NULL)
		prj_Error(ERROR_EXPR_BAD);
	else if (expr_IsConstant(expr)) {
		sect_OutputConst32(expr->Value.Value);
		expr_Free(expr);
	} else if (expr_IsRelocatable(expr))
		sect_OutputRel32(expr);
	else
		prj_Error(ERROR_EXPR_CONST_RELOC);
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

					read = fread(&g_pCurrentSection->pData[g_pCurrentSection->UsedSpace], sizeof(uint8_t), size, f);
					g_pCurrentSection->FreeSpace -= size;
					g_pCurrentSection->UsedSpace += size;
					g_pCurrentSection->PC += size / g_pConfiguration->eMinimumWordSize;
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

	uint32_t t = g_pCurrentSection->UsedSpace + align - 1;
	t -= t % align;
	sect_SkipBytes(t - g_pCurrentSection->UsedSpace);
}

void sect_SkipBytes(uint32_t count) {
	if (count == 0)
		return;

	assert(g_pConfiguration->eMinimumWordSize <= count);

	if (sect_CheckAvailableSpace(count)) {
		//printf("*DEBUG* skipping %d bytes\n", count);
		switch (sect_GetCurrentType()) {
			case GROUP_TEXT: {
				if (g_pOptions->UninitChar != -1) {
					while (count--)
						sect_OutputConst8((uint8_t) g_pOptions->UninitChar);
					return;
				}
				//	Fall through to GROUP_BSS
			}
			case GROUP_BSS: {
				g_pCurrentSection->FreeSpace -= count;
				g_pCurrentSection->UsedSpace += count;
				g_pCurrentSection->PC += count / g_pConfiguration->eMinimumWordSize;
				break;
			}
			default: {
				internalerror("Unknown GROUP type");
			}
		}
	}
}

bool sect_SwitchTo(char* sectname, SSymbol* group) {
	SSection* sect;

	sect = sect_Find(sectname, group);
	if (sect) {
		g_pCurrentSection = sect;
		return true;
	} else {
		sect = sect_Create(sectname);
		if (sect) {
			sect->pGroup = group;
			sect->Flags = 0;
		}
		g_pCurrentSection = sect;
		return sect != NULL;
	}
}

bool sect_SwitchTo_LOAD(char* sectname, SSymbol* group, uint32_t load) {
	SSection* sect;

	if ((sect = sect_Find(sectname, group)) != NULL) {
		if (sect->Flags == SECTF_LOADFIXED && sect->BasePC == load) {
			g_pCurrentSection = sect;
			return true;
		} else {
			prj_Fail(ERROR_SECT_EXISTS_LOAD);
			return false;
		}
	} else {
		if ((sect = sect_Create(sectname)) != NULL) {
			sect->pGroup = group;
			sect->Flags = SECTF_LOADFIXED;
			sect->BasePC = load;
			sect->Position = load * g_pConfiguration->eMinimumWordSize;
		}
		g_pCurrentSection = sect;
		return sect != NULL;
	}
}

bool sect_SwitchTo_BANK(char* sectname, SSymbol* group, uint32_t bank) {
	SSection* sect;

	if (!g_pConfiguration->bSupportBanks)
		internalerror("Banks not supported");

	sect = sect_Find(sectname, group);
	if (sect) {
		if (sect->Flags == SECTF_BANKFIXED && sect->Bank == bank) {
			g_pCurrentSection = sect;
			return true;
		}

		prj_Fail(ERROR_SECT_EXISTS_BANK);
		return false;
	}

	sect = sect_Create(sectname);
	if (sect) {
		sect->pGroup = group;
		sect->Flags = SECTF_BANKFIXED;
		sect->Bank = bank;
	}
	g_pCurrentSection = sect;
	return sect != NULL;
}

bool sect_SwitchTo_LOAD_BANK(char* sectname, SSymbol* group, uint32_t load, uint32_t bank) {
	SSection* sect;

	if (!g_pConfiguration->bSupportBanks)
		internalerror("Banks not supported");

	if ((sect = sect_Find(sectname, group)) != NULL) {
		if (sect->Flags == (SECTF_BANKFIXED | SECTF_LOADFIXED) && sect->Bank == bank && sect->BasePC == load) {
			g_pCurrentSection = sect;
			return true;
		}

		prj_Fail(ERROR_SECT_EXISTS_BANK_LOAD);
		return false;
	}

	if ((sect = sect_Create(sectname)) != NULL) {
		sect->pGroup = group;
		sect->Flags = SECTF_BANKFIXED | SECTF_LOADFIXED;
		sect->Bank = bank;
		sect->BasePC = load;
		sect->Position = load * g_pConfiguration->eMinimumWordSize;
	}

	g_pCurrentSection = sect;
	return sect != NULL;
}

bool sect_SwitchTo_NAMEONLY(char* sectname) {
	if ((g_pCurrentSection = sect_Find(sectname, NULL)) != NULL) {
		return true;
	} else {
		prj_Fail(ERROR_NO_SECT);
		return false;
	}
}

bool sect_Init(void) {
	g_pCurrentSection = NULL;
	g_pSectionList = NULL;
	return true;
}

void sect_SetOrgAddress(uint32_t org) {
	if (g_pCurrentSection == NULL) {
		prj_Error(ERROR_SECTION_MISSING);
	} else {
		g_pCurrentSection->Flags |= SECTF_ORGFIXED;
		g_pCurrentSection->OrgOffset = org - (g_pCurrentSection->PC + g_pCurrentSection->BasePC);
	}
}

uint32_t sect_TotalSections(void) {
	uint32_t totalSections = 0;
	for(const SSection* pSect = g_pSectionList; pSect != NULL; pSect = list_GetNext(pSect)) {
		++totalSections;
	}
	return totalSections;
}
