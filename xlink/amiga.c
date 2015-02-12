/*  Copyright 2008-2015 Carsten Elton Sorensen

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
#include <stdlib.h>
#include <string.h>
#include "xlink.h"

#define HUNK_HEADER 0x3F3
#define HUNKF_CHIP  (1 << 30)

static void writeInt32(int32_t d, FILE* f)
{
    fputc((d >> 24) & 0xFF, f);
    fputc((d >> 16) & 0xFF, f);
    fputc((d >> 8) & 0xFF, f);
    fputc(d & 0xFF, f);
}


static void writeBuffer(void* buffer, int bufferSize, FILE* fileHandle)
{
    fwrite(buffer, 1, bufferSize, fileHandle);

    while ((bufferSize & 3) != 0)
    {
        fputc(0, fileHandle);
        ++bufferSize;
    }
}


static void writeString(char* string, FILE* fileHandle, uint32_t extFlags)
{
    int stringLength = (int)strlen(string);

    writeInt32(((stringLength + 3) / 4) | extFlags, fileHandle);
    writeBuffer(string, stringLength, fileHandle);
}


static void writeSectionNames(FILE* fileHandle, bool_t debugInfo)
{
    if (debugInfo)
    {
        for (Section* section = g_sections; section != NULL; section = section->nextSection)
            writeString(section->name, fileHandle, 0);
    }

    writeInt32(0, fileHandle);
}


static int32_t countSectionsToWrite(void)
{
    int32_t count = 0;

    for (Section* section = g_sections; section != NULL; section = section->nextSection)
    {
        if (section->used && section->group != NULL)
            ++count;
    }

    return count;
}


static void writeHunkHeader(FILE* fileHandle, bool_t debugInfo)
{
    int32_t totalSections = countSectionsToWrite();

    writeInt32(HUNK_HEADER, fileHandle);
    writeSectionNames(fileHandle, debugInfo);

    writeInt32(totalSections, fileHandle);     // total hunks
    writeInt32(0, fileHandle);            // first hunk number
    writeInt32(totalSections - 1, fileHandle); // last hunk number

    for (Section* section = g_sections; section != NULL; section = section->nextSection)
    {
        if (section->used && section->group != NULL)
        {
            uint32_t size = (section->size + 3) / 4;
            if (section->group->flags & GROUP_FLAG_CHIP)
                size |= HUNKF_CHIP;
            writeInt32(size, fileHandle);
        }
    }
}

static void writeExecutable(FILE* fileHandle, bool_t debugInfo)
{
    writeHunkHeader(fileHandle, debugInfo);
}

void amiga_WriteExecutable(char* filename, bool_t debugInfo)
{
    FILE* fileHandle;

    if ((fileHandle = fopen(filename, "wb")) != NULL)
    {
        writeExecutable(fileHandle, debugInfo);
        fclose(fileHandle);
    }
    else
    {
        Error("Unable to open file \"%s\" for writing", filename);
    }
}
