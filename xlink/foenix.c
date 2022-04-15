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

#include <assert.h>

#include "file.h"
#include "mem.h"
#include "str.h"

#include "error.h"
#include "section.h"

#define PGZ_32BIT 'z'


static void
writeSection(FILE* fileHandle, int32_t cpuByteLocation, uint32_t size, void* data) {
	fputll(cpuByteLocation, fileHandle);
	fputll(size, fileHandle);
	if (size != 0) {
		assert(data != NULL);
		fwrite(data, 1, size, fileHandle);
	}
}

static void
writeSections(FILE* fileHandle) {
	for (SSection* section = sect_Sections; section != NULL; section = section->nextSection) {
		if (section->data != NULL && section->used) {
			writeSection(fileHandle, section->cpuByteLocation, section->size, section->data);
		}
	}
}

extern void
foenix_WriteExecutable(const char* outputFilename, const char* entry) {
	FILE* fileHandle = fopen(outputFilename, "wb");
	if (fileHandle == NULL) {
		error("Unable to open \"%s\" for writing", outputFilename);
	}

	// "Header"
	fputc(PGZ_32BIT, fileHandle);

	// Start address section
    int startAddress = 0;
    if (entry != NULL) {
        SSymbol* entrySymbol = sect_FindExportedSymbol(entry);
        if (entrySymbol == NULL)
            error("Entry symbol \"%s\" not found (it must be exported)", entry);
        startAddress = entrySymbol->value;
    } else {
        startAddress = sect_StartAddressOfFirstCodeSection();
    }
	writeSection(fileHandle, startAddress, 0, NULL);

	// The rest of the sections
	writeSections(fileHandle);

	fclose(fileHandle);
}

