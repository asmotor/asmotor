/*  Copyright 2008-2021 Carsten Elton Sorensen and contributors

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

static int
startAddressOfFirstCodeSection(void) {
    for (SSection* section = sect_Sections; section != NULL; section = section->nextSection) {
        if (section->used && !sect_IsEquSection(section) && (section->group->type == GROUP_TEXT)) {
            return section->cpuLocation;
        }
    }

    error("No start address found because no CODE sections found");
}

static void
writeHeader(FILE* fileHandle, uint32_t baseAddress) {
    int startAddress = startAddressOfFirstCodeSection();

    fputc(baseAddress & 0xFFu, fileHandle);
    fputc((baseAddress >> 8u) & 0xFFu, fileHandle);

    snprintf((char*) &basicSys[SYS_ASCII_ADDRESS], 5, "%d", startAddress);

    fwrite(basicSys, 1, sizeof(basicSys), fileHandle);
}

extern void
commodore_WritePrg(const char* outputFilename, uint32_t baseAddress) {
    FILE* fileHandle = fopen(outputFilename, "wb");
    if (fileHandle == NULL)
        error("Unable to open \"%s\" for writing", outputFilename);

    writeHeader(fileHandle, baseAddress);

    image_WriteBinaryToFile(fileHandle, -1);

    fclose(fileHandle);
}
