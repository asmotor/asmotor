/*  Copyright 2008-2022 Carsten Elton Sorensen and contributors

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
#include "file.h"

// From xasm
#include "xasm.h"
#include "section.h"
#include "errors.h"
#include "symbol.h"
#include "patch.h"


static SPatch*
needsOrg() {
    for (const SSection* section = sect_Sections; section != NULL; section = list_GetNext(section)) {
        if (section->patches != NULL)
            return section->patches;
    }
    return NULL;
}


static bool
commonPatch() {
    SPatch* firstPatch;

    if (sect_Sections == NULL)
        return false;

    // Check first section
    if ((firstPatch = needsOrg()) != NULL && (sect_Sections->flags & (SECTF_LOADFIXED | SECTF_BANKFIXED)) == 0) {
        err_PatchError(firstPatch, ERROR_SECTION_MUST_LOAD);
        return false;
    }

    uint32_t imagePos = 0;
    SSection* section = sect_Sections;
    do {
        if (section != NULL) {
			xasm_Configuration->assignSection(section);
            if (section->flags & SECTF_LOADFIXED) {
                if (section->imagePosition < imagePos) {
                    err_Error(ERROR_SECTION_LOAD, str_String(section->name), section->cpuOrigin);
                    return false;
                }
                imagePos = section->imagePosition + section->usedSpace;
            } else {
                uint32_t alignment = (section->flags & SECTF_ALIGNED) ? section->align : (uint32_t) opt_Current->sectionAlignment;
                imagePos += alignment - 1;
				imagePos -= imagePos % alignment;

                section->flags |= SECTF_LOADFIXED;
                section->imagePosition = imagePos;
                section->cpuOrigin = imagePos / xasm_Configuration->minimumWordSize;

                imagePos += section->usedSpace;
            }
        }
        section = list_GetNext(section);
    } while (section != NULL);

    for (uint_fast16_t i = 0; i < SYMBOL_HASH_SIZE; ++i) {
        for (SSymbol* symbol = sym_hashedSymbols[i]; symbol != NULL; symbol = list_GetNext(symbol)) {
			if (symbol->flags & SYMF_USED) {
				if (symbol->type == SYM_IMPORT || symbol->type == SYM_GLOBAL) {
					err_Fail(ERROR_SYMBOL_UNDEFINED, str_String(symbol->name));
				} else if (symbol->flags & SYMF_RELOC) {
					symbol->flags &= ~SYMF_RELOC;
					symbol->flags |= SYMF_CONSTANT;
					symbol->value.integer += symbol->section->cpuOrigin;
				}
			}
        }
    }

    patch_BackPatch();
    return true;
}


static bool
internalWrite(string* filename, const char* opentype, void (*writeSection)(FILE*, SSection*, uint32_t)) {
    if (!commonPatch())
        return false;

    FILE* fileHandle;
    if ((fileHandle = fopen(str_String(filename), opentype)) != NULL) {
        uint32_t lastWrittenPosition = sect_Sections->imagePosition;

        for (SSection* section = sect_Sections; section != NULL; section = list_GetNext(section)) {
            if (section->data) {
                writeSection(fileHandle, section, lastWrittenPosition);
                lastWrittenPosition += section->usedSpace;
            }
        }

        fclose(fileHandle);
        return true;
    }

    return false;
}


static void
writeBinarySection(FILE* fileHandle, SSection* section, uint32_t lastWrittenPosition) {
    while (lastWrittenPosition < section->imagePosition) {
        ++lastWrittenPosition;
        fputc(0, fileHandle);
    }

    fwrite(section->data, 1, section->usedSpace, fileHandle);
}


static void
writeVerilogSection(FILE* fileHandle, SSection* section, uint32_t lastWrittenPosition) {
    while (lastWrittenPosition < section->imagePosition) {
        ++lastWrittenPosition;
        fprintf(fileHandle, "00\n");
    }

    for (uint32_t i = 0; i < section->usedSpace; ++i) {
        uint8_t b = (uint8_t) (section->data ? section->data[i] : 0u);
        fprintf(fileHandle, "%02X\n", b);
    }
}


bool
bin_Write(string* filename) {
    return internalWrite(filename, "wb", writeBinarySection);
}


bool
bin_WriteVerilog(string* filename) {
    return internalWrite(filename, "wt", writeVerilogSection);
}
