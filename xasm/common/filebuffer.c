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


// Private functions

static string*
createUniqueValue(void) {
    static uint32_t runId = 0;
    return str_CreateFormat("_%u", runId++);
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


// Public functions

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


extern void
fbuf_Init(SFileBuffer* fileBuffer, string* buffer, vec_t* arguments) {
	chstk_Init(&fileBuffer->charStack);
	fileBuffer->uniqueValue = createUniqueValue();
	fileBuffer->text = str_Copy(buffer);
	fileBuffer->index = 0;
	fileBuffer->arguments = arguments;
}


extern void
fbuf_Destroy(SFileBuffer* fileBuffer) {
	strvec_Free(fileBuffer->arguments);
	str_Free(fileBuffer->text);
	str_Free(fileBuffer->uniqueValue);
}


extern SFileBuffer*
fbuf_Create(string* buffer, vec_t* arguments) {
	SFileBuffer* fileBuffer = mem_Alloc(sizeof(SFileBuffer));
	fbuf_Init(fileBuffer, buffer, arguments);

	return fileBuffer;
}


extern void
fbuf_ShiftArguments(SFileBuffer* fbuffer, int32_t count) {
	while (count-- > 0 && strvec_Count(fbuffer->arguments) >= 2) {
		strvec_RemoveAt(fbuffer->arguments, 1);
	}
}


extern void
fbuf_Copy(SFileBuffer* dest, const SFileBuffer* source) {
	chstk_Copy(&dest->charStack, &source->charStack);
	dest->uniqueValue = str_Copy(source->uniqueValue);
	dest->text = str_Copy(source->text);
	dest->index = source->index;
	dest->arguments = strvec_Clone(source->arguments);
}

extern size_t
fbuf_SkipUnexpandedChars(SFileBuffer* fbuffer, size_t count) {
	size_t linesSkipped = 0;

	char ch;
	while ((ch = chstk_Pop(&fbuffer->charStack)) != 0) {
		if (ch == '\n')
			++linesSkipped;

		--count;
	}

	for (size_t index = 0; index < count; ++index) {
		if (str_CharAt(fbuffer->text + fbuffer->index, index) == '\n')
			++linesSkipped;
	}

	fbuffer->index += count;

	return linesSkipped;
}

extern void
fbuf_RenewUniqueValue(SFileBuffer* fbuffer) {
	str_Free(fbuffer->uniqueValue);
	fbuffer->uniqueValue = createUniqueValue();
}

extern void
fbuf_CopyUnexpandedContent(SFileBuffer* fbuffer, char* dest, size_t count) {
	for (int32_t i = fbuffer->charStack.count - 1; i >= 0; --i) {
		*dest++ = fbuffer->charStack.stack[i];
		--count;
	}

	memcpy(dest, fbuffer->text->data + fbuffer->index, count);
}

extern void
fbuf_UnputChar(SFileBuffer* fbuffer, char ch) {
	chstk_Push(&fbuffer->charStack, ch);
}

extern char
fbuf_GetUnexpandedChar(SFileBuffer* fbuffer, size_t index) {
	if (index < chstk_Count(&fbuffer->charStack)) {
		return chstk_PeekAt(&fbuffer->charStack, index);
	} else {
		index -= chstk_Count(&fbuffer->charStack);
	}
	index += fbuffer->index;
	if (index < (size_t) str_Length(fbuffer->text)) {
		return str_CharAt(fbuffer->text, index);
	} else {
		return 0;
	}
}
