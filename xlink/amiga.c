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

#define HUNK_UNIT    0x3E7
#define HUNK_NAME    0x3E8
#define HUNK_CODE    0x3E9
#define HUNK_DATA    0x3EA
#define HUNK_BSS     0x3EB
#define HUNK_RELOC32 0x3EC
#define HUNK_EXT     0x3EF
#define HUNK_SYMBOL  0x3F0
#define HUNK_END     0x3F2
#define HUNK_HEADER  0x3F3
#define HUNKF_CHIP   (1 << 30)

#define EXT_DEF      0x01000000
#define EXT_REF32    0x81000000


static void writeInt32(FILE* f, int32_t d)
{
    fputc((d >> 24) & 0xFF, f);
    fputc((d >> 16) & 0xFF, f);
    fputc((d >> 8) & 0xFF, f);
    fputc(d & 0xFF, f);
}


static void writeBuffer(FILE* fileHandle, void* buffer, int bufferSize)
{
    fwrite(buffer, 1, bufferSize, fileHandle);

    while ((bufferSize & 3) != 0)
    {
        fputc(0, fileHandle);
        ++bufferSize;
    }
}


static void writeString(FILE* fileHandle, char* string, uint32_t extFlags)
{
    int stringLength = (int)strlen(string);

    writeInt32(fileHandle, ((stringLength + 3) / 4) | extFlags);
    writeBuffer(fileHandle, string, stringLength);
}


static void writeSectionNames(FILE* fileHandle, bool_t debugInfo)
{
    if (debugInfo)
    {
        for (Section* section = g_sections; section != NULL; section = section->nextSection)
            writeString(fileHandle, section->name, 0);
    }

    writeInt32(fileHandle, 0);
}


static void writeSectionSizes(FILE* fileHandle)
{
    for (Section* section = g_sections; section != NULL; section = section->nextSection)
    {
        if (section->used && section->group != NULL)
        {
            uint32_t size = (section->size + 3) / 4;
            if (section->group->flags & GROUP_FLAG_CHIP)
                size |= HUNKF_CHIP;
            writeInt32(fileHandle, size);
        }
    }
}


static void writeHunkHeader(FILE* fileHandle, bool_t debugInfo, uint32_t totalSections)
{
    writeInt32(fileHandle, HUNK_HEADER);
    writeSectionNames(fileHandle, debugInfo);

    writeInt32(fileHandle, totalSections);     // total hunks
    writeInt32(fileHandle, 0);                 // first hunk number
    writeInt32(fileHandle, totalSections - 1); // last hunk number
    writeSectionSizes(fileHandle);
}


static void writeExtHunk(FILE* fileHandle, Section* section, Patches* importPatches, uint32_t codePos)
{
    // TODO: Implement when writing Amiga linker object
}


static uint32_t hunkType(Section* section)
{
    uint32_t hunktype;

    switch (section->group->type)
    {
        case GROUP_TEXT:
            if (section->group->flags & GROUP_FLAG_DATA)
                hunktype = HUNK_DATA;
            else
                hunktype = HUNK_CODE;

            break;
        case GROUP_BSS:
            hunktype = HUNK_BSS;

            break;
    }

    if (section->group->flags & GROUP_FLAG_CHIP)
        hunktype |= HUNKF_CHIP;

    return hunktype;
}


typedef struct
{
    uint32_t capacity;
    uint32_t total;
    uint32_t offsets[];
} Offsets;


static Offsets** allocOffsets(uint32_t totalSections)
{
    Offsets** offsets = mem_Alloc(sizeof(Offsets*) * totalSections);
    for (uint32_t i = 0; i < totalSections; ++i)
    {
        Offsets* offset = mem_Alloc(sizeof(Offsets) + sizeof(uint32_t) * 4);
        offset->capacity = 4;
        offset->total = 0;
        offsets[i] = offset;
    }

    return offsets;
}


static void freeOffsets(Offsets** offsets, uint32_t totalSections)
{
    for (uint32_t i = 0; i < totalSections; ++i)
        mem_Free(offsets[i]);

    mem_Free(offsets);
}


