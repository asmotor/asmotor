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

#include "file.h"

#include "section.h"
#include "xlink.h"

static int
compareSymbols(const void* element1, const void* element2) {
    const SSymbol* symbol1 = (const SSymbol*) element1;
    const SSymbol* symbol2 = (const SSymbol*) element2;

    bool symbol1Import = sym_IsImport(symbol1);
    bool symbol2Import = sym_IsImport(symbol2);

    if (symbol1Import != symbol2Import)
        return symbol1Import - symbol2Import;

    return symbol1->value - symbol2->value;
}

static void
sortSymbols(SSymbol* symbolArray, uint32_t totalSymbols) {
    qsort(symbolArray, totalSymbols, sizeof(SSymbol), compareSymbols);
}

static void
writeSectionToMapFile(SSection* section, intptr_t data) {
    FILE* fileHandle = (FILE*) data;

    sortSymbols(section->symbols, section->totalSymbols);

    for (uint32_t i = 0; i < section->totalSymbols; ++i) {
        SSymbol* symbol = &section->symbols[i];

        if (!sym_IsImport(symbol) && symbol->resolved) {
            if (section->cpuBank != -1) {
                fprintf(fileHandle, "%X:", section->cpuBank);
            }

            fprintf(fileHandle, "%X %s\n", symbol->value, symbol->name);
        }
    }
}

static void
writeMapFile(FILE* fileHandle) {
    sect_ForEachUsedSection(writeSectionToMapFile, (intptr_t) fileHandle);
}

void
map_Write(const char* name) {
    FILE* fileHandle = fopen(name, "wt");
    if (fileHandle == NULL) {
        error("Unable to open file \"%s\" for writing", name);
    } 
    writeMapFile(fileHandle);
}
