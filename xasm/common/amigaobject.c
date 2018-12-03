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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asmotor.h"
#include "mem.h"
#include "file.h"

#include "types.h"

#include "section.h"
#include "symbol.h"
#include "patch.h"
#include "project.h"
#include "amigaobject.h"

#define HUNK_UNIT    0x3E7u
#define HUNK_NAME    0x3E8u
#define HUNK_CODE    0x3E9u
#define HUNK_DATA    0x3EAu
#define HUNK_BSS     0x3EBu
#define HUNK_RELOC32 0x3ECu
#define HUNK_EXT     0x3EFu
#define HUNK_SYMBOL  0x3F0u
#define HUNK_END     0x3F2u
#define HUNK_HEADER  0x3F3u
#define HUNKF_CHIP   (1u << 30u)

#define EXT_DEF      0x01000000u
#define EXT_REF32    0x81000000u

static void fputbuf(const void* buffer, size_t length, FILE* fileHandle) {
	fwrite(buffer, 1, length, fileHandle);

	while ((length & 3u) != 0) {
		fputc(0, fileHandle);
		++length;
	}
}

static void fputstr(const string* str, FILE* fileHandle, uint32_t flags) {
	uint32_t length = (uint32_t) str_Length(str);
	fputbl(((length + 3) / 4) | flags, fileHandle);
	fputbuf(str_String(str), length, fileHandle);
}

static void writeSymbolHunk(FILE* fileHandle, const SSection* section, bool_t bSkipExt) {
	int count = 0;
	long startPosition = ftell(fileHandle);

	fputbl(HUNK_SYMBOL, fileHandle);

	for (uint_fast16_t i = 0; i < HASHSIZE; ++i) {
		for (SSymbol* symbol = g_pHashedSymbols[i]; symbol != NULL; symbol = list_GetNext(symbol)) {
			if ((symbol->nFlags & SYMF_RELOC) != 0 && symbol->pSection == section) {
				if (!((symbol->nFlags & SYMF_EXPORT) && bSkipExt)) {
					fputstr(symbol->pName, fileHandle, 0);
					fputbl((uint32_t) symbol->Value.Value, fileHandle);
					++count;
				}
			}
		}
	}

	if (count == 0)
		fseek(fileHandle, startPosition, SEEK_SET);
	else
		fputbl(0, fileHandle);
}

static void writeExtHunk(FILE* fileHandle, const SSection* section, const SPatch* importPatches, long hunkPosition) {
	int count = 0;
	long fpos = ftell(fileHandle);
	fputbl(HUNK_EXT, fileHandle);

	while (importPatches != NULL) {
		uint32_t offset;
		SSymbol* pSym = NULL;
		if (patch_GetImportOffset(&offset, &pSym, importPatches->pExpression)) {
			uint32_t symbolCount = 0;

			fputstr(pSym->pName, fileHandle, EXT_REF32);
			long symbolCountPosition = ftell(fileHandle);
			fputbl(0, fileHandle);

			const SPatch* patch = importPatches;
			do {
				long pos = ftell(fileHandle);
				fseek(fileHandle, patch->Offset + hunkPosition, SEEK_SET);
				fputbl(offset, fileHandle);
				fseek(fileHandle, pos, SEEK_SET);
				fputbl(patch->Offset, fileHandle);
				++count;
				++symbolCount;

				patch = list_GetNext(patch);
				while (patch != NULL) {
					SSymbol* sym = NULL;
					if (patch_GetImportOffset(&offset, &sym, patch->pExpression) && sym == pSym) {
						assert (patch->pPrev != NULL);
						patch->pPrev->pNext = patch->pNext;
						if (patch->pNext)
							patch->pNext->pPrev = patch->pPrev;

						break;
					}
					patch = list_GetNext(patch);
				}
			} while (patch != NULL);

			long currentPosition = ftell(fileHandle);
			fseek(fileHandle, symbolCountPosition, SEEK_SET);
			fputbl(symbolCount, fileHandle);
			fseek(fileHandle, currentPosition, SEEK_SET);
		}
		importPatches = list_GetNext(importPatches);
	}

	for (uint_fast16_t i = 0; i < HASHSIZE; ++i) {
		for (SSymbol* sym = g_pHashedSymbols[i]; sym != NULL; sym = list_GetNext(sym)) {
			if ((sym->nFlags & (SYMF_RELOC | SYMF_EXPORT)) == (SYMF_RELOC | SYMF_EXPORT) && sym->pSection == section) {
				fputstr(sym->pName, fileHandle, EXT_DEF);
				fputbl((uint32_t) sym->Value.Value, fileHandle);
				++count;
			}
		}
	}

	if (count == 0)
		fseek(fileHandle, fpos, SEEK_SET);
	else
		fputbl(0, fileHandle);
}

