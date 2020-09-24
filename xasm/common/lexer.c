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
#include <stdlib.h>
#include <memory.h>
#include <ctype.h>
#include <string.h>

#include "util.h"
#include "crc32.h"
#include "file.h"
#include "fmath.h"
#include "lists.h"
#include "mem.h"
#include "strbuf.h"
#include "strcoll.h"

#include "errors.h"
#include "lexer.h"
#include "lexer_constants.h"
#include "filestack.h"
#include "symbol.h"


static SLexerBuffer* g_currentBuffer;

/* Private functions */

INLINE void
copyBuffer(SLexerBuffer* dest, const SLexerBuffer* source) {
	fbuf_Copy(&dest->fileBuffer, &source->fileBuffer);
	dest->atLineStart = source->atLineStart;
	dest->mode = source->mode;
}

INLINE char
getUnexpandedChar(size_t index) {
	return fbuf_GetUnexpandedChar(&g_currentBuffer->fileBuffer, index);
}

static bool 
acceptQuotedStringUntil(char terminator);

static bool
acceptStringInterpolation() {
	char ch;
	while ((ch = lex_GetChar()) != '}' || ch == 0) {
		lex_Current.value.string[lex_Current.length++] = ch;
		switch (ch) {
			case '\'':
			case '\"': {
				if (!acceptQuotedStringUntil(ch)) {
					return false;
				}
				break;
			}
			default:
				break;
		}
	}
	if (ch != 0) {
		lex_Current.value.string[lex_Current.length++] = ch;
		return true;
	}
	return false;
}

static bool
acceptQuotedStringUntil(char terminator) {
	char ch;
	while ((ch = lex_GetChar()) != terminator || ch == 0) {
		lex_Current.value.string[lex_Current.length++] = ch;
		switch (ch) {
			case '\\': {
				if ((ch = lex_GetChar()) == 0)
					return false;

				lex_Current.value.string[lex_Current.length++] = ch;
				break;
			}
			case '{': {
				if (!acceptStringInterpolation()) {
					return false;
				}
				break;
			}
			default:
				break;
		}
	}
	if (ch != 0) {
		lex_Current.value.string[lex_Current.length++] = ch;
		return true;
	}
	return false;
}

static bool
acceptString(void) {
	char ch = lex_GetChar();
	switch (ch) {
		case '"':
		case '\'': {
			lex_Current.length = 0;
			if (acceptQuotedStringUntil(ch)) {
				lex_Current.value.string[--lex_Current.length] = 0;
				lex_Current.token = T_STRING;
				return true;
			}
			break;
		}
		default:
			lex_UnputChar(ch);
	}
	return false;
}

static bool
skipUnimportantWhitespace(void) {
	bool charSkipped = false;
	char ch;
	while ((ch = lex_GetChar()) != '\n' && ch != 0 && isspace(ch)) {
		charSkipped = true;
	}
	if (ch != 0)
		lex_UnputChar(ch);
	return charSkipped;
}

static bool
acceptChar(void) {
	uint8_t ch = (uint8_t) lex_GetChar();

	if (ch == '\n') {
		g_currentBuffer->atLineStart = true;
	}

	lex_Current.length = 1;
	lex_Current.token = (EToken) ch;
	return ch != 0;
}

static bool
isLineEnd(char ch) {
	return ch == '\n' || ch == 0;
}

static bool
consumeComment(bool wasSpace, bool lineStart) {
	char ch = lex_GetChar();

	if (ch == ';' || (ch == '*' && (wasSpace || lineStart))) {
		lex_GetChar();
		while (!isLineEnd(ch = lex_GetChar())) {}
		wasSpace = false;
	}
	lex_UnputChar(ch);
	return wasSpace;
}

static bool
isStartSymbolCharacter(char ch) {
	return isalpha(ch) ||  ch == '_';
}

static bool
isSymbolCharacter(char ch) {
	return isStartSymbolCharacter(ch) || isdigit(ch) || ch == '#';
}

static bool
verifyLocalLabel(void) {
	for (size_t i = 0; i < lex_Current.length; ++i) {
		if (!isdigit(lex_Current.value.string[i])) {
			return err_Error(ERROR_ID_MALFORMED);
		}
	}
	return true;
}

static bool
acceptSymbolTail() {
	char ch;
	while (isSymbolCharacter(ch = lex_GetChar())) {
		lex_Current.value.string[lex_Current.length++] = ch;
	}
	if (ch == '$') {
		if (!verifyLocalLabel())
			return false;
		lex_Current.value.string[lex_Current.length++] = ch;
	} else {
		lex_UnputChar(ch);
	}

	ch = lex_Current.value.string[0];
	bool correct = (lex_Current.length >= 2) || (ch != '.');
	if (correct) {
		lex_Current.value.string[lex_Current.length] = 0;
		return true;
	} else {
		lex_UnputChar(ch);
		return false;
	}
}

