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

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "file.h"

#include "error.h"
#include "group.h"
#include "object.h"
#include "section.h"
#include "symbol.h"

#define PGZ_32BIT 'z'

#define F256_SLOT_SIZE   8192
#define F256_HEADER_SIZE 10

#define MINIMUM_STRING_SIZE 2

static off_t g_sectionSizeLocation;
static int32_t g_sectionSize;
static int32_t g_nextCpuByteLocation;

static void
writePGZSection(FILE* fileHandle, int32_t cpuByteLocation, uint32_t size, void* data) {
	// Time to start a new section?
	if (cpuByteLocation != g_nextCpuByteLocation) {
		// Update current section's size
		if (g_sectionSizeLocation != -1) {
			fseeko(fileHandle, g_sectionSizeLocation, SEEK_SET);
			fputll(g_sectionSize, fileHandle);
			fseeko(fileHandle, 0, SEEK_END);
		}

		// Begin a new section
		fputll(cpuByteLocation, fileHandle);
		g_sectionSizeLocation = ftello(fileHandle);
		g_sectionSize = 0;
		fputll(0, fileHandle);
	}

	if (size != 0) {
		assert(data != NULL);
		fwrite(data, 1, size, fileHandle);
	}

	g_sectionSize += size;
	g_nextCpuByteLocation = cpuByteLocation + size;
}

static void
writePGZSections(FILE* fileHandle) {
	g_sectionSizeLocation = -1;
	g_sectionSize = -1;
	g_nextCpuByteLocation = -1;

	for (SSection* section = sect_Sections; section != NULL; section = section->nextSection) {
		if (section->data != NULL && section->used) {
			writePGZSection(fileHandle, section->cpuByteLocation, section->size, section->data);
		}
	}
}

static void
writeRepeatedBytes(FILE* fileHandle, uint32_t offset, int bytes) {
	fseek(fileHandle, offset, SEEK_SET);
	ffill(0xFF, bytes, fileHandle);
}

static void
writeKUPSections(FILE* fileHandle, int firstSlot, bool pad) {
	int imageStart = firstSlot * 8192;
	uint32_t currentFileSize = ftell(fileHandle);

	for (SSection* section = sect_Sections; section != NULL; section = section->nextSection) {
		//	This is a special exported EQU symbol section
		if (sect_IsEquSection(section))
			continue;

		if (section->used && section->assigned && section->imageLocation != -1 && section->group->type != GROUP_BSS) {
			if (section->imageLocation < imageStart + F256_HEADER_SIZE) {
				error("Section \"%s\" overlaps header", section->name);
			}

			uint32_t startOffset = section->imageLocation - imageStart;
			uint32_t endOffset = startOffset + section->size;

			if (startOffset > currentFileSize) {
				fseek(fileHandle, currentFileSize, SEEK_SET);
				writeRepeatedBytes(fileHandle, currentFileSize, startOffset - currentFileSize);
			}

			fseek(fileHandle, startOffset, SEEK_SET);
			fwrite(section->data, 1, section->size, fileHandle);
			if (endOffset > currentFileSize)
				currentFileSize = endOffset;
		}
	}

	if (pad) {
		int bytesToPad = F256_SLOT_SIZE - currentFileSize % F256_SLOT_SIZE;
		writeRepeatedBytes(fileHandle, currentFileSize, bytesToPad);
	}
}

extern void
foenix_WriteExecutablePGZ(const char* outputFilename, const char* entry) {
	FILE* fileHandle = fopen(outputFilename, "w+b");
	if (fileHandle == NULL) {
		error("Unable to open \"%s\" for writing", outputFilename);
	}

	// "Header"
	fputc(PGZ_32BIT, fileHandle);

	// The rest of the sections
	writePGZSections(fileHandle);

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
	writePGZSection(fileHandle, startAddress, 0, NULL);

	fclose(fileHandle);
}

