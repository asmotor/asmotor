/*  Copyright 2008-2026 Carsten Elton Sorensen and contributors

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
#include <string.h>

#include "file.h"

#include "group.h"
#include "image.h"
#include "section.h"
#include "xlink.h"

#include "coco.h"

// .BIN file format
//
// Section:
//   uint8_t type
//   uint16_t size
//   uint16_t address
//   if type == 0
//     uint8_t data[size]
//
// File contains zero or more sections of type = 0x00
// File ends with section of type = 0xFF, size = 0 and start address = address. Rest of file ignored.

#define HEADER_SIZE 5

static void
writeSection(FILE* fileHandle, uint16_t cpuByteLocation, uint16_t size, void* data) {
	fputc(0, fileHandle);
	fputbw(size, fileHandle);
	fputbw(cpuByteLocation, fileHandle);
	if (size != 0) {
		assert(data != NULL);
		fwrite(data, 1, size, fileHandle);
	}
}

static void
writeEndSection(FILE* fileHandle, uint16_t startAddress) {
	fputc(0xFFu, fileHandle);
	fputbw(0, fileHandle);
	fputbw(startAddress, fileHandle);
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
coco_WriteQuickloadBin(const char* outputFilename, const char* entry) {
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

	if (startAddress >= 0x8000) {
		error("Invalid start address %04X", startAddress);
	}

	FILE* fileHandle = fopen(outputFilename, "w+b");
	if (fileHandle == NULL) {
		error("Unable to open \"%s\" for writing", outputFilename);
	}

	// The rest of the sections
	writeSections(fileHandle);

	writeEndSection(fileHandle, startAddress);

	fclose(fileHandle);
}
