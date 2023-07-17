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

#include <stdio.h>
#include <string.h>

#include "file.h"

#include "group.h"
#include "image.h"
#include "section.h"
#include "xlink.h"

#define SYS_ASCII_ADDRESS 5

static uint8_t basicSys[] = {
    0x0B, 0x08, 0x0A, 0x00, 0x9E, 0x37, 0x31, 0x38, 0x31, 0x00, 0x00, 0x00, 0x00
};

static void
setupUnbankedCommodore(uint32_t baseAddress, uint32_t size) {
    MemoryGroup* group;
    MemoryPool* codepool;

    codepool = pool_Create(0, baseAddress, 0, size, false);

    //	Create CODE group
    group = group_Create("CODE", 1);
    group->pools[0] = codepool;

    //	Create DATA group
    group = group_Create("DATA", 1);
    group->pools[0] = codepool;

    //	Create BSS group
    group = group_Create("BSS", 1);
    group->pools[0] = codepool;
    group->pools[1] = pool_Create(-1, 0x200, 0, baseAddress, true);
    group->pools[2] = pool_Create(-1, baseAddress + size, 0, 0x10000 - baseAddress - size, true);
}

static void
setupCommodore128ROM(uint32_t baseAddress, uint32_t size) {
    MemoryGroup* group;
    MemoryPool* pool;

    pool = pool_Create(0, baseAddress, 0, size, false);

    //	Create CODE group
    group = group_Create("CODE", 1);
    group->pools[0] = pool;

    //	Create DATA group
    group = group_Create("DATA", 1);
    group->pools[0] = pool;

    //	Create BSS group
    pool = pool_Create(-1, 0x0000, 0, 0x10000, true);
    group = group_Create("BSS", 1);
    group->pools[0] = pool;
}

static void
writeHeader(FILE* fileHandle, const char* entry, uint32_t baseAddress) {
    int startAddress = 0;
    if (entry != NULL) {
        SSymbol* entrySymbol = sect_FindExportedSymbol(entry);
        if (entrySymbol == NULL)
            error("Entry symbol \"%s\" not found (it must be exported)", entry);
        startAddress = entrySymbol->value;
    } else {
        startAddress = sect_StartAddressOfFirstCodeSection();
    }

    fputc(baseAddress & 0xFFu, fileHandle);
    fputc((baseAddress >> 8u) & 0xFFu, fileHandle);

    snprintf((char*) &basicSys[SYS_ASCII_ADDRESS], 5, "%d", startAddress);

    fwrite(basicSys, 1, sizeof(basicSys), fileHandle);
}

extern void
commodore_WritePrg(const char* outputFilename, const char* entry, uint32_t baseAddress) {
    FILE* fileHandle = fopen(outputFilename, "wb");
    if (fileHandle == NULL)
        error("Unable to open \"%s\" for writing", outputFilename);

    writeHeader(fileHandle, entry, baseAddress);

    image_WriteBinaryToFile(fileHandle, -1);

    fclose(fileHandle);
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