static Offsets** getPatchOffsets(Patches* patches, uint32_t totalSections)
{
    bool_t hasReloc = false;
    Offsets** offsets = allocOffsets(totalSections);
    Patch* patch = patches->patches;

    for (uint32_t index = 0; index < patches->totalPatches; ++index, ++patch)
    {
        if (patch->type == PATCH_RELOC && patch->offsetSection != NULL)
        {
            Offsets** offset = &offsets[patch->offsetSection->sectionId];
            if ((*offset)->total == (*offset)->capacity)
            {
                (*offset)->capacity *= 2;
                *offset = mem_Realloc(*offset, sizeof(Offsets) + sizeof(uint32_t) * (*offset)->capacity);
            }
            (*offset)->offsets[(*offset)->total++] = patch->offset;
            hasReloc = true;
        }
    }

    if (!hasReloc)
    {
        freeOffsets(offsets, totalSections);
        return NULL;
    }

    return offsets;
}


static void writeReloc32(FILE* fileHandle, Section* section, uint32_t totalSections)
{
    Offsets** offsets = getPatchOffsets(section->patches, totalSections);

    if (offsets != NULL)
    {
        writeInt32(fileHandle, HUNK_RELOC32);
        for (uint32_t sectionId = 0; sectionId < totalSections; ++sectionId)
        {
            Offsets* offset = offsets[sectionId];
            if (offset->total > 0)
            {
                writeInt32(fileHandle, offset->total);
                writeInt32(fileHandle, sectionId);

                for (uint32_t i = 0; i < offset->total; ++i)
                    writeInt32(fileHandle, offset->offsets[i]);
            }
        }
        writeInt32(fileHandle, 0);

        freeOffsets(offsets, totalSections);
    }
}


static void writeTextSection(FILE* fileHandle, Section* section, bool_t debugInfo, uint32_t totalSections, bool_t linkObject)
{
    writeInt32(fileHandle, hunkType(section));
    writeInt32(fileHandle, (section->size + 3) / 4);
    writeBuffer(fileHandle, section->data, section->size);
    writeReloc32(fileHandle, section, totalSections);

    if (linkObject)
        writeExtHunk(fileHandle, section, NULL, 0);

}


static void writeBssSection(FILE* fileHandle, Section* section, bool_t debugInfo, uint32_t totalSections, bool_t linkObject)
{
    writeInt32(fileHandle, hunkType(section));
    writeInt32(fileHandle, (section->size + 3) / 4);

    if (linkObject)
        writeExtHunk(fileHandle, section, NULL, 0);
}


static void writeSymbolHunk(FILE* fileHandle, Section* section)
{
}


static void writeSection(FILE* fileHandle, Section* section, bool_t debugInfo, uint32_t totalSections, bool_t linkObject)
{
    switch (section->group->type)
    {
        case GROUP_TEXT:
            writeTextSection(fileHandle, section, debugInfo, totalSections, linkObject);
            break;
        case GROUP_BSS:
            writeBssSection(fileHandle, section, debugInfo, totalSections, linkObject);
            break;
    }

    if(debugInfo)
        writeSymbolHunk(fileHandle, section);

    writeInt32(fileHandle, HUNK_END);
}


static void writeSections(FILE* fileHandle, bool_t debugInfo, uint32_t totalSections, bool_t linkObject)
{
    for (Section* section = g_sections; section != NULL; section = section->nextSection)
    {
        if (section->used && section->group != NULL)
            writeSection(fileHandle, section, debugInfo, totalSections, linkObject);
    }
}


static uint32_t updateSectionIds()
{
    uint32_t sectionId = 0;

    for (Section* section = g_sections; section != NULL; section = section->nextSection)
    {
        if (section->used && section->group != NULL)
            section->sectionId = sectionId++;
        else
            section->sectionId = -1;
    }

    return sectionId;
}


static void writeExecutable(FILE* fileHandle, bool_t debugInfo)
{
    uint32_t totalSections = updateSectionIds();

    writeHunkHeader(fileHandle, debugInfo, totalSections);
    writeSections(fileHandle, debugInfo, totalSections, false);
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
