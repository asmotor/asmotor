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

#include "file.h"

size_t fsize(FILE* fileHandle) {
	off_t size;
	off_t currentOffset = ftello(fileHandle);
	fseeko(fileHandle, 0, SEEK_END);
	size = ftello(fileHandle);
	fseeko(fileHandle, currentOffset, SEEK_SET);

	return (size_t) size;
}

void fputll(uint32_t d, FILE* f) {
	fputc(d & 0xFFu, f);
	fputc((d >> 8u) & 0xFFu, f);
	fputc((d >> 16u) & 0xFFu, f);
	fputc((d >> 24u) & 0xFFu, f);
}

uint32_t fgetll(FILE* fileHandle) {
	uint32_t r;

	r = (uint8_t) fgetc(fileHandle);
	r |= (uint32_t) fgetc(fileHandle) << 8u;
	r |= (uint32_t) fgetc(fileHandle) << 16u;
	r |= (uint32_t) fgetc(fileHandle) << 24u;

	return r;
}

void fputbl(uint32_t d, FILE* f) {
	fputc((d >> 24u) & 0xFFu, f);
	fputc((d >> 16u) & 0xFFu, f);
	fputc((d >> 8u) & 0xFFu, f);
	fputc(d & 0xFFu, f);
}

uint16_t fgetbw(FILE* f) {
	uint16_t hi = (uint16_t) fgetc(f) << 8u;
	return hi | (uint8_t) fgetc(f);
}

void fputbw(uint16_t d, FILE* f) {
	fputc((uint8_t) (d >> 8u), f);
	fputc((uint8_t) d, f);
}

void fputlw(uint16_t d, FILE* f) {
	fputc((uint8_t) d, f);
	fputc((uint8_t) (d >> 8u), f);
}

void fgetsz(char* destination, size_t maxLength, FILE* fileHandle) {
	if (maxLength > 0) {
		char ch;

		do {
			ch = *destination++ = (char) fgetc(fileHandle);
			--maxLength;
		} while (maxLength != 0 && ch);
	}
}

void fputsz(const char* str, FILE* fileHandle) {
	while (*str) {
		fputc(*str++, fileHandle);
	}
	fputc(0, fileHandle);
}
