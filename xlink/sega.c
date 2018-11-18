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

#include "xlink.h"


static void fputml(uint32_t d, FILE* f)
{
	fputc((d >> 24) & 0xFF, f);
	fputc((d >> 16) & 0xFF, f);
	fputc((d >> 8) & 0xFF, f);
	fputc(d & 0xFF, f);
}


static void fputmw(uint16_t d, FILE* f)
{
	fputc((d >> 8) & 0xFF, f);
	fputc(d & 0xFF, f);
}


static uint16_t fgetmw(FILE* f)
{
    uint16_t hi = fgetc(f) << 8;
    return hi | fgetc(f);
}


static uint16_t sega_CalcChecksum(FILE* fileHandle, long length)
{
    uint16_t r = 0;
    long c = (length - 0x200) >> 1;

    fseek(fileHandle, 0x200, SEEK_SET);
    while (c--)
        r += fgetmw(fileHandle);

    return r;
}


static void sega_UpdateHeader(FILE* fileHandle)
{
    fseek(fileHandle, 0, SEEK_END);
    long length = ftell(fileHandle);

    fseek(fileHandle, 0x1A4, SEEK_SET);
    fputml(length - 1, fileHandle);

    uint16_t checksum = sega_CalcChecksum(fileHandle, length);
    fseek(fileHandle, 0x18E, SEEK_SET);
    fputmw(checksum, fileHandle);
}


void sega_WriteMegaDriveImage(char* outputFilename)
{
	FILE* fileHandle = fopen(outputFilename, "w+b");
	if (fileHandle == NULL)
		Error("Unable to open \"%s\" for writing", outputFilename);

	image_WriteBinaryToFile(fileHandle, 0);

    sega_UpdateHeader(fileHandle);

	fclose(fileHandle);
}