static bool
acceptLabel(EToken token) {
	char ch = lex_GetChar();
	lex_Current.length = 0;
	if (ch == '.') {
		lex_Current.length = 1;
		lex_Current.value.string[0] = ch;
		ch = lex_GetChar();
	}
	lex_Current.value.string[lex_Current.length++] = ch;
	if (isStartSymbolCharacter(ch)) {
		if (acceptSymbolTail()) {
			lex_Current.token = token;
			return true;
		}
		return false;
	}
	while (lex_Current.length > 0) {
		lex_UnputChar(lex_Current.value.string[--lex_Current.length]);
	}
	return false;
}

static bool
acceptSymbolIfLonger(void) {
	size_t tokenLength = lex_Current.length;

	size_t index = 0;
	if (tokenLength >= 2 && lex_Current.value.string[index] == '.')
		++index;

	while (index < tokenLength && isSymbolCharacter(lex_Current.value.string[index])) {
		++index;
	}

	if (index == tokenLength) {
		if (acceptSymbolTail() && lex_Current.length > tokenLength) {
			lex_Current.token = T_ID;
			return true;
		}
	}

	lex_Current.length = tokenLength;
	return false;
}

static int
asciiToBinary(char ch) {
	if (isdigit(ch)) {
		return ch - '0';
	} if ((ch >= 'a') && (ch <= 'f')) {
		return ch - 'a' + 10;
	} else if ((ch >= 'A') && (ch <= 'F')) {
		return ch - 'A' + 10;
	}
	return -1;
}

static int
asciiToRadixBinary(char ch, int radix) {
	if (radix == 2) {
		for (int i = 0; i < 2; ++i) {
			if (ch == opt_Current->binaryLiteralCharacters[i])
				return i;
		}
	}
	int binary = asciiToBinary(ch);
	return binary < radix ? binary : -1;
}

static bool
acceptNumericAndLocalLabel(int radix, bool lineStart) {
	int binary;
	char ch;
	uint32_t high = 0;

	lex_Current.length = 0;
	while ((binary = asciiToRadixBinary(ch = lex_GetChar(), radix)) != -1) {
		high = high * radix + binary;
		lex_Current.value.string[lex_Current.length++] = ch;
	}

	if (radix == 10 && ch == '$') {
		if (!verifyLocalLabel())
			return false;
		lex_Current.value.string[lex_Current.length++] = ch;
		lex_Current.value.string[lex_Current.length] = 0;
		lex_Current.token = lineStart ? T_LABEL : T_ID;
		return true;
	}

	uint32_t denominator = 1;
	uint32_t low = 0;
	bool dot = false;
	if (ch == '.') {
		while ((binary = asciiToRadixBinary(ch = lex_GetChar(), radix)) != -1) {
			dot = true;
			low = low * radix + binary;
			denominator = denominator * radix;
			lex_Current.value.string[lex_Current.length++] = ch;
		}
		if (!dot) {
			lex_UnputChar(ch);
			ch = '.';
		}
	}

	if (ch == 'f') {
		lex_Current.token = T_FLOAT;
		lex_Current.value.floating = ((long double) high) + ((long double) low) / denominator;
		return true;
	}

	lex_UnputChar(ch);

	if (dot) {
		lex_Current.token = T_NUMBER;
		lex_Current.value.integer = high * 65536 + imuldiv(low, 65536, denominator);
	} else {
		lex_Current.token = T_NUMBER;
		lex_Current.value.integer = high;
	}

	return true;
}


static int
gameboyCharToInt(char ch) {
	for (uint32_t i = 0; i <= 3; ++i) {
		if (opt_Current->gameboyLiteralCharacters[i] == ch)
			return i;
	}

	return -1;
}


static bool 
acceptGameboyLiteral() {
	uint32_t result = 0;
	char ch = 0;
	int value;

	while ((value = gameboyCharToInt(ch = lex_GetChar())) != -1) {
		result = result * 2 + ((value & 1u) << 8u) + ((value & 2u) >> 1u);
	}

	lex_Current.value.integer = result;
	lex_Current.token = T_NUMBER;

	return true;
}


static bool
acceptVariadic(bool lineStart) {
	char ch = lex_GetChar();

	if (ch == '$') {
		return acceptNumericAndLocalLabel(16, lineStart);
	} else if (ch == '%') {
		return acceptNumericAndLocalLabel(2, lineStart);
	} else if (ch == '`') {
		return acceptGameboyLiteral();
	}
	
	lex_UnputChar(ch);
	if (isdigit(ch)) {
		return acceptNumericAndLocalLabel(10, lineStart);
	}

	return false;
}

