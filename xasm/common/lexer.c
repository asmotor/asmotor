/*  Copyright 2008-2021 Carsten Elton Sorensen and contributors

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
#include "lexer_context.h"
#include "symbol.h"


/* Private functions */

INLINE char
getUnexpandedChar(size_t index) {
	return lexbuf_GetUnexpandedChar(&lex_Context->buffer, index);
}

static bool 
acceptQuotedStringUntil(char terminator);

static bool
acceptStringInterpolation() {
	char ch;
	while ((ch = lex_GetChar()) != '}' || ch == 0) {
		lex_Context->token.value.string[lex_Context->token.length++] = ch;
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
		lex_Context->token.value.string[lex_Context->token.length++] = ch;
		return true;
	}
	return false;
}

static bool
acceptQuotedStringUntil(char terminator) {
	char ch;
	while ((ch = lex_GetChar()) != terminator || ch == 0) {
		lex_Context->token.value.string[lex_Context->token.length++] = ch;
		switch (ch) {
			case '\\': {
				if ((ch = lex_GetChar()) == 0)
					return false;

				lex_Context->token.value.string[lex_Context->token.length++] = ch;
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
		lex_Context->token.value.string[lex_Context->token.length++] = ch;
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
			lex_Context->token.length = 0;
			if (acceptQuotedStringUntil(ch)) {
				lex_Context->token.value.string[--lex_Context->token.length] = 0;
				lex_Context->token.id = T_STRING;
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
		lex_Context->atLineStart = true;
	}

	lex_Context->token.length = 1;
	lex_Context->token.id = (EToken) ch;
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
	for (size_t i = 0; i < lex_Context->token.length; ++i) {
		if (!isdigit(lex_Context->token.value.string[i])) {
			return err_Error(ERROR_ID_MALFORMED);
		}
	}
	return true;
}

static bool
acceptSymbolTail() {
	char ch;
	while (isSymbolCharacter(ch = lex_GetChar())) {
		lex_Context->token.value.string[lex_Context->token.length++] = ch;
	}
	if (ch == '$') {
		if (!verifyLocalLabel())
			return false;
		lex_Context->token.value.string[lex_Context->token.length++] = ch;
	} else {
		lex_UnputChar(ch);
	}

	ch = lex_Context->token.value.string[0];
	bool correct = (lex_Context->token.length >= 2) || (ch != '.');
	if (correct) {
		lex_Context->token.value.string[lex_Context->token.length] = 0;
		return true;
	} else {
		lex_UnputChar(ch);
		return false;
	}
}

static bool
acceptLabel(EToken token) {
	char ch = lex_GetChar();
	lex_Context->token.length = 0;
	if (ch == '.') {
		lex_Context->token.length = 1;
		lex_Context->token.value.string[0] = ch;
		ch = lex_GetChar();
	}
	lex_Context->token.value.string[lex_Context->token.length++] = ch;
	if (isStartSymbolCharacter(ch)) {
		if (acceptSymbolTail()) {
			lex_Context->token.id = token;
			return true;
		}
		return false;
	}
	while (lex_Context->token.length > 0) {
		lex_UnputChar(lex_Context->token.value.string[--lex_Context->token.length]);
	}
	return false;
}

static bool
acceptSymbolIfLonger(void) {
	size_t tokenLength = lex_Context->token.length;

	size_t index = 0;
	if (tokenLength >= 2 && lex_Context->token.value.string[index] == '.')
		++index;

	while (index < tokenLength && isSymbolCharacter(lex_Context->token.value.string[index])) {
		++index;
	}

	if (index == tokenLength) {
		if (acceptSymbolTail() && lex_Context->token.length > tokenLength) {
			lex_Context->token.id = T_ID;
			return true;
		}
	}

	lex_Context->token.length = tokenLength;
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

	lex_Context->token.length = 0;
	while ((binary = asciiToRadixBinary(ch = lex_GetChar(), radix)) != -1) {
		high = high * radix + binary;
		lex_Context->token.value.string[lex_Context->token.length++] = ch;
	}

	if (radix == 10 && ch == '$') {
		if (!verifyLocalLabel())
			return false;
		lex_Context->token.value.string[lex_Context->token.length++] = ch;
		lex_Context->token.value.string[lex_Context->token.length] = 0;
		lex_Context->token.id = lineStart ? T_LABEL : T_ID;
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
			lex_Context->token.value.string[lex_Context->token.length++] = ch;
		}
		if (!dot) {
			lex_UnputChar(ch);
			ch = '.';
		}
	}

	if (ch == 'f') {
		lex_Context->token.id = T_FLOAT;
		lex_Context->token.value.floating = ((long double) high) + ((long double) low) / denominator;
		return true;
	}

	lex_UnputChar(ch);

	if (dot) {
		lex_Context->token.id = T_NUMBER;
		lex_Context->token.value.integer = high * 65536 + imuldiv(low, 65536, denominator);
	} else {
		lex_Context->token.id = T_NUMBER;
		lex_Context->token.value.integer = high;
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

	lex_Context->token.value.integer = result;
	lex_Context->token.id = T_NUMBER;

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
	bool lineStart = lex_Context->atLineStart;
	lex_Context->atLineStart = false;

	for (;;) {
		if (acceptNext(lineStart)) {
			return true;
		} else {
			if (lexctx_EndCurrentBuffer()) {
				lineStart = lex_Context->atLineStart;
				lex_Context->atLineStart = false;
			} else {
				lex_Context->token.id = T_POP_END;
				return true;
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
		char ch;
		lex_Context->token.length = 0;
		while (!isMacroArgument0Terminator(ch = lex_GetChar())) {
			lex_Context->token.value.string[lex_Context->token.length++] = ch;
		}
		lex_Context->token.value.string[lex_Context->token.length] = 0;
		lex_Context->token.id = T_MACROARG0;
		lex_UnputChar(ch);
		return true;
	}

	return false;
}

static void
trimTokenStringRight() {
	char* asterisk = strrchr(lex_Context->token.value.string, '*');
	if (asterisk != NULL && isspace(*(asterisk - 1))) {
		*asterisk = 0;
		lex_Context->token.length = asterisk - lex_Context->token.value.string;
	}

	while (lex_Context->token.value.string[lex_Context->token.length - 1] == ' ') {
		lex_Context->token.value.string[--lex_Context->token.length] = 0;
	}

}

static void
getStringUntilTerminators(const char* terminators) {
	char ch;
	while (strchr(terminators, ch = lex_GetChar()) == NULL) {
		lex_Context->token.value.string[lex_Context->token.length++] = ch;
	}
	lex_Context->token.value.string[lex_Context->token.length] = 0;
	lex_UnputChar(ch);
}

static bool
stateMacroArguments() {
	consumeComment(skipUnimportantWhitespace(), false);

	if (matchChar('\n')) {
		lex_Context->atLineStart = true;
		lex_Context->token.length = 1;
		lex_Context->token.id = T_LINEFEED;
		return true;
	}

	if (matchChar(',')) {
		lex_Context->token.length = 1;
		lex_Context->token.id = T_COMMA;
		return true;
	}

	if (matchChar('<')) {
		lex_Context->token.length = 0;
		getStringUntilTerminators("\n>");
		matchChar('>');
		lex_Context->token.id = T_STRING;
		return true;
	}

	if (matchChar('}')) {
		lex_UnputChar('}');
		lex_Context->token.length = 1;
		lex_Context->token.id = T_LINEFEED;
		return true;
	}

	lex_Context->token.length = 0;
	bool wasSpace = false;
	for (;;) {
		char ch = lex_GetChar();
		if (ch != 0 && strchr("\n,;}", ch) != NULL) {
			lex_UnputChar(ch);
			break;
		}

		if (ch == '*' && wasSpace) {
			lex_UnputString(" *");
			break;
		}

		lex_Context->token.value.string[lex_Context->token.length++] = ch;
		wasSpace = strchr("\t ", ch) != NULL && ch != 0;
	}

	if (lex_Context->token.length > 0) {
		char ch = lex_GetChar();
		if (strchr("\n\t ;", ch) != NULL && ch != 0) {
			trimTokenStringRight();
		}
		lex_Context->token.id = T_STRING;
		lex_UnputChar(ch);
		return true;
	} else {
		lex_Context->token.id = T_NONE;
		return false;
	}
}


static bool
skipToNextLine(void) {
	char ch = lex_GetChar();

	if (ch == 0)
		return false;

	while (!isLineEnd(ch) && ch != 0) {
		ch = lex_GetChar();
	}

	return true;
}

static bool
skipToNextLineIndexed(size_t* index) {
	for (;;) {
		char ch = getUnexpandedChar(*index);
		*index += 1;
		if (isLineEnd(ch) || ch == 0)
			return ch != 0;
	}
}

static bool
charIs(char* candidates) {
	char ch = lex_GetChar();
	lex_UnputChar(ch);
	return ch != 0 && strchr(candidates, ch) != NULL;
}

static bool
charIsIndexed(size_t* index, char* candidates) {
	char ch = getUnexpandedChar(*index);
	return ch != 0 && strchr(candidates, ch) != NULL;
}

static void
skipLabel(void) {
	char ch = lex_GetChar();
	while (strchr(" \t\n",ch) == NULL && ch != 0) {
		ch = lex_GetChar();
	}
	lex_UnputChar(ch);
}

static void
skipLabelIndexed(size_t* index) {
	for (;;) {
		char ch = getUnexpandedChar(*index);
		if (strchr(" \t",ch) != NULL || ch == 0)
			return;
		*index += 1;
	}
}

static bool
skipWhiteSpace(void) {
	char ch = lex_GetChar();
	while (ch != 0 && strchr("\t ", ch) != NULL) {
		ch = lex_GetChar();
		if (ch == '*') {
			return true;
		}
	}
	if (ch == ';')
		return true;
	lex_UnputChar(ch);
	return false;
}

static bool
skipWhiteSpaceIndexed(size_t* index) {
	char ch = getUnexpandedChar(*index);
	while (ch != 0 && strchr("\t ", ch) != NULL) {
		*index += 1;
		ch = getUnexpandedChar(*index);
		if (ch == '*') {
			return true;
		}
	}
	return (ch == ';') || (ch == 0);
}

/*	Public functions */

void
lex_UnputChar(char ch) {
	lexbuf_UnputChar(&lex_Context->buffer, ch);
}

extern char
lex_GetChar(void) {
	return lexbuf_GetChar(&lex_Context->buffer);
}

extern void
lex_CopyUnexpandedContent(char* dest, size_t count) {
	lexbuf_CopyUnexpandedContent(&lex_Context->buffer, dest, count);
}

void
lex_Bookmark(SLexerContext* bookmark) {
	lexctx_ShallowCopy(bookmark, lex_Context);
}

void
lex_Goto(SLexerContext* bookmark) {
	lexctx_ShallowCopy(lex_Context, bookmark);
}

size_t
lex_SkipBytes(size_t count) {
	return lexbuf_SkipUnexpandedChars(&lex_Context->buffer, count);
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
lex_SetMode(ELexerMode mode) {
	assert (lex_Context != NULL);
	lex_Context->mode = mode;
}

bool
lex_Init(string* filename) {
	lex_ConstantsInit();
	return lexctx_ContextInit(filename);
}

bool
lex_GetNextDirective(void) {
	for (;;) {
		if (!skipToNextLine())
			return false;

		lex_Context->lineNumber += 1;
		if (charIs(";*"))
			continue;
		skipLabel();
		if (skipWhiteSpace())
			continue;

		lex_Context->token.length = 0;
		char ch;
		while (isalpha(ch = lex_GetChar())) {
			lex_Context->token.value.string[lex_Context->token.length++] = ch;
		}
		lex_Context->token.value.string[lex_Context->token.length] = 0;
		lex_UnputChar(ch);
		if (lex_ConstantsMatchTokenString() != NULL)
			return true;
	}
}

bool
lex_GetNextDirectiveUnexpanded(size_t* index) {
	for (;;) {
		if (!skipToNextLineIndexed(index))
			return false;
			
		if (charIsIndexed(index, ";*"))
			continue;
		skipLabelIndexed(index);
		if (skipWhiteSpaceIndexed(index))
			continue;

		lex_Context->token.length = 0;
		char ch;
		while (isalpha(ch = getUnexpandedChar(*index))) {
			lex_Context->token.value.string[lex_Context->token.length++] = ch;
			*index += 1;
		}
		lex_Context->token.value.string[lex_Context->token.length] = 0;
		if (lex_ConstantsMatchTokenString() != NULL)
			return true;
	}
}

bool
lex_GetNextToken(void) {
	switch (lex_Context->mode) {
		case LEXER_MODE_NORMAL: {
			return stateNormal();
		}
		case LEXER_MODE_MACRO_ARGUMENT0: {
			lex_Context->mode = LEXER_MODE_MACRO_ARGUMENT;

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
	return str_CreateLength(lex_Context->token.value.string, lex_Context->token.length);
}

extern void
lex_Exit(void) {
	lex_ConstantsExit();
	lexctx_Cleanup();
}
