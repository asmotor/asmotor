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

#ifndef XASM_COMMON_LEXERBUFFER_H_INCLUDED_
#define XASM_COMMON_LEXERBUFFER_H_INCLUDED_

#include "charstack.h"
#include "lists.h"
#include "strcoll.h"

typedef struct LexerBuffer {
	SCharStack charStack;
    string* name;
	string* uniqueValue;
    string* text;
	ssize_t index;
    vec_t* arguments;
} SLexerBuffer;

extern void
lexbuf_Init(SLexerBuffer* fileBuffer, string* name, string* buffer, vec_t* arguments);

extern void
lexbuf_Destroy(SLexerBuffer* fileBuffer);

extern void
lexbuf_ShiftArguments(SLexerBuffer* fbuffer, int32_t count);

extern char
lexbuf_GetChar(SLexerBuffer* fbuffer);

extern void
lexbuf_UnputChar(SLexerBuffer* fbuffer, char ch);

extern char
lexbuf_GetUnexpandedChar(SLexerBuffer* fbuffer, size_t index);

extern void
lexbuf_Copy(SLexerBuffer* dest, const SLexerBuffer* source);

extern void
lexbuf_ShallowCopy(SLexerBuffer* dest, const SLexerBuffer* source);

extern void
lexbuf_ContinueFrom(SLexerBuffer* dest, const SLexerBuffer* source);

extern void
lexbuf_CopyUnexpandedContent(SLexerBuffer* fbuffer, char* dest, size_t count);

extern size_t
lexbuf_SkipUnexpandedChars(SLexerBuffer* fbuffer, size_t count);

extern void
lexbuf_RenewUniqueValue(SLexerBuffer* fbuffer);


#endif /* XASM_COMMON_LEXERBUFFER_H_INCLUDED_ */