static bool
acceptNext(bool lineStart) {
	if (lineStart) {
		consumeComment(false, true);
		if (acceptLabel(T_LABEL)) {
			if (lex_ConstantsMatchTokenString() != NULL) {
				err_Warn(WARN_SYMBOL_WITH_RESERVED_NAME);
				lex_Current.token = T_LABEL;
			}
			return true;
		}
	}

	bool wasSpace = skipUnimportantWhitespace();
	lineStart &= !wasSpace;

	wasSpace = consumeComment(wasSpace, lineStart);
	lineStart &= !wasSpace;

	if (acceptVariadic(lineStart)) {
		return true;
	}

	const SLexConstantsWord* constantWord = lex_ConstantsMatchWord();
	if (constantWord != NULL) {
		acceptSymbolIfLonger();
		return true;
	} else if (acceptLabel(T_ID)) {
		return true;
	}
	
	return acceptString() || acceptChar();
}

static bool
matchChar(char match) {
	char ch = lex_GetChar();
	if (ch == match) {
		return true;
	}
	
	lex_UnputChar(ch);
	return false;
}

static bool
stateNormal() {
	bool lineStart = g_currentBuffer->atLineStart;
	g_currentBuffer->atLineStart = false;

	for (;;) {
		if (acceptNext(lineStart)) {
			return true;
		} else {
			if (fstk_EndCurrentBuffer()) {
				lineStart = g_currentBuffer->atLineStart;
				g_currentBuffer->atLineStart = false;
			} else {
				lex_Current.token = T_NONE;
				return false;
			}
		}
	}
}

static bool
isMacroArgument0Terminator(char ch) {
	return isspace(ch) || ch == ';';
}

static bool
stateMacroArgument0(void) {
	if (matchChar('.')) {
		int i = 0;
		char ch;

		while (!isMacroArgument0Terminator(ch = lex_GetChar())) {
			lex_Current.value.string[i++] = ch;
		}
		lex_Current.value.string[i] = 0;
		lex_Current.token = T_MACROARG0;
		lex_UnputChar(ch);
		return true;
	}

	return false;
}

static void
trimTokenStringRight() {
	char* asterisk = strrchr(lex_Current.value.string, '*');
	if (asterisk != NULL && isspace(*(asterisk - 1))) {
		*asterisk = 0;
		lex_Current.length = asterisk - lex_Current.value.string;
	}

	while (lex_Current.value.string[lex_Current.length - 1] == ' ') {
		lex_Current.value.string[--lex_Current.length] = 0;
	}

}

static void
getStringUntilTerminators(const char* terminators) {
	char ch;
	while (strchr(terminators, ch = lex_GetChar()) == NULL) {
		lex_Current.value.string[lex_Current.length++] = ch;
	}
	lex_Current.value.string[lex_Current.length] = 0;
	lex_UnputChar(ch);
}

static bool
stateMacroArguments() {
	consumeComment(skipUnimportantWhitespace(), false);

	lex_Current.length = 0;

	if (matchChar('<')) {
		getStringUntilTerminators("\n>");
		lex_GetChar();
	} else {
		getStringUntilTerminators("\n\t ,;}");
	}

	if (lex_Current.length > 0) {
		char ch = lex_GetChar();
		if (ch == '\n' || ch == ';' || ch == ' ' || ch == '\t') {
			trimTokenStringRight();
		}
		lex_Current.token = T_STRING;
		lex_UnputChar(ch);
		return true;
	} else if (matchChar('\n')) {
		g_currentBuffer->atLineStart = true;
		lex_Current.length = 1;
		lex_Current.token = T_LINEFEED;
		return true;
	} else if (matchChar(',')) {
		lex_Current.length = 1;
		lex_Current.token = T_COMMA;
		return true;
	} else {
		char ch = lex_GetChar();
		lex_UnputChar(ch);
		if (ch == '}') {
			lex_Current.length = 1;
			lex_Current.token = T_LINEFEED;
			return true;
		} else {
			lex_Current.token = T_NONE;
			return false;
		}
	}
}

/* Public variables */

SLexerToken lex_Current;

/*	Public functions */

void
lex_UnputChar(char ch) {
	fbuf_UnputChar(&g_currentBuffer->fileBuffer, ch);
}

extern char
lex_GetChar(void) {
	return fbuf_GetChar(&g_currentBuffer->fileBuffer);
}

extern void
lex_CopyUnexpandedContent(char* dest, size_t count) {
	fbuf_CopyUnexpandedContent(&g_currentBuffer->fileBuffer, dest, count);
}

void
lex_Bookmark(SLexerBookmark* bookmark) {
	copyBuffer(&bookmark->Buffer, g_currentBuffer);
	bookmark->Token = lex_Current;
}

