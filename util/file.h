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

#ifndef UTIL_FILE_H_INCLUDED_
#define UTIL_FILE_H_INCLUDED_

#include <stdio.h>

#include "asmotor.h"
#include "types.h"

/* Determine whether a file exists */
extern bool
fexists(const char* filename);

/* Return the size of a file in bytes */
extern size_t
fsize(FILE* fileHandle);

/* Write a little endian 32 bit value to a file */
extern void
fputll(uint32_t value, FILE* fileHandle);

/* Retrieve a little endian 32 bit value from a file */
extern uint32_t
fgetll(FILE* fileHandle);

/* Write a big endian 32 bit value to a file */
extern void
fputbl(uint32_t value, FILE* fileHandle);

/* Retrieve a big endian 16 bit value from a file */
extern uint16_t
fgetbw(FILE* fileHandle);

/* Write a big endian 16 bit value to a file */
extern void
fputbw(uint16_t value, FILE* fileHandle);

/* Write a little endian 16 bit value to a file */
extern void
fputlw(uint16_t value, FILE* fileHandle);

/* Retrieve a zero terminated string from a file */
extern size_t
fgetsz(char* destination, size_t maxLength, FILE* fileHandle);

/* Write zero terminated string to a file */
extern void
fputsz(const char* str, FILE* fileHandle);

#if defined(_MSC_VER)
typedef __int64 off_t;

INLINE off_t ftello(FILE* fileHandle) {
	return _ftelli64(fileHandle);
}

INLINE int fseeko(FILE* fileHandle, off_t offset, int origin) {
	return _fseeki64(fileHandle, offset, origin);
}
#endif

#endif /* UTIL_FILE_H_INCLUDED_ */