extern void
foenix_WriteExecutableKUP(const char* outputFilename, const char* entry, bool pad) {
	FILE* fileHandle = fopen(outputFilename, "w+b");
	if (fileHandle == NULL) {
		error("Unable to open \"%s\" for writing", outputFilename);
	}

	// Start address section
	int startAddress = 0;
	if (entry != NULL) {
		SSymbol* entrySymbol = sect_FindExportedSymbol(entry);
		if (entrySymbol == NULL)
			error("Entry symbol \"%s\" not found (it must be exported)", entry);
		startAddress = entrySymbol->value;
	} else {
		error("Kernel user programs must have an entry address");
	}

	int firstSlot = sect_StartAddressOfFirstCodeSection() / F256_SLOT_SIZE;
	int lastSlot = sect_EndAddressOfLastCodeSection() / F256_SLOT_SIZE;
	int totalSlots = lastSlot - firstSlot + 1;

	if (startAddress < F256_HEADER_SIZE + MINIMUM_STRING_SIZE) {
		error("Start address must not be in the header, or the following name");
	}

	if (firstSlot == 0) {
		error("Program must not start in slot 0");
	}

	if (totalSlots > 4) {
		error("Program must not be larger than 4 slots");
	}

	fputlw(0x56F2, fileHandle);       // Magic header
	fputc(totalSlots, fileHandle);    // size in 8KiB blocks
	fputc(firstSlot, fileHandle);     // first slot to load program into
	fputlw(startAddress, fileHandle); // entry address
	ffill(0, 4, fileHandle);          // reserved area

	writeKUPSections(fileHandle, firstSlot, pad);

	fclose(fileHandle);
}

extern void
foenix_SetupFoenixA2560XGroups(void) {
	MemoryGroup* group;

	MemoryPool* system_ram = pool_Create(0x10000, UINT32_MAX, 0x10000, 0, 0x400000 - 0x10000, false);
	MemoryPool* vicky_a_ram = pool_Create(0x800000, UINT32_MAX, 0x800000, 0, 0x400000, false);
	MemoryPool* vicky_b_ram = pool_Create(0xC00000, UINT32_MAX, 0xC00000, 0, 0x400000, false);
	MemoryPool* sdram = pool_Create(0x02000000, UINT32_MAX, 0x02000000, 0, 0x04000000, false);

	//	Create CODE group

	group = group_Create("CODE", 1);
	group->pools[0] = system_ram;

	//	Create DATA group

	group = group_Create("DATA", 2);
	group->pools[0] = system_ram;
	group->pools[1] = sdram;

	//	Create BSS group

	group = group_Create("BSS", 2);
	group->pools[0] = system_ram;
	group->pools[1] = sdram;

	//	Create DATA_VA group

	group = group_Create("DATA_VA", 1);
	group->pools[0] = vicky_a_ram;

	//	Create BSS_VA group

	group = group_Create("BSS_VA", 1);
	group->pools[0] = vicky_a_ram;

	//	Create DATA_VB group

	group = group_Create("DATA_VB", 1);
	group->pools[0] = vicky_b_ram;

	//	Create BSS_VB group

	group = group_Create("BSS_VB", 1);
	group->pools[0] = vicky_b_ram;

	//	Create DATA_D group

	group = group_Create("DATA_D", 1);
	group->pools[0] = sdram;

	//	Create BSS_D group

	group = group_Create("BSS_D", 1);
	group->pools[0] = sdram;

	//	initialise memory chunks

	group_InitMemoryChunks();
}

void
foenix_SetupFoenixF256JrSmallGroups(void) {
	MemoryGroup* group;
	MemoryPool* main_ram = pool_Create(0, UINT32_MAX, 0, 0, 0xC000, false);
	MemoryPool* high_ram = pool_Create(0xE000, UINT32_MAX, 0xE000, 0, 0x10000 - 0xE000, false);
	MemoryPool* zp = pool_Create(-1, UINT32_MAX, 0x0010, 0, 0x100 - 0x10, false);

	//	Create HOME group

	group = group_Create("HOME", 1);
	group->pools[0] = high_ram;

	//	Create CODE group

	group = group_Create("CODE", 2);
	group->pools[0] = main_ram;
	group->pools[1] = high_ram;

	//	Create DATA group

	group = group_Create("DATA", 1);
	group->pools[0] = main_ram;

	//	Create BSS group

	group = group_Create("BSS", 1);
	group->pools[0] = main_ram;

	//	Create ZP group

	group = group_Create("ZP", 1);
	group->pools[0] = zp;

	//	initialise memory chunks

	group_InitMemoryChunks();
}
