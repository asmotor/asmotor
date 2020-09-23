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

#ifndef XASM_COMMON_FILEBUFFER_H_INCLUDED_
#define XASM_COMMON_FILEBUFFER_H_INCLUDED_

#include "charstack.h"
#include "lists.h"
#include "strcoll.h"

typedef struct FileBuffer {
	SCharStack charStack;
	string* uniqueValue;
    string* text;
	ssize_t index;
    vec_t* arguments;
} SFileBuffer;

extern void
fbuf_Init(SFileBuffer* fileBuffer, string* buffer, vec_t* arguments);

extern void
fbuf_Destroy(SFileBuffer* fileBuffer);

extern SFileBuffer*
fbuf_Create(string* buffer, vec_t* arguments);

extern void
fbuf_ShiftArguments(SFileBuffer* fbuffer, int32_t count);

extern char
fbuf_GetChar(SFileBuffer* fbuffer);

extern void
fbuf_UnputChar(SFileBuffer* fbuffer, char ch);

extern char
fbuf_GetUnexpandedChar(SFileBuffer* fbuffer, size_t index);

extern void
fbuf_Copy(SFileBuffer* dest, const SFileBuffer* source);

extern void
fbuf_CopyUnexpandedContent(SFileBuffer* fbuffer, char* dest, size_t count);

extern size_t
fbuf_SkipUnexpandedChars(SFileBuffer* fbuffer, size_t count);

extern void
fbuf_RenewUniqueValue(SFileBuffer* fbuffer);


#endif /* XASM_COMMON_FILEBUFFER_H_INCLUDED_ */
