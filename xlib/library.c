/*  Copyright 2008-2017 Carsten Elton Sorensen

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

/*
 *	uint32_t	"XLB0"
 *	uint32_t	TotalFiles
 *	REPT	TotalFiles
 *		ASCIIZ	Name
 *		uint32_t	Size
 *		uint8_t	Data[Size]
 *	ENDR
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asmotor.h"
#include "types.h"
#include "file.h"
#include "mem.h"

#include "module.h"

extern void
fatalError(const char* s);

static void
truncateFileName(char* dest, const char* src) {
    int32_t l;

    l = (int32_t) strlen(src) - 1;
    while ((l >= 0) && (src[l] != '\\') && (src[l] != '/'))
        --l;

    strcpy(dest, &src[l + 1]);
}

static SModule*
readLib0(FILE* fileHandle, size_t size) {
    if (size) {
        SModule* l = NULL;
        SModule* first = NULL;

        fgetll(fileHandle);
        size -= 4;    //	Skip count

        while (size > 0) {
            if (l == NULL) {
                first = l = (SModule*) mem_Alloc(sizeof(SModule));
            } else {
                l->nextModule = (SModule*) mem_Alloc(sizeof(SModule));
                l = l->nextModule;
            }

            size -= fgetsz(l->name, MAXNAMELENGTH, fileHandle);
            l->byteLength = fgetll(fileHandle);
            size -= 4;

            l->data = (uint8_t*) mem_Alloc(l->byteLength);
            if (l->byteLength != fread(l->data, sizeof(uint8_t), l->byteLength, fileHandle))
                fatalError("File read failed");
            size -= l->byteLength;

            l->nextModule = NULL;
        }
        return first;
    }

    return NULL;
}

SModule*
lib_Read(const char* filename) {
    FILE* fileHandle = fopen(filename, "rb");

    if (fileHandle != NULL) {
        size_t size = fsize(fileHandle);
        if (size == 0) {
            fclose(fileHandle);
            return NULL;
        }

        char ID[5];
        if (4 != fread(ID, sizeof(char), 4, fileHandle))
            internalerror("File read failed");
        ID[4] = 0;
        size -= 4;

        if (strcmp(ID, "XLB0") == 0) {
            SModule* result = readLib0(fileHandle, size);
            fclose(fileHandle);
            return result;
        } else {
            fclose(fileHandle);
            fatalError("Not a valid xLib library");
            return NULL;
        }
    } else {
        return NULL;
    }
}

bool
lib_Write(SModule* library, const char* filename) {
    FILE* fileHandle = fopen(filename, "wb");

    if (fileHandle != NULL) {
        fwrite("XLB0", sizeof(char), 4, fileHandle);
        fputll(0, fileHandle);

        uint32_t count = 0;
        while (library != NULL) {
            fputsz(library->name, fileHandle);
            fputll(library->byteLength, fileHandle);
            fwrite(library->data, sizeof(uint8_t), library->byteLength, fileHandle);
            library = library->nextModule;
            ++count;
        }

        fseek(fileHandle, 4, SEEK_SET);
        fputll(count, fileHandle);

        fclose(fileHandle);
        return true;
    }

    return false;
}

SModule*
lib_Find(SModule* library, const char* filename) {
    char truncatedName[MAXNAMELENGTH];
    truncateFileName(truncatedName, filename);

    while (library != NULL) {
        if (strcmp(library->name, truncatedName) == 0)
            break;

        library = library->nextModule;
    }

    return library;
}

SModule*
lib_AddReplace(SModule* library, const char* filename) {
    FILE* fileHandle = fopen(filename, "rb");

    if (fileHandle != NULL) {
        char truncatedName[MAXNAMELENGTH];
        truncateFileName(truncatedName, filename);

        SModule* module = lib_Find(library, filename);
        if (module == NULL) {
            module = (SModule*) mem_Alloc(sizeof(SModule));
            module->nextModule = library;
            library = module;
        } else {
            /* Module already exists */
            mem_Free(module->data);
        }

        module->byteLength = (uint32_t) fsize(fileHandle);
        strcpy(module->name, truncatedName);
        module->data = (uint8_t*) mem_Alloc(module->byteLength);

        if (module->byteLength != fread(module->data, sizeof(uint8_t), module->byteLength, fileHandle))
            internalerror("File read failed");

        fclose(fileHandle);
    }

    return library;
}

SModule*
lib_DeleteModule(SModule* library, const char* filename) {
    SModule** pp;
    SModule** first;
    first = pp = &library;

    char truncatedName[MAXNAMELENGTH];
    truncateFileName(truncatedName, filename);

    bool found = false;
    while (*pp != NULL && !found) {
        if (strcmp((*pp)->name, truncatedName) == 0) {
            SModule* t = *pp;

            if (t->data)
                mem_Free(t->data);

            *pp = t->nextModule;

            mem_Free(t);
            found = 1;
        }
        pp = &(*pp)->nextModule;
    }

    if (!found)
        fatalError("Module not found");

    return *first;
}

void
lib_Free(SModule* library) {
    while (library != NULL) {
        SModule* l;

        if (library->data != NULL)
            mem_Free(library->data);

        l = library;
        library = library->nextModule;
        mem_Free(l);
    }
}
