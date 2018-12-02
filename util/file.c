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
	fputc(d & 0xFF, f);
	fputc((d >> 8) & 0xFF, f);
	fputc((d >> 16) & 0xFF, f);
	fputc((d >> 24) & 0xFF, f);
}

void fputbl(uint32_t d, FILE* f) {
	fputc((d >> 24) & 0xFF, f);
	fputc((d >> 16) & 0xFF, f);
	fputc((d >> 8) & 0xFF, f);
	fputc(d & 0xFF, f);
}

void fputsz(const char* str, FILE* f) {
	while (*str) {
		fputc(*str++, f);
	}
	fputc(0, f);
}
