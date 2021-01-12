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

#include <string.h>

#include "mem.h"

#include "section.h"
#include "symbol.h"
#include "types.h"

#include "xlink.h"

SSection* sect_Sections = NULL;

static uint32_t g_sectionId = 0;

static void
resolveSymbol(SSection* section, SSymbol* symbol, bool allowImports) {
    switch (symbol->type) {
        case SYM_LOCALEXPORT:
        case SYM_EXPORT:
        case SYM_LOCAL: {
            symbol->resolved = true;
            symbol->section = section;

            if (section->cpuLocation != -1)
                symbol->value += section->cpuLocation;

            break;
        }

        case SYM_IMPORT: {
            SSection* definingSection;

            for (definingSection = sect_Sections;
                 definingSection != NULL; definingSection = definingSection->nextSection) {
                if (definingSection->used || definingSection->group == NULL) {
                    uint32_t i;
                    SSymbol* exportedSymbol = definingSection->symbols;

                    for (i = 0; i < definingSection->totalSymbols; ++i, ++exportedSymbol) {
                        if (exportedSymbol->type == SYM_EXPORT && strcmp(exportedSymbol->name, symbol->name) == 0) {
                            if (!exportedSymbol->resolved)
                                resolveSymbol(definingSection, exportedSymbol, allowImports);

                            symbol->resolved = true;
                            symbol->value = exportedSymbol->value;
                            symbol->section = definingSection;

                            return;
                        }
                    }
                }
            }

            if (!allowImports)
                error("Unresolved symbol \"%s\"", symbol->name);

            break;
        }

        case SYM_LOCALIMPORT: {
            for (SSection* definingSection = sect_Sections; definingSection != NULL; definingSection = definingSection->nextSection) {
                if (definingSection->used && definingSection->fileId == section->fileId) {
                    SSymbol* exportedSymbol = definingSection->symbols;

                    for (uint32_t i = 0; i < definingSection->totalSymbols; ++i, ++exportedSymbol) {
                        if ((exportedSymbol->type == SYM_LOCALEXPORT || exportedSymbol->type == SYM_EXPORT)
                            && strcmp(exportedSymbol->name, symbol->name) == 0) {
                            if (!exportedSymbol->resolved)
                                resolveSymbol(definingSection, exportedSymbol, allowImports);

                            symbol->resolved = true;
                            symbol->value = exportedSymbol->value;
                            symbol->section = definingSection;

                            return;
                        }
                    }
                }
            }

            error("Unresolved symbol \"%s\"", symbol->name);
        }

        default: {
            error("Unhandled symbol type");
        }
    }
}

static void
resolveUnresolvedSymbols(SSection* section, intptr_t data) {
    for (uint32_t i = 0; i < section->totalSymbols; ++i) {
        SSymbol* symbol = &section->symbols[i];
        if (!symbol->resolved)
            resolveSymbol(section, symbol, true);
    }
}


static void
fillSectionArray(SSection** sectionArray) {
    for (SSection* section = sect_Sections; section != NULL; section = section->nextSection) {
        *sectionArray++ = section;
    }
}

static void
fillSectionList(SSection** sectionArray) {
    sect_Sections = sectionArray[0];

    for (uint32_t i = 1; i < sect_TotalSections(); ++i) {
        sectionArray[i - 1]->nextSection = sectionArray[i];
    }

    sectionArray[sect_TotalSections() - 1]->nextSection = NULL;
}

static int
compareSections(const void* element1, const void* element2) {
    SSection* section1 = *(SSection**) element1;
    SSection* section2 = *(SSection**) element2;

    if (section1->used != section2->used)
        return section1->used - section2->used;

    if (section1->cpuBank != section2->cpuBank)
        return section1->cpuBank - section2->cpuBank;

    return section1->cpuLocation - section2->cpuLocation;
}


