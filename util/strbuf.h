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

#if !defined(UTIL_STRBUF_H_INCLUDED_)
#define UTIL_STRBUF_H_INCLUDED_

#include <string.h>

#include "asmotor.h"
#include "str.h"

typedef struct {
	size_t nSize;
	size_t nAllocated;
	char* pBuffer;
} stringbuffer;

extern stringbuffer* strbuf_Create(void);
extern void strbuf_Free(stringbuffer* pBuffer);
extern string* strbuf_String(stringbuffer* pBuffer);
extern void strbuf_AppendChars(stringbuffer* pBuffer, const char* pChars, size_t nCount);

INLINE void strbuf_AppendChar(stringbuffer* pBuffer, char nChar) {
	strbuf_AppendChars(pBuffer, &nChar, 1);
}

INLINE void strbuf_AppendStringZero(stringbuffer* pBuffer, const char* pszString) {
	if (pszString == NULL)
		return;

	strbuf_AppendChars(pBuffer, pszString, strlen(pszString));
}

#endif
