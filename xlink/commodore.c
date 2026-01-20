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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "group.h"
#include "image.h"
#include "section.h"
#include "symbol.h"
#include "xlink.h"

#define SYS_ASCII_ADDRESS   5
#define NEXT_LINE_HIGH_BYTE 1

static uint8_t basicSys[13] = {0x0C, 0x08, 0x0A, 0x00, 0x9E, 0x31, 0x32, 0x33, 0x34, 0x00, 0x00, 0x00, 0x00};

#define MEGA65_SYS_ASCII_ADDRESS 9
static uint8_t mega65BasicSys[17] = {0x10, 0x20, 0x0A, 0x00, 0xFE, 0x02, 0x30, 0x3A, 0x9E,
                                     0x31, 0x32, 0x33, 0x34, 0x00, 0x00, 0x00, 0x00};

static void
setupUnbankedCommodore(uint32_t baseAddress, uint32_t size) {
	MemoryGroup* group;
	MemoryPool* codepool;

	codepool = pool_Create(0, UINT32_MAX, baseAddress, 0, size, false);

	//	Create CODE group
	group = group_Create("CODE", 1);
	group->pools[0] = codepool;

	//	Create DATA group
	group = group_Create("DATA", 1);
	group->pools[0] = codepool;

	//	Create BSS group
	group = group_Create("BSS", 1);
	group->pools[0] = codepool;
	group->pools[1] = pool_Create(-1, UINT32_MAX, 0x200, 0, baseAddress, true);
	group->pools[2] = pool_Create(-1, UINT32_MAX, baseAddress + size, 0, 0x10000 - baseAddress - size, true);
}

static void
setupCommodore128ROM(uint32_t baseAddress, uint32_t size) {
	MemoryGroup* group;
	MemoryPool* pool;

	pool = pool_Create(0, UINT32_MAX, baseAddress, 0, size, false);

	//	Create CODE group
	group = group_Create("CODE", 1);
	group->pools[0] = pool;

	//	Create DATA group
	group = group_Create("DATA", 1);
	group->pools[0] = pool;

	//	Create BSS group
	pool = pool_Create(-1, UINT32_MAX, 0x0000, 0, 0x10000, true);
	group = group_Create("BSS", 1);
	group->pools[0] = pool;
}

static int32_t
findStartAddress(const char* entry) {
	if (entry != NULL) {
		SSymbol* entrySymbol = sect_FindExportedSymbol(entry);
		if (entrySymbol == NULL)
			error("Entry symbol \"%s\" not found (it must be exported)", entry);
		return entrySymbol->value;
	}
	return sect_StartAddressOfFirstCodeSection();
}

static void
writeHeader(FILE* fileHandle, const char* entry, uint32_t headerAddress) {
	fputc(headerAddress & 0xFFu, fileHandle);
	fputc((headerAddress >> 8u) & 0xFFu, fileHandle);

	basicSys[NEXT_LINE_HIGH_BYTE] = (headerAddress >> 8u) & 0xFF;
	snprintf((char*) &basicSys[SYS_ASCII_ADDRESS], 6, "%d", findStartAddress(entry));

	fwrite(basicSys, 1, sizeof(basicSys), fileHandle);
}

extern void
commodore_WritePrg(const char* outputFilename, const char* entry, uint32_t headerAddress) {
	FILE* fileHandle = fopen(outputFilename, "w+b");
	if (fileHandle == NULL)
		error("Unable to open \"%s\" for writing", outputFilename);

	writeHeader(fileHandle, entry, headerAddress);

	image_WriteBinaryToFile(fileHandle, -1);
}

static void
writeMega65Header(FILE* fileHandle, const char* entry) {
	fputc(0x01, fileHandle);
	fputc(0x20, fileHandle);

	snprintf((char*) &mega65BasicSys[MEGA65_SYS_ASCII_ADDRESS], 6, "%d", findStartAddress(entry));

	fwrite(mega65BasicSys, 1, sizeof(mega65BasicSys), fileHandle);
}

extern void
commodore_WriteMega65Prg(const char* outputFilename, const char* entry) {
	FILE* fileHandle = fopen(outputFilename, "w+b");
	if (fileHandle == NULL)
		error("Unable to open \"%s\" for writing", outputFilename);

	writeMega65Header(fileHandle, entry);

	image_WriteBinaryToFile(fileHandle, -1);
}

extern void
commodore_SetupUnbankedCommodore128(void) {
	setupUnbankedCommodore(0x1C0E, 0xF000 - 0x1C0E);
}

extern void
commodore_SetupCommodore64(void) {
	setupUnbankedCommodore(0x080E, 0xA000 - 0x080E);
}

extern void
commodore_SetupCommodore264(void) {
	setupUnbankedCommodore(0x100E, 0xFD00 - 0x100E);
}

extern void
commodore_SetupMega65(void) {
	setupUnbankedCommodore(0x2012, 0xF700 - 0x2012);
}

extern void
commodore_SetupCommodore128FunctionROM(void) {
	setupCommodore128ROM(0x8000, 0x8000);
}

extern void
commodore_SetupCommodore128FunctionROMLow(void) {
	setupCommodore128ROM(0x8000, 0x4000);
}

extern void
commodore_SetupCommodore128FunctionROMHigh(void) {
	setupCommodore128ROM(0xC000, 0x4000);
}