static void writeReloc32(FILE* fileHandle, SPatch** patchesPerSection, uint32_t totalSections, long hunkPosition) {
	fputbl(HUNK_RELOC32, fileHandle);

	SSection* offsetToSection = g_pSectionList;
	for (uint32_t i = 0; i < totalSections; ++i) {
		uint32_t totalRelocations = 0;
		for (SPatch* patch = patchesPerSection[i]; patch != NULL; patch = list_GetNext(patch)) {
			++totalRelocations;
		}
		if (totalRelocations > 0) {
			fputbl(totalRelocations, fileHandle);
			fputbl(i, fileHandle);
			for (SPatch* patch = patchesPerSection[i]; patch != NULL; patch = list_GetNext(patch)) {
				uint32_t value;
				patch_GetSectionPcOffset(&value, patch->pExpression, offsetToSection);

				long currentPosition = ftell(fileHandle);
				fseek(fileHandle, patch->Offset + hunkPosition, SEEK_SET);
				fputbl(value, fileHandle);
				fseek(fileHandle, currentPosition, SEEK_SET);
				fputbl(patch->Offset, fileHandle);
			}
		}

		offsetToSection = list_GetNext(offsetToSection);
	}
	fputbl(0, fileHandle);
}

static bool_t writeSection(FILE* fileHandle, SSection* section, bool_t writeDebugInfo, uint32_t totalSections, bool_t isLinkObject) {
	if (section->pGroup->Value.GroupType == GROUP_TEXT) {
		SPatch** patchesPerSection = mem_Alloc(sizeof(SPatch*) * totalSections);
		for (uint32_t i = 0; i < totalSections; ++i)
			patchesPerSection[i] = NULL;

		uint32_t hunkType =
			(g_pConfiguration->bSupportAmiga && (section->pGroup->nFlags & SYMF_DATA)) ? HUNK_DATA : HUNK_CODE;

		fputbl(hunkType, fileHandle);
		fputbl((section->UsedSpace + 3) / 4, fileHandle);
		long hunkPosition = ftell(fileHandle);
		fputbuf(section->pData, section->UsedSpace, fileHandle);

		// Move the patches into the patchesPerSection array according the section to which their value is relative
		SPatch* patch = section->pPatches;
		SPatch* importPatches = NULL;
		bool_t hasReloc32 = false;

		while (patch) {
			SPatch* nextPatch = list_GetNext(patch);
			if (patch->Type == PATCH_BLONG) {
				bool_t foundSection = false;
				int sectionIndex = 0;
				SSection* originSection = g_pSectionList;
				while (originSection != NULL) {
					if (patch_IsRelativeToSection(patch->pExpression, originSection)) {
						if (patch->pPrev)
							patch->pPrev->pNext = patch->pNext;
						else
							section->pPatches = patch->pNext;
						if (patch->pNext)
							patch->pNext->pPrev = patch->pPrev;

						patch->pPrev = NULL;
						patch->pNext = patchesPerSection[sectionIndex];
						if (patchesPerSection[sectionIndex])
							patchesPerSection[sectionIndex]->pPrev = patch;
						patchesPerSection[sectionIndex] = patch;
						hasReloc32 = true;
						foundSection = true;
						break;
					}
					++sectionIndex;
					originSection = list_GetNext(originSection);
				}

				if ((!foundSection) && isLinkObject) {
					uint32_t offset;
					SSymbol* pSym = NULL;
					if (patch_GetImportOffset(&offset, &pSym, patch->pExpression)) {
						if (patch->pPrev)
							patch->pPrev->pNext = patch->pNext;
						else
							section->pPatches = patch->pNext;

						if (patch->pNext)
							patch->pNext->pPrev = patch->pPrev;

						patch->pPrev = NULL;
						patch->pNext = importPatches;
						if (importPatches)
							importPatches->pPrev = patch;
						importPatches = patch;
					}
				}
			}
			patch = nextPatch;
		}
		if (section->pPatches != NULL) {
			prj_Error(ERROR_OBJECTFILE_PATCH);
			return false;
		}

		if (hasReloc32)
			writeReloc32(fileHandle, patchesPerSection, totalSections, hunkPosition);

		if (isLinkObject)
			writeExtHunk(fileHandle, section, importPatches, hunkPosition);

		mem_Free(patchesPerSection);
	} else {
		uint32_t hunkType = HUNK_BSS;
		fputbl(hunkType, fileHandle);
		fputbl((section->UsedSpace + 3) / 4, fileHandle);

		if (isLinkObject)
			writeExtHunk(fileHandle, section, NULL, 0);
	}

	if (writeDebugInfo)
		writeSymbolHunk(fileHandle, section, /*bLink*/ false);

	fputbl(HUNK_END, fileHandle);
	return true;
}

