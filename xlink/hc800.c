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

#include "xlink.h"
#include "hc800.h"
#include "image.h"
#include "section.h"

#define HUNK_MMU 0
#define HUNK_END 1
#define HUNK_DATA 2


#define HC800_HARVARD 0x01
#define HC800_32K_BANKS 0x14

uint8_t
hc800_ConfigSmall[9] = { 0x00,            
	0x81, 0x82, 0x83, 0x84, 0x81, 0x82, 0x83, 0x84 };

uint8_t
hc800_ConfigSmallHarvard[9] = { HC800_HARVARD,
	0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88 };

uint8_t
hc800_ConfigMedium[9] =	{ HC800_32K_BANKS,
	0x81, 0x82, 0x83, 0x84, 0x81, 0x82, 0x83, 0x84 };

uint8_t
hc800_ConfigMediumHarvard[9] = { HC800_HARVARD | HC800_32K_BANKS,
	0x81, 0x82, 0x87, 0x88, 0x83, 0x84, 0x85, 0x86 };

uint8_t
hc800_ConfigLarge[9] = { HC800_32K_BANKS,
	0x81, 0x82, 0x83, 0x84, 0x81, 0x82, 0x83, 0x84 };


static off_t
writeHunkStart(FILE* fileHandle, int hunkType) {
	fputc(hunkType, fileHandle);
	fputlw(0, fileHandle);

	off_t dataPos = ftello(fileHandle);

	return dataPos;
}


static void
writeHunkEnd(FILE* fileHandle, off_t dataPos) {
	off_t length = ftello(fileHandle) - dataPos;
	fseeko(fileHandle, dataPos - 2, SEEK_SET);
	fputlw((uint16_t) length, fileHandle);

	fseek(fileHandle, 0, SEEK_END);
}


static SSection*
writeBank(FILE* fileHandle, SSection* section) {
	off_t hunk = writeHunkStart(fileHandle, HUNK_DATA);

	uint8_t bank = section->cpuBank;
	fputc(bank, fileHandle);

	off_t dataPos = ftello(fileHandle);
	off_t written = 0;
	SSection* r = NULL;

	while (section != NULL) {
		assert (section->cpuBank >= bank);
		if (section->cpuBank == bank) {
			if (section->group->type != GROUP_BSS) {
				// Make sure hunk is as large as needed before adding more data
				fseek(fileHandle, 0, SEEK_END);
				while (written < section->imageLocation) {
					fputc(0, fileHandle);
					++written;
				}

				fseeko(fileHandle, dataPos + section->imageLocation, SEEK_SET);
				fwrite(section->data, 1, section->size, fileHandle);

				off_t sectionEnd = section->imageLocation + section->size;
				if (sectionEnd > written)
					written = sectionEnd;
			}
			section = section->nextSection;
		} else {
			r = section;
			break;
		}
	}

	fseek(fileHandle, 0, SEEK_END);
	writeHunkEnd(fileHandle, hunk);

	return r;
}

static void
writeDataHunks(FILE* fileHandle) {
	for (int bankNumber = 0x81; bankNumber <= 0xFF; ++bankNumber) {
		for (SSection* section = sect_Sections; section != NULL;) {
			if (section->cpuBank == bankNumber && section->data != NULL) {
				section = writeBank(fileHandle, section);
			} else {
				section = section->nextSection;
			}
		}
	}
}


static void
writeMmuHunk(FILE* fileHandle, uint8_t* configuration) {
	off_t pos = writeHunkStart(fileHandle, HUNK_MMU);
	fwrite(configuration, 1, 9, fileHandle);
	writeHunkEnd(fileHandle, pos);
}


static void
writeEndHunk(FILE* fileHandle) {
	off_t pos = writeHunkStart(fileHandle, HUNK_END);
	writeHunkEnd(fileHandle, pos);
}


static void
writeHeader(FILE* fileHandle) {
	fwrite("UC", 1, 2, fileHandle);
}

extern void
hc800_WriteExecutable(const char* outputFilename, uint8_t* configuration) {
	FILE* fileHandle = fopen(outputFilename, "wb");
	if (fileHandle == NULL) {
		error("Unable to open \"%s\" for writing", outputFilename);
	}

	writeHeader(fileHandle);
	writeMmuHunk(fileHandle, configuration);
	writeDataHunks(fileHandle);
	writeEndHunk(fileHandle);

	fclose(fileHandle);
}

static void
updateKernalHeader(FILE* f) {
	// Zero checksum byte
	fseek(f, 7, SEEK_SET);
	fputc(0, f);

	// Calculate checksum

	fseek(f, 0, SEEK_SET);
	int c;
	int8_t checksum = 0;
	while ((c = fgetc(f)) != EOF) {
		checksum += c;
	}
	checksum = 0xA5 - checksum;

	// Update checksum in image
	fseek(f, 7, SEEK_SET);
	fputc(checksum, f);
}

extern void
hc800_WriteKernal(const char* outputFilename) {
	image_WriteBinary(outputFilename, 0);

	FILE* fileHandle = fopen(outputFilename, "w+b");
	if (fileHandle == NULL)
		error("Unable to open \"%s\" for writing", outputFilename);

	updateKernalHeader(fileHandle);

	fclose(fileHandle);
}