static SSymbol*
sectionHasSymbol(SSection* section, const char* symbolName, ESymbolType symbolType) {
    for (uint32_t i = 0; i < section->totalSymbols; ++i) {
        SSymbol* symbol = &section->symbols[i];
        if (symbol->type == symbolType && strcmp(symbol->name, symbolName) == 0)
            return symbol;
    }

    return NULL;
}

static SSection*
findSectionContainingAddress(int32_t value, uint32_t fileId) {
    for (SSection* section = sect_Sections; section != NULL; section = section->nextSection) {
        if (section->fileId == fileId && section->cpuLocation <= value && value < section->cpuLocation + (int32_t) section->size) {
            return section;
        }
    }
    return NULL;
}


/* Exported functions */

extern SSymbol*
sect_GetSymbol(SSection* section, uint32_t symbolId, bool allowImports) {
    if (symbolId >= section->totalSymbols) {
        error("Symbol ID out of range");
    } else {
        SSymbol* symbol = &section->symbols[symbolId];

        if (!symbol->resolved)
            resolveSymbol(section, symbol, allowImports);

        return symbol;
    }
}

extern char*
sect_GetSymbolName(SSection* section, uint32_t symbolId) {
    SSymbol* symbol = &section->symbols[symbolId];

    return symbol->name;
}

extern bool
sect_GetConstantSymbolBank(SSection* section, uint32_t symbolId, int32_t* outValue) {
    SSymbol* symbol = &section->symbols[symbolId];

    if (!symbol->resolved)
        resolveSymbol(section, symbol, false);

    int32_t bank = symbol->section->cpuBank;
    if (bank != -1) {
        *outValue = bank;
        return true;
    }

    return false;
}

extern void
sect_ForEachUsedSection(void (* function)(SSection*, intptr_t), intptr_t data) {
    SSection* section;

    for (section = sect_Sections; section != NULL; section = section->nextSection) {
        if (section->used)
            function(section, data);
    }

}

extern void
sect_ResolveUnresolved(void) {
    sect_ForEachUsedSection(resolveUnresolvedSymbols, 0);
}

extern SSection*
sect_CreateNew(void) {
    SSection** section = &sect_Sections;

    while (*section != NULL)
        section = &(*section)->nextSection;

    *section = (SSection*) mem_Alloc(sizeof(SSection));
    if (*section == NULL)
        error("Out of memory");

    (*section)->sectionId = g_sectionId++;
    (*section)->nextSection = NULL;
    (*section)->used = false;
    (*section)->assigned = false;
    (*section)->patches = NULL;

    return *section;
}

extern uint32_t
sect_TotalSections(void) {
    return g_sectionId;
}

extern void
sect_SortSections(void) {
    SSection** sections = mem_Alloc(sizeof(SSection*) * sect_TotalSections());

    fillSectionArray(sections);
    qsort(sections, sect_TotalSections(), sizeof(SSection*), compareSections);
    fillSectionList(sections);

    mem_Free(sections);
}

extern bool
sect_IsEquSection(SSection* section) {
    return section->group == NULL;
}

extern SSection*
sect_FindSectionWithExportedSymbol(const char* symbolName) {
    for (SSection* section = sect_Sections; section != NULL; section = section->nextSection) {
        SSymbol* symbol = sectionHasSymbol(section, symbolName, SYM_EXPORT);
        if (symbol != NULL) {
            if (sect_IsEquSection(section)) {
                return findSectionContainingAddress(symbol->value, section->fileId);
            }
            return section;
        }
    }
    return NULL;
}

extern SSection*
sect_FindSectionWithLocallyExportedSymbol(const char* symbolName, uint32_t fileId) {
    for (SSection* section = sect_Sections; section != NULL; section = section->nextSection) {
        if (section->fileId == fileId) {
            SSymbol* symbol = sectionHasSymbol(section, symbolName, SYM_LOCALEXPORT);
            if (symbol != NULL) {
                if (sect_IsEquSection(section)) {
                    return findSectionContainingAddress(symbol->value, section->fileId);
                }
                return section;
            }
        }
    }
    return sect_FindSectionWithExportedSymbol(symbolName);
}
