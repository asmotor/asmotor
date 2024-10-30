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
#include <memory.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// From util
#include "file.h"
#include "fmath.h"
#include "mem.h"
#include "map.h"

// From xlink
#include "object.h"
#include "section.h"
#include "str.h"
#include "xlink.h"

#define WRITE_BLOCK_SIZE 16384

static void
freeVecFileHandle(intptr_t userData, intptr_t v) {
	fclose((FILE*) v);
}

static void
writeRepeatedBytes(FILE* fileHandle, void* data, uint32_t offset, int bytes) {
    fseek(fileHandle, offset, SEEK_SET);
    while (bytes > 0) {
        uint32_t towrite = bytes > WRITE_BLOCK_SIZE ? WRITE_BLOCK_SIZE : bytes;
        if (towrite != fwrite(data, 1, towrite, fileHandle))
            error("Disk possibly full");
        bytes -= towrite;
    }
}

static void*
allocEmptyBytes(void) {
    void* data = (char*) mem_Alloc(WRITE_BLOCK_SIZE);
    if (data == NULL)
        error("Out of memory");

    memset(data, 0xFF, WRITE_BLOCK_SIZE);

    return data;
}


static FILE*
getFileHandle(map_t* fileHandles, uint32_t overlay) {
	intptr_t int_file;
	if (map_Value(fileHandles, overlay, &int_file)) {
		return (FILE*) int_file;
	}

	string* name = str_CreateFormat("%s.%d", g_outputFilename, overlay);
	FILE* new_handle = fopen(str_String(name), "wb");

	map_Insert(fileHandles, overlay, (intptr_t) new_handle);
	str_Free(name);

	return new_handle;
}

extern void
image_WriteBinaryToFile(map_t* fileHandles, int padding) {
	FILE* mainFile = getFileHandle(fileHandles, UINT32_MAX);
	assert(mainFile != NULL);

	uint32_t headerSize = ftell(mainFile);
    char* emptyBytes = allocEmptyBytes();

    for (SSection* section = sect_Sections; section != NULL; section = section->nextSection) {
        //	This is a special exported EQU symbol section
        if (sect_IsEquSection(section))
            continue;

        if (section->used && section->assigned && section->imageLocation != -1 && section->group->type != GROUP_BSS) {
            uint32_t startOffset = section->imageLocation;

			if (section->overlay == UINT32_MAX) {
				startOffset += headerSize;
			}

            uint32_t endOffset = startOffset + section->size;
			FILE* file = getFileHandle(fileHandles, section->overlay);
			uint32_t currentFileSize = fsize(file);

            if (startOffset > currentFileSize) {
                fseek(file, currentFileSize, SEEK_SET);
                writeRepeatedBytes(file, emptyBytes, currentFileSize, startOffset - currentFileSize);
            }

            fseek(file, startOffset, SEEK_SET);
            fwrite(section->data, 1, section->size, file);
            if (endOffset > currentFileSize)
                currentFileSize = endOffset;
        }
    }

    if (padding != -1) {
		uint32_t mainFileSize = fsize(mainFile);
        int bytesToPad = padding == 0 ? (2u << log2n(mainFileSize)) - mainFileSize : padding - mainFileSize % padding;
        writeRepeatedBytes(mainFile, emptyBytes, mainFileSize, bytesToPad);
    }

    mem_Free(emptyBytes);
}


static bool
intKeyEquals(intptr_t userData, intptr_t element1, intptr_t element2) {
	return element1 == element2;
}


static uint32_t
intKeyHash(intptr_t userData, intptr_t element) {
	return (uint32_t) element;
}


static void
intKeyFree(intptr_t userData, intptr_t element) {
}


extern void
image_WriteBinary(int padding) {
	map_t* fileHandles = map_Create(intKeyEquals, intKeyHash, intKeyFree, freeVecFileHandle);

    FILE* fileHandle = fopen(g_outputFilename, "wb");
    if (fileHandle == NULL) {
        error("Unable to open \"%s\" for writing", g_outputFilename);
	}

	map_Insert(fileHandles, UINT32_MAX, (intptr_t) fileHandle);
    image_WriteBinaryToFile(fileHandles, padding);

	map_Free(fileHandles);
}
