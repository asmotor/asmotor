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

#include "file.h"

#include "str.h"

#include "mapfile.h"
#include "object.h"
#include "section.h"
#include "xlink.h"

static uint32_t max_symbol_length = 0;

static int32_t max_symbol_value = 0;

static uint32_t symbol_value_field_size;

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
			if (symbol->fileInfoIndex != UINT32_MAX) {
				fprintf(fileHandle, "$%0*X | %-*s | %s:%d\n", symbol_value_field_size, symbol->value, max_symbol_length,
				        symbol->name, str_String(obj_GetFilename(symbol->fileInfoIndex)), symbol->lineNumber);
			} else {
				fprintf(fileHandle, "$%0*X | %-*s |\n", symbol_value_field_size, symbol->value, max_symbol_length, symbol->name);
			}
		}
	}
}

static void
findLongestSymbolName(SSection* section, intptr_t data) {
	for (uint32_t i = 0; i < section->totalSymbols; ++i) {
		SSymbol* symbol = &section->symbols[i];
		if (!sym_IsImport(symbol) && symbol->resolved) {
			uint32_t symlength = strlen(symbol->name);

			if (symlength > max_symbol_length)
				max_symbol_length = symlength;

			if (symbol->value > max_symbol_value)
				max_symbol_value = symbol->value;
		}
	}
}

static void
writeMapFile(FILE* fileHandle) {
	sect_ForEachUsedSection(findLongestSymbolName, (intptr_t) 0);
	symbol_value_field_size = map_GetValueFieldSize(max_symbol_value, 4);
	sect_ForEachUsedSection(writeSectionToMapFile, (intptr_t) fileHandle);
}

void
map_Write(const char* name) {
	FILE* fileHandle = fopen(name, "wt");
	if (fileHandle == NULL) {
		error("Unable to open file \"%s\" for writing", name);
	}
	writeMapFile(fileHandle);
	fclose(fileHandle);
}

extern uint32_t
map_GetValueFieldSize(uint32_t value, uint32_t minimum_size) {
	value >>= minimum_size * 4;
	while (value != 0) {
		value >>= 4;
		++minimum_size;
	}

	return minimum_size;
}
