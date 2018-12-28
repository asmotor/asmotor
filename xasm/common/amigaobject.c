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

// From util
#include "asmotor.h"
#include "mem.h"
#include "file.h"
#include "types.h"

// From xasm
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

static void
fputbuf(const void* buffer, size_t length, FILE* fileHandle) {
    fwrite(buffer, 1, length, fileHandle);

    while ((length & 3u) != 0) {
        fputc(0, fileHandle);
        ++length;
    }
}

static void
fputstr(const string* str, FILE* fileHandle, uint32_t flags) {
    uint32_t length = (uint32_t) str_Length(str);
    fputbl(((length + 3) / 4) | flags, fileHandle);
    fputbuf(str_String(str), length, fileHandle);
}

static void
writeSymbolHunk(FILE* fileHandle, const SSection* section) {
    uint32_t symbolCount = 0;
    off_t startPosition = ftello(fileHandle);

    fputbl(HUNK_SYMBOL, fileHandle);

    for (uint_fast16_t i = 0; i < SYMBOL_HASH_SIZE; ++i) {
        for (const SSymbol* symbol = sym_hashedSymbols[i]; symbol != NULL; symbol = list_GetNext(symbol)) {
            if ((symbol->flags & SYMF_RELOC) != 0 && symbol->section == section) {
                fputstr(symbol->name, fileHandle, 0);
                fputbl((uint32_t) symbol->value.integer, fileHandle);
                ++symbolCount;
            }
        }
    }

    if (symbolCount == 0)
        fseeko(fileHandle, startPosition, SEEK_SET);
    else
        fputbl(0, fileHandle);
}

static void
writeExtHunk(FILE* fileHandle, const SSection* section, const SPatch* importPatches, off_t hunkPosition) {
    bool dataWritten = false;
    off_t startPosition = ftello(fileHandle);

    fputbl(HUNK_EXT, fileHandle);

    while (importPatches != NULL) {
        uint32_t offset;
        SSymbol* patchSymbol = NULL;
        if (expr_GetImportOffset(&offset, &patchSymbol, importPatches->expression)) {
            uint32_t patchCount = 0;

            fputstr(patchSymbol->name, fileHandle, EXT_REF32);
            long symbolCountPosition = ftell(fileHandle);
            fputbl(0, fileHandle);

            const SPatch* patch = importPatches;
            do {
                off_t offsetPosition = ftello(fileHandle);

                fseeko(fileHandle, patch->offset + hunkPosition, SEEK_SET);
                fputbl(offset, fileHandle);
                fseeko(fileHandle, offsetPosition, SEEK_SET);

                fputbl(patch->offset, fileHandle);

                ++patchCount;
                dataWritten = true;

                for (patch = list_GetNext(patch); patch != NULL; patch = list_GetNext(patch)) {
                    SSymbol* symbol = NULL;
                    if (expr_GetImportOffset(&offset, &symbol, patch->expression) && symbol == patchSymbol) {
                        assert (patch->pPrev != NULL);

                        patch->pPrev->pNext = patch->pNext;
                        if (patch->pNext)
                            patch->pNext->pPrev = patch->pPrev;

                        break;
                    }
                }
            } while (patch != NULL);

            off_t currentPosition = ftello(fileHandle);
            fseek(fileHandle, symbolCountPosition, SEEK_SET);
            fputbl(patchCount, fileHandle);
            fseek(fileHandle, currentPosition, SEEK_SET);
        }
        importPatches = list_GetNext(importPatches);
    }

    for (uint_fast16_t i = 0; i < SYMBOL_HASH_SIZE; ++i) {
        for (SSymbol* symbol = sym_hashedSymbols[i]; symbol != NULL; symbol = list_GetNext(symbol)) {
            if ((symbol->flags & (SYMF_RELOC | SYMF_EXPORT)) == (SYMF_RELOC | SYMF_EXPORT)
                && symbol->section == section) {
                fputstr(symbol->name, fileHandle, EXT_DEF);
                fputbl((uint32_t) symbol->value.integer, fileHandle);

                dataWritten = true;
            }
        }
    }

    if (!dataWritten)
        fseeko(fileHandle, startPosition, SEEK_SET);
    else
        fputbl(0, fileHandle);
}

static void
writeReloc32(FILE* fileHandle, SPatch** patchesPerSection, uint32_t totalSections, long hunkPosition) {
    fputbl(HUNK_RELOC32, fileHandle);

    SSection* offsetToSection = sect_Sections;
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
                expr_GetSectionOffset(patch->expression, offsetToSection, &value);

                off_t currentPosition = ftello(fileHandle);
                fseek(fileHandle, patch->offset + hunkPosition, SEEK_SET);
                fputbl(value, fileHandle);
                fseeko(fileHandle, currentPosition, SEEK_SET);
                fputbl(patch->offset, fileHandle);
            }
        }

        offsetToSection = list_GetNext(offsetToSection);
    }
    fputbl(0, fileHandle);
}

