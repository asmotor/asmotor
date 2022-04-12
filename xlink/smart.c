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

#include "section.h"
#include "xlink.h"

static void
useSectionWithGlobalExport(const char* name);

static void
useSectionWithLocalExport(const char* name, uint32_t fileId);

static void
markReferencedSectionsUsed(SSection* section) {
    section->used = true;
    for (uint32_t i = 0; i < section->totalSymbols; ++i) {
        SSymbol* symbol = &section->symbols[i];
        if (symbol->type == SYM_LOCALIMPORT)
            useSectionWithLocalExport(symbol->name, section->fileId);
        else if (symbol->type == SYM_IMPORT)
            useSectionWithGlobalExport(symbol->name);
    }
}

static void
useSectionWithGlobalExport(const char* name) {
    SSection* section = sect_FindSectionWithExportedSymbol(name);
    if (section != NULL) {
		if (!section->used)
	        markReferencedSectionsUsed(section);
    } else {
        error("Symbol \"%s\" not found (it must be exported)", name);
    }
}

static void
useSectionWithLocalExport(const char* name, uint32_t fileId) {
    SSection* section = sect_FindSectionWithLocallyExportedSymbol(name, fileId);
    if (section != NULL && !section->used) {
        markReferencedSectionsUsed(section);
    }
}


static void
useRootedSections(void) {
	for (SSection* section = sect_Sections; section != NULL; section = section->nextSection) {
		if (section->root) {
			markReferencedSectionsUsed(section);
		}
	}
}

/* Exported functions */

extern void
smart_Process(const char* name) {
    if (name != NULL) {
        useSectionWithGlobalExport(name);
        useRootedSections();
    } else {
        // Link in all sections
        SSection* section = sect_Sections;

        while (section != NULL) {
            section->used = true;
            section = section->nextSection;
        }
    }
}
