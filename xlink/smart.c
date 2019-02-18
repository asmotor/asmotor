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

#include "section.h"
#include "xlink.h"

static void
useSectionWithGlobalExport(const char* name);

static void
useSectionWithLocalExport(const char* name, uint32_t fileId);

static void
markReferencedSectionsUsed(Section* section) {
    section->used = true;
    for (uint32_t i = 0; i < section->totalSymbols; ++i) {
        Symbol* symbol = &section->symbols[i];
        if (symbol->type == SYM_LOCALIMPORT)
            useSectionWithLocalExport(symbol->name, section->fileId);
        else if (symbol->type == SYM_IMPORT)
            useSectionWithGlobalExport(symbol->name);
    }
}

static void
useSectionWithGlobalExport(const char* name) {
    Section* section = sect_FindSectionWithExportedSymbol(name);
    if (section != NULL && !section->used) {
        markReferencedSectionsUsed(section);
    }
}

static void
useSectionWithLocalExport(const char* name, uint32_t fileId) {
    Section* section = sect_FindSectionWithLocallyExportedSymbol(name, fileId);
    if (section != NULL && !section->used) {
        markReferencedSectionsUsed(section);
    }
}

void
smart_Process(const char* name) {
    if (name != NULL) {
        useSectionWithGlobalExport(name);
    } else {
        // Link in all sections
        Section* section = sect_Sections;

        while (section != NULL) {
            section->used = true;
            section = section->nextSection;
        }
    }
}
