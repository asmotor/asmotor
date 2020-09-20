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

#include <assert.h>

#include "filebuffer.h"


static string*
createUniqueValue(void) {
    static uint32_t runId = 0;
    return str_CreateFormat("_%u", runId++);
}


extern SFileBuffer*
fbuf_Create(string* buffer, vec_t* arguments) {
	SFileBuffer* buf = mem_Alloc(sizeof(SFileBuffer));
	chstk_Init(&buf->charStack);
	buf->uniqueValue = createUniqueValue();
	buf->text = str_Copy(buffer);
	buf->index = 0;
	buf->arguments = arguments;

	return buf;
}


extern void
fbuf_ShiftArguments(SFileBuffer* fbuffer, int32_t count) {
	while (count-- > 0 && strvec_Count(fbuffer->arguments) >= 2) {
		strvec_RemoveAt(fbuffer->arguments, 1);
	}
}


static char
nextChar(SFileBuffer* fbuffer) {
	char ch = chstk_Pop(&fbuffer->charStack);
	if (ch != 0) {
		return ch;
	}

	if (fbuffer->index < str_Length(fbuffer->text)) {
		return str_CharAt(fbuffer->text, fbuffer->index++);
	}

	return 0;
}


static char
peekChar(SFileBuffer* fbuffer) {
	char ch = chstk_Peek(&fbuffer->charStack);
	if (ch != 0) {
		return ch;
	}

	if (fbuffer->index < str_Length(fbuffer->text)) {
		return str_CharAt(fbuffer->text, fbuffer->index);
	}

	return 0;
}


extern char
fbuf_GetChar(SFileBuffer* fbuffer) {
	for (;;) {
		char ch = nextChar(fbuffer);
		if (ch == 0) {
			return 0;
		} else if (ch == '\\') {
			char next = peekChar(fbuffer);
			if (next == 0) {
				return ch;
			} else if (next >= '0' && next <= '9') {
				size_t index = nextChar(fbuffer) - '0';
				if (index < strvec_Count(fbuffer->arguments)) {
					string* value = strvec_StringAt(fbuffer->arguments, index);
					if (value != NULL) {
						chstk_PushString(&fbuffer->charStack, value);
					}
				}
			} else if (next == '@') {
				nextChar(fbuffer);
				if (fbuffer->uniqueValue != NULL) {
					chstk_PushString(&fbuffer->charStack, fbuffer->uniqueValue);
				}
			} else {
				return ch;
			}
		} else {
			return ch;
		}
	}
}
