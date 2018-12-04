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

#include "types.h"

extern size_t fsize(FILE* fileHandle);

extern void fputll(uint32_t value, FILE* fileHandle);
extern uint32_t fgetll(FILE* fileHandle);

extern void fputbl(uint32_t value, FILE* fileHandle);

extern uint16_t fgetbw(FILE* fileHandle);
extern void fputbw(uint16_t value, FILE* fileHandle);

extern void fputlw(uint16_t value, FILE* fileHandle);

extern void fgetsz(char* destination, size_t maxLength, FILE* fileHandle);
extern void fputsz(const char* str, FILE* fileHandle);

#endif /* UTIL_FILE_H_INCLUDED_ */
