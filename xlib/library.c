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
 *		uint32_t	Time
 *		uint32_t	Date
 *		uint32_t	Size
 *		uint8_t	Data[Size]
 *	ENDR
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"
#include "mem.h"

#include "asmotor.h"
#include "types.h"

#include "libwrap.h"

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

SLibrary*
lib_ReadLib0(FILE* fileHandle, size_t size) {
    if (size) {
        SLibrary* l = NULL;
        SLibrary* first = NULL;

        fgetll(fileHandle);
        size -= 4;    //	Skip count

        while (size > 0) {
            if (l == NULL) {
                first = l = (SLibrary*) mem_Alloc(sizeof(SLibrary));
            } else {
                l->pNext = (SLibrary*) mem_Alloc(sizeof(SLibrary));
                l = l->pNext;
            }

            size -= fgetsz(l->tName, MAXNAMELENGTH, fileHandle);
            l->ulTime = fgetll(fileHandle);
            size -= 4;
            l->ulDate = fgetll(fileHandle);
            size -= 4;
            l->nByteLength = fgetll(fileHandle);
            size -= 4;

            l->pData = (uint8_t*) mem_Alloc(l->nByteLength);
            if (l->nByteLength != fread(l->pData, sizeof(uint8_t), l->nByteLength, fileHandle))
                fatalError("File read failed");
            size -= l->nByteLength;

            l->pNext = NULL;
        }
        return first;
    }

    return NULL;
}

SLibrary*
lib_Read(const char* filename) {
    FILE* f;

    if ((f = fopen(filename, "rb")) != NULL) {
        size_t size;
        char ID[5];

        size = fsize(f);
        if (size == 0) {
            fclose(f);
            return NULL;
        }

        if (4 != fread(ID, sizeof(char), 4, f))
            internalerror("File read failed");
        ID[4] = 0;
        size -= 4;

        if (strcmp(ID, "XLB0") == 0) {
            SLibrary* r;

            r = lib_ReadLib0(f, size);
            fclose(f);
            return r;
        } else {
            fclose(f);
            fatalError("Not a valid xLib library");
            return NULL;
        }
    } else {
        return NULL;
    }
}

bool
lib_Write(SLibrary* lib, const char* filename) {
    FILE* f;

    if ((f = fopen(filename, "wb")) != NULL) {
        uint32_t count = 0;

        fwrite("XLB0", sizeof(char), 4, f);
        fputll(0, f);

        while (lib) {
            fputsz(lib->tName, f);
            fputll(lib->ulTime, f);
            fputll(lib->ulDate, f);
            fputll(lib->nByteLength, f);
            fwrite(lib->pData, sizeof(uint8_t), lib->nByteLength, f);
            lib = lib->pNext;
            ++count;
        }

        fseek(f, 4, SEEK_SET);
        fputll(count, f);

        fclose(f);
        return true;
    }

    return false;
}

SLibrary*
lib_Find(SLibrary* lib, const char* filename) {
    char truncatedName[MAXNAMELENGTH];

    truncateFileName(truncatedName, filename);

    while (lib) {
        if (strcmp(lib->tName, truncatedName) == 0)
            break;

        lib = lib->pNext;
    }

    return lib;
}

SLibrary*
lib_AddReplace(SLibrary* lib, const char* filename) {
    FILE* f;

    if ((f = fopen(filename, "rb")) != NULL) {
        SLibrary* module;
        char truncatedName[MAXNAMELENGTH];

        truncateFileName(truncatedName, filename);

        if ((module = lib_Find(lib, filename)) == NULL) {
            module = (SLibrary*) mem_Alloc(sizeof(SLibrary));
            module->pNext = lib;
            lib = module;
        } else {
            /* Module already exists */
            mem_Free(module->pData);
        }

        module->nByteLength = (uint32_t) fsize(f);
        strcpy(module->tName, truncatedName);
        module->pData = (uint8_t*) mem_Alloc(module->nByteLength);

        if (module->nByteLength != fread(module->pData, sizeof(uint8_t), module->nByteLength, f))
            internalerror("File read failed");

        fclose(f);
    }

    return lib;
}

SLibrary*
lib_DeleteModule(SLibrary* lib, const char* filename) {
    char truncatedName[MAXNAMELENGTH];
    SLibrary** pp;
    SLibrary** first;
    bool found = 0;

    first = pp = &lib;

    truncateFileName(truncatedName, filename);
    while (*pp != NULL && !found) {
        if (strcmp((*pp)->tName, truncatedName) == 0) {
            SLibrary* t = *pp;

            if (t->pData)
                mem_Free(t->pData);

            *pp = t->pNext;

            mem_Free(t);
            found = 1;
        }
        pp = &(*pp)->pNext;
    }

    if (!found)
        fatalError("Module not found");

    return *first;
}

void
lib_Free(SLibrary* lib) {
    while (lib) {
        SLibrary* l;

        if (lib->pData)
            mem_Free(lib->pData);

        l = lib;
        lib = lib->pNext;
        mem_Free(l);
    }
}
