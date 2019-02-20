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

#ifndef UTIL_STRBUF_H_INCLUDED_
#define UTIL_STRBUF_H_INCLUDED_

#include <string.h>

#include "asmotor.h"
#include "str.h"

typedef struct {
    size_t size;
    size_t allocated;
    char* data;
} string_buffer;

extern string_buffer*
strbuf_Create(void);

extern void
strbuf_Free(string_buffer* buffer);

extern string*
strbuf_String(string_buffer* buffer);

extern void
strbuf_AppendChars(string_buffer* buffer, const char* data, size_t length);

INLINE void
strbuf_AppendChar(string_buffer* buffer, char ch) {
    strbuf_AppendChars(buffer, &ch, 1);
}

INLINE void
strbuf_AppendStringZero(string_buffer* buffer, const char* str) {
    if (str == NULL)
        return;

    strbuf_AppendChars(buffer, str, strlen(str));
}

INLINE void
strbuf_AppendString(string_buffer* buffer, const string* str) {
    if (str == NULL)
        return;

    strbuf_AppendChars(buffer, str_String(str), str_Length(str));
}

#endif /* UTIL_STRBUF_H_INCLUDED_ */