static void writeSectionNames(FILE* fileHandle, bool_t writeDebugInfo) {
	if (writeDebugInfo) {
		for (const SSection* pSect = g_pSectionList; pSect != NULL; pSect = list_GetNext(pSect)) {
			fputstr(pSect->Name, fileHandle, 0);
		}
	}

	/* name list terminator */
	fputbl(0, fileHandle);
}

bool_t ami_WriteObject(string* destFilename, string* sourceFilename) {
	bool_t r = true;

	FILE* fileHandle = fopen(str_String(destFilename), "wb");
	if (fileHandle == NULL)
		return false;

	fputbl(HUNK_UNIT, fileHandle);
	fputstr(sourceFilename, fileHandle, 0);

	uint32_t totalSections = sect_TotalSections();

	for (SSection* section = g_pSectionList; section != NULL; section = list_GetNext(section)) {
		fputbl(HUNK_NAME, fileHandle);
		fputstr(section->Name, fileHandle, 0);
		if (!writeSection(fileHandle, section, true, totalSections, true)) {
			r = false;
			break;
		}
	}

	fclose(fileHandle);
	return r;
}

bool_t ami_WriteExecutable(string* destFilename, bool_t writeDebugInfo) {
	bool_t r = true;

	FILE* fileHandle = fopen(str_String(destFilename), "wb");
	if (fileHandle == NULL)
		return false;

	fputbl(HUNK_HEADER, fileHandle);
	writeSectionNames(fileHandle, writeDebugInfo);

	uint32_t totalSections = sect_TotalSections();
	fputbl(totalSections, fileHandle);
	fputbl(0, fileHandle);
	fputbl(totalSections - 1, fileHandle);

	for (const SSection* section = g_pSectionList; section != NULL; section = list_GetNext(section)) {
		uint32_t size = (section->UsedSpace + 3) / 4;
		if (g_pConfiguration->bSupportAmiga && (section->pGroup->nFlags & SYMF_CHIP))
			size |= HUNKF_CHIP;
		fputbl(size, fileHandle);
	}

	for (SSection* section = g_pSectionList; section != NULL; section = list_GetNext(section)) {
		if (!writeSection(fileHandle, section, writeDebugInfo, totalSections, false)) {
			r = false;
			break;
		}
	}

	fclose(fileHandle);
	return r;
}