void
lex_Goto(SLexerBookmark* bookmark) {
	copyBuffer(g_currentBuffer, &bookmark->Buffer);
	lex_Current = bookmark->Token;
}

size_t
lex_SkipBytes(size_t count) {
	return fbuf_SkipUnexpandedChars(&g_currentBuffer->fileBuffer, count);
}

void
lex_UnputStringLength(const char* str, size_t length) {
	str += length;
	for (size_t i = 0; i < length; ++i) {
		lex_UnputChar(*(--str));
	}
}

void
lex_UnputString(const char* str) {
	lex_UnputStringLength(str, strlen(str));
}

void
lex_SetBuffer(SLexerBuffer* buffer) {
	assert (buffer != NULL);
	g_currentBuffer = buffer;
}

void
lex_SetMode(ELexerMode mode) {
	assert (g_currentBuffer != NULL);
	g_currentBuffer->mode = mode;
}

void
lex_FreeBuffer(SLexerBuffer* buffer) {
	if (buffer != NULL) {
		fbuf_Destroy(&buffer->fileBuffer);
		mem_Free(buffer);
	} else {
		internalerror("Argument must not be NULL");
	}
}

SLexerBuffer*
lex_CreateBookmarkBuffer(SLexerBookmark* bookmark) {
	SLexerBuffer* lexerBuffer = (SLexerBuffer*) mem_Alloc(sizeof(SLexerBuffer));
	copyBuffer(lexerBuffer, &bookmark->Buffer);
	return lexerBuffer;
}

SLexerBuffer*
lex_CreateMemoryBuffer(string* memory, vec_t* arguments) {
	SLexerBuffer* lexerBuffer = (SLexerBuffer*) mem_Alloc(sizeof(SLexerBuffer));

	fbuf_Init(&lexerBuffer->fileBuffer, memory, arguments);
	lexerBuffer->atLineStart = true;
	lexerBuffer->mode = LEXER_MODE_NORMAL;
	return lexerBuffer;
}

SLexerBuffer*
lex_CreateFileBuffer(FILE* fileHandle, uint32_t* checkSum) {
	SLexerBuffer* lexerBuffer = (SLexerBuffer*) mem_Alloc(sizeof(SLexerBuffer));
	memset(lexerBuffer, 0, sizeof(SLexerBuffer));

	size_t size = fsize(fileHandle);
	string* fileContent = str_ReadFile(fileHandle, size);

	if (checkSum != NULL && opt_Current->enableDebugInfo)
		*checkSum = crc32((const uint8_t *)str_String(fileContent), size);

	fbuf_Init(&lexerBuffer->fileBuffer, fileContent, NULL);
	lexerBuffer->atLineStart = true;
	lexerBuffer->mode = LEXER_MODE_NORMAL;

	str_Free(fileContent);

	return lexerBuffer;
}

void
lex_Init(void) {
	lex_ConstantsInit();
}

bool
lex_GetNextDirective(void) {
	for (;;) {
		char ch;
		while ((ch = lex_GetChar()) != '\n') {
		}
		while (strchr(":\t ", (ch = lex_GetChar())) != NULL) {
		}
		lex_Current.length = 0;
		while (isalpha(ch = lex_GetChar())) {
			lex_Current.value.string[lex_Current.length++] = ch;
		}
		lex_Current.value.string[lex_Current.length] = 0;
		lex_UnputChar(ch);
		if (lex_ConstantsMatchTokenString() != NULL)
			return true;
	}
}

bool
lex_GetNextDirectiveUnexpanded(size_t* index) {
	for (;;) {
		char ch;
		while ((ch = getUnexpandedChar(*index)) != '\n') {
			*index += 1;
		}
		*index += 1;
		while (strchr(":\t ", (ch = getUnexpandedChar(*index))) != NULL) {
			*index += 1;
		}
		lex_Current.length = 0;
		while (isalpha(ch = getUnexpandedChar(*index))) {
			lex_Current.value.string[lex_Current.length++] = ch;
			*index += 1;
		}
		lex_Current.value.string[lex_Current.length] = 0;
		if (lex_ConstantsMatchTokenString() != NULL)
			return true;
	}
}

bool
lex_GetNextToken(void) {
	switch (g_currentBuffer->mode) {
		case LEXER_MODE_NORMAL: {
			return stateNormal();
		}
		case LEXER_MODE_MACRO_ARGUMENT0: {
			g_currentBuffer->mode = LEXER_MODE_MACRO_ARGUMENT;

			if (stateMacroArgument0())
				return true;
		}
		// fall through
		case LEXER_MODE_MACRO_ARGUMENT: {
			return stateMacroArguments();
		}
	}

	internalerror("Abnormal error encountered");
	return 0;
}

extern string*
lex_TokenString(void) {
	return str_CreateLength(lex_Current.value.string, lex_Current.length);
}
