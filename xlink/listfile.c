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

#include "listfile.h"
#include "error.h"
#include "mapfile.h"
#include "object.h"
#include "section.h"

static uint32_t longest_address = 0;

static uint32_t address_field_size = 0;

static int
compareLineMappings(const void* v1, const void* v2) {
	SLineMapping* line1 = (SLineMapping*) v1;
	SLineMapping* line2 = (SLineMapping*) v2;

	return line1->offset - line2->offset;
}

static void
sortLineMappings(SSection* section) {
	qsort(section->lineMappings, section->totalLineMappings, sizeof(SLineMapping), compareLineMappings);
}

static void
writeSection(SSection* section, intptr_t data) {
	FILE* fileHandle = (FILE*) data;

	sortLineMappings(section);

	uint32_t offset = 0;
	for (uint32_t i = 0; i < section->totalLineMappings; ++i) {
		SLineMapping* line = &section->lineMappings[i];
		uint32_t nextOffset = i < section->totalLineMappings - 1 ? section->lineMappings[i + 1].offset : section->size;
		bool more = false;

		uint32_t dataSize = section->data != NULL ? nextOffset - line->offset : 0;
		if (dataSize > 4) {
			dataSize = 4;
			more = true;
		}

		fprintf(fileHandle, " %0*X ", address_field_size, section->cpuByteLocation + line->offset);

		if (section->data != NULL) {
			for (uint32_t j = 0; j < dataSize; ++j) {
				fprintf(fileHandle, " %02X", section->data[offset + j]);
			}
		}

		int writtenChars = dataSize * 3;

		if (more) {
			fprintf(fileHandle, " ...");
			writtenChars += 4;
		}

		fprintf(fileHandle, "%*s", (17 - writtenChars), "");

		fprintf(fileHandle, " | %s:%d\n", str_String(obj_GetFilename(line->fileInfoIndex)), line->lineNumber);

		offset = nextOffset;
	}
}

static void
findLongestAddress(SSection* section, intptr_t data) {
	for (uint32_t i = 0; i < section->totalLineMappings; ++i) {
		SLineMapping* line = &section->lineMappings[i];
		uint32_t offset = section->cpuByteLocation + line->offset;

		if (offset > longest_address)
			longest_address = offset;
	}
}

static void
writeListFile(FILE* fileHandle) {
	sect_ForEachUsedSection(findLongestAddress, 0);
	address_field_size = map_GetValueFieldSize(longest_address, 4);
	sect_ForEachUsedSection(writeSection, (intptr_t) fileHandle);
}

void
list_Write(const char* name) {
	FILE* fileHandle = fopen(name, "wt");
	if (fileHandle == NULL) {
		error("Unable to open file \"%s\" for writing", name);
	}
	writeListFile(fileHandle);
	fclose(fileHandle);
}
