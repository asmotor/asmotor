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

#include "str.h"

#include "lexer_buffer.h"


// Private functions

static string*
createUniqueValue(void) {
    static uint32_t runId = 0;
    return str_CreateFormat("_%u", runId++);
}


static char
nextChar(SLexerBuffer* buffer) {
	char ch = chstk_Pop(&buffer->charStack);
	if (ch != 0) {
		return ch;
	}

	if (buffer->index < str_Length(buffer->text)) {
		return str_CharAt(buffer->text, buffer->index++);
	}

	return 0;
}


static char
peekChar(SLexerBuffer* buffer) {
	char ch = chstk_Peek(&buffer->charStack);
	if (ch != 0) {
		return ch;
	}

	if (buffer->index < str_Length(buffer->text)) {
		return str_CharAt(buffer->text, buffer->index);
	}

	return 0;
}


// Public functions

extern char
lexbuf_GetChar(SLexerBuffer* buffer) {
	for (;;) {
		char ch = nextChar(buffer);
		if (ch == 0) {
			return 0;
		} else if (ch == '\\') {
			char next = peekChar(buffer);
			if (next >= '0' && next <= '9') {
				size_t index = nextChar(buffer) - '0';
				if (index < strvec_Count(buffer->arguments)) {
					string* value = strvec_StringAt(buffer->arguments, index);
					if (value != NULL) {
						chstk_PushString(&buffer->charStack, value);
					}
					str_Free(value);
				}
			} else if (next == '@') {
				nextChar(buffer);
				if (buffer->uniqueValue != NULL) {
					chstk_PushString(&buffer->charStack, buffer->uniqueValue);
				}
			} else {
				return ch;
			}
			return nextChar(buffer);
		} else {
			return ch;
		}
	}
}


extern void
lexbuf_Init(SLexerBuffer* buffer, string* name, string* content, vec_t* arguments) {
	assert (arguments != NULL);

	chstk_Init(&buffer->charStack);
	buffer->name = str_Copy(name);
	buffer->uniqueValue = createUniqueValue();
	buffer->text = str_Copy(content);
	buffer->index = 0;
	buffer->arguments = arguments;
}


extern void
lexbuf_Destroy(SLexerBuffer* buffer) {
	str_Free(buffer->name);
	str_Free(buffer->text);
	str_Free(buffer->uniqueValue);
}


extern void
lexbuf_ShiftArguments(SLexerBuffer* buffer, int32_t count) {
	while (count-- > 0 && strvec_Count(buffer->arguments) >= 2) {
		strvec_RemoveAt(buffer->arguments, 1);
	}
}


extern void
lexbuf_Copy(SLexerBuffer* dest, const SLexerBuffer* source) {
	chstk_Copy(&dest->charStack, &source->charStack);
	dest->name = str_Copy(source->name);
	dest->uniqueValue = str_Copy(source->uniqueValue);
	dest->text = str_Copy(source->text);
	dest->index = source->index;
	dest->arguments = source->arguments;
}


extern void
lexbuf_ContinueFrom(SLexerBuffer* dest, const SLexerBuffer* source) {
	chstk_Copy(&dest->charStack, &source->charStack);
	dest->name = str_Copy(source->name);
	dest->text = str_Copy(source->text);
	dest->index = source->index;
	dest->arguments = source->arguments;
}


extern size_t
lexbuf_SkipUnexpandedChars(SLexerBuffer* buffer, size_t count) {
	size_t linesSkipped = 0;

	char ch;
	while ((ch = chstk_Pop(&buffer->charStack)) != 0) {
		if (ch == '\n')
			++linesSkipped;

		--count;
	}

	for (size_t index = 0; index < count; ++index) {
		if (str_CharAt(buffer->text, buffer->index + index) == '\n')
			++linesSkipped;
	}

	buffer->index += count;

	return linesSkipped;
}


extern void
lexbuf_RenewUniqueValue(SLexerBuffer* buffer) {
	str_Free(buffer->uniqueValue);
	buffer->uniqueValue = createUniqueValue();
}


extern void
lexbuf_CopyUnexpandedContent(SLexerBuffer* buffer, char* dest, size_t count) {
	for (int32_t i = (int32_t) buffer->charStack.count - 1; i >= 0; --i) {
		*dest++ = buffer->charStack.stack[i];
		--count;
	}

	memcpy(dest, buffer->text->data + buffer->index, count);
}


extern void
lexbuf_UnputChar(SLexerBuffer* buffer, char ch) {
	chstk_Push(&buffer->charStack, ch);
}


extern char
lexbuf_GetUnexpandedChar(SLexerBuffer* buffer, size_t index) {
	if (index < chstk_Count(&buffer->charStack)) {
		return chstk_PeekAt(&buffer->charStack, index);
	} else {
		index -= chstk_Count(&buffer->charStack);
	}
	index += buffer->index;
	if (index < (size_t) str_Length(buffer->text)) {
		return str_CharAt(buffer->text, index);
	} else {
		return 0;
	}
}
