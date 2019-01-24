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

#include "xlink.h"

Section* sect_Sections = NULL;

static uint32_t s_sectionId = 0;

Section*
sect_CreateNew(void) {
    Section** section = &sect_Sections;

    while (*section != NULL)
        section = &(*section)->nextSection;

    *section = (Section*) mem_Alloc(sizeof(Section));
    if (*section == NULL)
        error("Out of memory");

    (*section)->sectionId = s_sectionId++;
    (*section)->nextSection = NULL;
    (*section)->used = false;
    (*section)->assigned = false;
    (*section)->patches = NULL;

    return *section;
}

uint32_t
sect_TotalSections(void) {
    return s_sectionId;
}

static void
resolveSymbol(Section* section, Symbol* symbol, bool allowImports) {
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
            Section* definingSection;

            for (definingSection = sect_Sections;
                 definingSection != NULL; definingSection = definingSection->nextSection) {
                if (definingSection->used) {
                    uint32_t i;
                    Symbol* exportedSymbol = definingSection->symbols;

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
            Section* definingSection;

            for (definingSection = sect_Sections;
                 definingSection != NULL; definingSection = definingSection->nextSection) {
                if (definingSection->used && definingSection->fileId == section->fileId) {
                    uint32_t i;
                    Symbol* exportedSymbol = definingSection->symbols;

                    for (i = 0; i < definingSection->totalSymbols; ++i, ++exportedSymbol) {
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

Symbol*
sect_GetSymbol(Section* section, uint32_t symbolId, bool allowImports) {
    if (symbolId >= section->totalSymbols) {
        error("Symbol ID out of range");
    } else {
        Symbol* symbol = &section->symbols[symbolId];

        if (!symbol->resolved)
            resolveSymbol(section, symbol, allowImports);

        return symbol;
    }
}

char*
sect_GetSymbolName(Section* section, uint32_t symbolId) {
    Symbol* symbol = &section->symbols[symbolId];

    return symbol->name;
}

bool
sect_GetConstantSymbolBank(Section* section, uint32_t symbolId, int32_t* outValue) {
    Symbol* symbol = &section->symbols[symbolId];

    if (!symbol->resolved)
        resolveSymbol(section, symbol, false);

    int32_t bank = symbol->section->cpuBank;
    if (bank != -1) {
        *outValue = bank;
        return true;
    }

    return false;
}

void
sect_ForEachUsedSection(void (* function)(Section*)) {
    Section* section;

    for (section = sect_Sections; section != NULL; section = section->nextSection) {
        if (section->used)
            function(section);
    }

}
