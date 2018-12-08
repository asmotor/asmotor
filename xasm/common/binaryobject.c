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

// From util
#include "types.h"

// From xasm
#include "section.h"
#include "project.h"
#include "symbol.h"
#include "patch.h"

static bool needsOrg() {
	for (const SSection* section = g_pSectionList; section != NULL; section = list_GetNext(section)) {
		if (section->pPatches != NULL)
			return true;
	}
	return false;
}

static bool commonPatch() {
	// Check first section
	if (needsOrg() && (g_pSectionList->Flags & SECTF_LOADFIXED) == 0) {
		prj_Error(ERROR_SECTION_MUST_LOAD);
		return false;
	}

	uint32_t nAddress = g_pSectionList->Position;
	SSection* section = g_pSectionList;
	do {
		uint32_t alignment = g_pConfiguration->nSectionAlignment - 1u;
		nAddress += (section->UsedSpace + alignment) & ~alignment;
		section = list_GetNext(section);
		if (section != NULL) {
			if (section->Flags & SECTF_LOADFIXED) {
				if (section->Position < nAddress) {
					prj_Error(ERROR_SECTION_LOAD, section->Name, section->BasePC);
					return false;
				}
				nAddress = section->Position;
			} else {
				section->Flags |= SECTF_LOADFIXED;
				section->Position = nAddress;
				section->BasePC = nAddress / g_pConfiguration->eMinimumWordSize;
			}
		}
	} while (section != NULL);

	for (uint_fast16_t i = 0; i < HASHSIZE; ++i) {
		for (SSymbol* symbol = g_pHashedSymbols[i]; symbol != NULL; symbol = list_GetNext(symbol)) {
			if (symbol->eType == SYM_IMPORT) {
				prj_Fail(ERROR_SYMBOL_UNDEFINED, str_String(symbol->pName));
			} else if (symbol->nFlags & SYMF_RELOC) {
				symbol->nFlags &= ~SYMF_RELOC;
				symbol->nFlags |= SYMF_CONSTANT;
				symbol->Value.Value += symbol->pSection->BasePC;
			}
		}
	}

	patch_BackPatch();
	return true;
}

bool bin_Write(string* filename) {
	if (!commonPatch())
		return false;

	FILE* fileHandle;
	if ((fileHandle = fopen(str_String(filename), "wb")) != NULL) {
		uint32_t position = g_pSectionList->Position;

		for (SSection* section = g_pSectionList; section != NULL; section = list_GetNext(section)) {
			if (section->pData) {
				while (position < section->Position) {
					++position;
					fputc(0, fileHandle);
				}

				fwrite(section->pData, 1, section->UsedSpace, fileHandle);
			}

			position += section->UsedSpace;
		}

		fclose(fileHandle);
		return true;
	}

	return false;
}

bool bin_WriteVerilog(string* filename) {
	if (!commonPatch())
		return false;

	FILE* fileHandle;
	if ((fileHandle = fopen(str_String(filename), "wt")) != NULL) {
		uint32_t position = g_pSectionList->Position;

		for (SSection* section = g_pSectionList; section != NULL; section = list_GetNext(section)) {
			while (position < section->Position) {
				++position;
				fprintf(fileHandle, "00\n");
			}

			for (uint32_t i = 0; i < section->UsedSpace; ++i) {
				uint8_t b = (uint8_t) (section->pData ? section->pData[i] : 0u);
				fprintf(fileHandle, "%02X\n", b);
			}
			position += section->UsedSpace;
		}

		fclose(fileHandle);
		return true;
	}

	return false;
}