static bool
writeSection(FILE* fileHandle, SSection* section, bool writeDebugInfo, uint32_t totalSections, bool isLinkObject) {
    if (section->group->value.groupType == GROUP_TEXT) {
        SPatch** patchesPerSection = mem_Alloc(sizeof(SPatch*) * totalSections);
        for (uint32_t i = 0; i < totalSections; ++i)
            patchesPerSection[i] = NULL;

        uint32_t hunkType =
                (g_pConfiguration->bSupportAmiga && (section->group->flags & SYMF_DATA)) ? HUNK_DATA : HUNK_CODE;

        fputbl(hunkType, fileHandle);
        fputbl((section->usedSpace + 3) / 4, fileHandle);
        off_t hunkPosition = ftello(fileHandle);
        fputbuf(section->data, section->usedSpace, fileHandle);

        // Move the patches into the patchesPerSection array according the section to which their value is relative
        SPatch* patch = section->patches;
        SPatch* importPatches = NULL;
        bool hasReloc32 = false;

        while (patch) {
            SPatch* nextPatch = list_GetNext(patch);
            if (patch->type == PATCH_BE_32) {
                bool foundSection = false;
                int sectionIndex = 0;

                for (SSection* originSection = sect_Sections;
                     originSection != NULL; originSection = list_GetNext(originSection)) {
                    if (expr_IsRelativeToSection(patch->expression, originSection)) {
                        if (patch->pPrev)
                            patch->pPrev->pNext = patch->pNext;
                        else
                            section->patches = patch->pNext;
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
                }

                if ((!foundSection) && isLinkObject) {
                    uint32_t offset;
                    SSymbol* symbol = NULL;
                    if (expr_GetImportOffset(&offset, &symbol, patch->expression)) {
                        if (patch->pPrev)
                            patch->pPrev->pNext = patch->pNext;
                        else
                            section->patches = patch->pNext;

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

        if (section->patches != NULL) {
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
        fputbl((section->usedSpace + 3) / 4, fileHandle);

        if (isLinkObject)
            writeExtHunk(fileHandle, section, NULL, 0);
    }

    if (writeDebugInfo)
        writeSymbolHunk(fileHandle, section);

    fputbl(HUNK_END, fileHandle);
    return true;
}

static void
writeSectionNames(FILE* fileHandle, bool writeDebugInfo) {
    if (writeDebugInfo) {
        for (const SSection* section = sect_Sections; section != NULL; section = list_GetNext(section)) {
            fputstr(section->name, fileHandle, 0);
        }
    }

    /* name list terminator */
    fputbl(0, fileHandle);
}

bool
ami_WriteObject(string* destFilename, string* sourceFilename) {
    bool r = true;

    FILE* fileHandle = fopen(str_String(destFilename), "wb");
    if (fileHandle == NULL)
        return false;

    fputbl(HUNK_UNIT, fileHandle);
    fputstr(sourceFilename, fileHandle, 0);

    uint32_t totalSections = sect_TotalSections();

    for (SSection* section = sect_Sections; section != NULL; section = list_GetNext(section)) {
        fputbl(HUNK_NAME, fileHandle);
        fputstr(section->name, fileHandle, 0);
        if (!writeSection(fileHandle, section, true, totalSections, true)) {
            r = false;
            break;
        }
    }

    fclose(fileHandle);
    return r;
}

bool
ami_WriteExecutable(string* destFilename, bool writeDebugInfo) {
    bool r = true;

    FILE* fileHandle = fopen(str_String(destFilename), "wb");
    if (fileHandle == NULL)
        return false;

    fputbl(HUNK_HEADER, fileHandle);
    writeSectionNames(fileHandle, writeDebugInfo);

    uint32_t totalSections = sect_TotalSections();
    fputbl(totalSections, fileHandle);
    fputbl(0, fileHandle);
    fputbl(totalSections - 1, fileHandle);

    for (const SSection* section = sect_Sections; section != NULL; section = list_GetNext(section)) {
        uint32_t size = (section->usedSpace + 3) / 4;
        if (g_pConfiguration->bSupportAmiga && (section->group->flags & SYMF_CHIP))
            size |= HUNKF_CHIP;
        fputbl(size, fileHandle);
    }

    for (SSection* section = sect_Sections; section != NULL; section = list_GetNext(section)) {
        if (!writeSection(fileHandle, section, writeDebugInfo, totalSections, false)) {
            r = false;
            break;
        }
    }

    fclose(fileHandle);
    return r;
}
