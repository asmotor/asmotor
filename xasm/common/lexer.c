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
#include "lists.h"
#include "fmath.h"
#include "mem.h"
#include "strbuf.h"

#include "errors.h"
#include "lexer.h"
#include "lexer_constants.h"
#include "filestack.h"
#include "symbol.h"


static SLexerBuffer* g_currentBuffer;

/* Private functions */

INLINE void
copyCharStack(SCharStack* dest, const SCharStack* source) {
    memcpy(dest->stack, source->stack, source->count);
    dest->count = source->count;
}

INLINE void
copyBuffer(SLexerBuffer* dest, const SLexerBuffer* source) {
    copyCharStack(&dest->charStack, &source->charStack);
    dest->buffer = source->buffer;
    dest->index = source->index;
    dest->bufferSize = source->bufferSize;
    dest->atLineStart = source->atLineStart;
    dest->mode = source->mode;
}

INLINE size_t
charsAvailable(void) {
    return g_currentBuffer->bufferSize - g_currentBuffer->index + g_currentBuffer->charStack.count;
}

static size_t
appendAndFreeString(char* destination, string* str) {
    if (str != NULL) {
        size_t length = str_Length(str);
        memcpy(destination, str_String(str), length);
        str_Free(str);
        return length;
    }
    return 0;
}

static void
expandEscapeSequence(char** destination, char escapeSymbol) {
    if (escapeSymbol == '@' || (escapeSymbol >= '0' && escapeSymbol <= '9')) {
        *destination += appendAndFreeString(*destination, fstk_GetMacroArgValue(escapeSymbol));
    } else {
        *(*destination)++ = '\\';
        *(*destination)++ = escapeSymbol;
    }
}

static char*
expandStreamIncluding(char* destination, const char* stopChars, const char* sequenceBeginChars, char* (*next)(char*, char)) {
    for (;;) {
        char ch = lex_PeekChar(0);
        if (ch == 0 || ch == '\n') {
            break;
        }

        ch = lex_GetChar();
		assert(ch != 0);

        if (strchr(stopChars, ch) != NULL) {
            *destination++ = ch;
            break;
        }

        if (ch == '\\') {
            expandEscapeSequence(&destination, lex_GetChar());
        } else if (strchr(sequenceBeginChars, ch) != NULL) {
            *destination++ = ch;
            destination = next(destination, ch);
        } else {
            *destination++ = ch;
        }
    }

    *destination = 0;
    return destination;
}

static char*
expandStringIncluding(char* destination, char terminator);

static char*
expandExpressionIncluding(char* destination, char terminator) {
    return expandStreamIncluding(destination, "}", "\"'", expandStringIncluding);
}

static char*
expandStringIncludingSeveral(char* destination, const char* stopChars) {
    return expandStreamIncluding(destination, stopChars, "{", expandExpressionIncluding);
}

static char*
expandStringIncluding(char* destination, char terminator) {
	char stopChars[] = { terminator, 0 };
    return expandStringIncludingSeveral(destination, stopChars);
}

static bool
acceptString(void) {
    char ch = lex_GetChar();
    if (ch == '"' || ch == '\'') {
        expandStringIncluding(lex_Current.value.string, ch);
        lex_Current.token = T_STRING;
        lex_Current.length = strlen(lex_Current.value.string);
        if (lex_Current.length > 0)
            lex_Current.value.string[--lex_Current.length] = 0;

        return true;
    }
	lex_UnputChar(ch);
    return false;
}

static bool
skipUnimportantWhitespace(void) {
    bool didSkipCharacter = false;
    for (;;) {
        uint8_t ch = (uint8_t) lex_PeekChar(0);
        if (isspace(ch) && ch != '\n') {
            didSkipCharacter = true;
            lex_GetChar();
        } else {
            return didSkipCharacter;
        }
    }
}

static bool
acceptNext(bool lineStart);

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
isSymbolCharacter(char ch) {
	return isalnum(ch) ||  ch == '_' || ch == '#';
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
acceptLabel(void) {
	char ch = lex_GetChar();
	if (ch == '.' || isSymbolCharacter(ch)) {
		lex_Current.length = 1;
		lex_Current.value.string[0] = ch;
		if (acceptSymbolTail()) {
			lex_Current.token = T_LABEL;
			return true;
		}
		return false;
	}
	lex_UnputChar(ch);
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
acceptNumeric(int radix) {
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
		lex_Current.token = T_ID;
		return true;
	}

	uint32_t denominator = 1;
	uint32_t low = 0;
	bool dot = false;
	if (ch == '.') {
		dot = true;
		while ((binary = asciiToRadixBinary(ch = lex_GetChar(), radix)) != -1) {
			low = low * radix + binary;
			denominator = denominator * radix;
			lex_Current.value.string[lex_Current.length++] = ch;
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
acceptVariadic(void) {
	char ch = lex_GetChar();
	lex_UnputChar(ch);

	if (ch == '$') {
		return acceptNumeric(16);
	} else if (ch == '%') {
		return acceptNumeric(2);
	} else if (ch == '`') {
		return acceptGameboyLiteral();
	} else if (isdigit(ch)) {
		return acceptNumeric(10);
	}
	return false;
}

static bool
acceptNext(bool lineStart) {
	if (lineStart) {
		consumeComment(false, true);
		if (acceptLabel()) {
			return true;
		}
	}

    bool wasSpace = skipUnimportantWhitespace();
    lineStart &= !wasSpace;

    wasSpace = consumeComment(wasSpace, lineStart);
    lineStart &= !wasSpace;

	if (acceptVariadic()) {
		return true;
	}

    const SLexConstantsWord* constantWord = lex_ConstantsMatchWord();
    if (constantWord != NULL) {
		acceptSymbolIfLonger();
		return true;
    }
	
    return acceptString() || acceptChar();
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
isMacroArgument0Terminator(uint8_t ch) {
    return isspace(ch) || ch == ';';
}

static bool
stateMacroArgument0(void) {
    if (lex_MatchChar('.')) {
        int i = 0;

        while (!isMacroArgument0Terminator((unsigned char) lex_PeekChar(0))) {
            lex_Current.value.string[i++] = lex_GetChar();
        }
        lex_Current.value.string[i] = 0;
        lex_Current.token = T_MACROARG0;
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
    char* lastChar = expandStringIncludingSeveral(lex_Current.value.string, terminators);
    lex_Current.length = lastChar - lex_Current.value.string;
    if (lex_Current.length > 0 && strchr(terminators, lex_Current.value.string[lex_Current.length - 1]) != 0) {
        char terminator = lex_Current.value.string[lex_Current.length - 1];
        lex_Current.value.string[--lex_Current.length] = 0;
        lex_UnputChar(terminator);
    }
}

static void
getStringUntilTerminator(char terminator) {
    char* lastChar = expandStringIncluding(lex_Current.value.string, terminator);
    lex_Current.length = lastChar - lex_Current.value.string;
    if (lex_Current.value.string[lex_Current.length - 1] == terminator) {
        lex_Current.value.string[--lex_Current.length] = 0;
        lex_UnputChar(terminator);
    }
}

static bool
stateMacroArguments() {
    bool skippedSpace = false;
    while (isspace((unsigned char) lex_PeekChar(0)) && lex_PeekChar(0) != '\n') {
        lex_GetChar();
        skippedSpace = true;
    }

    if (lex_MatchChar(';') || (skippedSpace && lex_MatchChar('*'))) {
        while (lex_PeekChar(0) != '\n')
            lex_GetChar();
    }

    lex_Current.length = 0;

    if (lex_MatchChar('<')) {
        getStringUntilTerminator('>');
        lex_GetChar();
    } else {
        getStringUntilTerminators("\t ,;}");
    }

    if (lex_Current.length > 0) {
        char ch = lex_PeekChar(0);
        if (ch == '\n' || ch == ';' || ch == ' ' || ch == '\t') {
            trimTokenStringRight();
        }
        lex_Current.token = T_STRING;
        return true;
    } else if (lex_MatchChar('\n')) {
        g_currentBuffer->atLineStart = true;
        lex_Current.length = 1;
        lex_Current.token = T_LINEFEED;
        return true;
    } else if (lex_MatchChar(',')) {
        lex_Current.length = 1;
        lex_Current.token = T_COMMA;
        return true;
    } else if (lex_PeekChar(0) == '}') {
        lex_Current.length = 1;
        lex_Current.token = T_LINEFEED;
        return true;
    } else {
        lex_Current.token = T_NONE;
        return false;
    }
}

/* Public variables */

SLexerToken lex_Current;

/*	Public functions */

void
lex_UnputChar(char ch) {
    g_currentBuffer->charStack.stack[g_currentBuffer->charStack.count++] = ch;
}

char
lex_PeekChar(size_t index) {
    if (index < g_currentBuffer->charStack.count) {
        return g_currentBuffer->charStack.stack[g_currentBuffer->charStack.count - index - 1];
    } else {
        index -= g_currentBuffer->charStack.count;
    }
    index += g_currentBuffer->index;
    if (index < g_currentBuffer->bufferSize) {
        return g_currentBuffer->buffer[index];
    } else {
        return 0;
    }
}

string*
lex_PeekString(size_t length) {
    string_buffer* buf = strbuf_Create();

    for (size_t i = 0; i < length; ++i) {
        strbuf_AppendChar(buf, lex_PeekChar(i));
    }

    string* result = strbuf_String(buf);
    strbuf_Free(buf);

    return result;
}


char
lex_GetChar(void) {
    char r;

    if (g_currentBuffer->charStack.count > 0) {
        r = g_currentBuffer->charStack.stack[--(g_currentBuffer->charStack.count)];
    } else if (g_currentBuffer->index < g_currentBuffer->bufferSize) {
        r =  g_currentBuffer->buffer[g_currentBuffer->index++];
    } else {
        r = 0;
    }

    return r;
}

size_t
lex_GetZeroTerminatedString(char* dest, size_t actualCharacters) {
    size_t copiedLines = 0;
    for (size_t i = 0; i < actualCharacters; ++i) {
        char ch = lex_GetChar();
        *dest++ = ch;
        if (ch == '\n')
            copiedLines += 1;
    }
    *dest = 0;
    return copiedLines;
}

bool
lex_MatchChar(char ch) {
    if (lex_PeekChar(0) == ch) {
        lex_GetChar();
        return true;
    } else {
        return false;
    }
}

bool
lex_CompareNoCase(size_t index, const char* str, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        if (tolower(lex_PeekChar(index + i)) != tolower(str[i]))
            return false;
    }
    return true;
}

bool
lex_StartsWithNoCase(const char* str, size_t length) {
    return lex_CompareNoCase(0, str, length);
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
    size_t linesSkipped = 0;

    if (g_currentBuffer) {
        for (size_t i = 0; i < count; ++i) {
            char ch = lex_GetChar();
            if (ch == 0) {
                break;
            } else if (ch == '\n') {
                linesSkipped += 1;
            }
        }
    } else {
        internalerror("g_pCurrentBuffer not initialized");
    }

    return linesSkipped;
}

size_t
lex_SkipCurrentBuffer(void) {
    return lex_SkipBytes(charsAvailable());
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
    if (buffer) {
        if (buffer->buffer) {
            mem_Free(buffer->buffer);
        } else {
            internalerror("buf->pBufferStart not initialized");
        }
        mem_Free(buffer);
    } else {
        internalerror("Argument must not be NULL");
    }
}

SLexerBuffer*
lex_CreateMemoryBuffer(const char* memory, size_t size) {
    SLexerBuffer* lexerBuffer = (SLexerBuffer*) mem_Alloc(sizeof(SLexerBuffer));

    lexerBuffer->buffer = (char*) mem_Alloc(size);
    memcpy(lexerBuffer->buffer, memory, size);
    chstk_Init(&lexerBuffer->charStack);
    lexerBuffer->index = 0;
    lexerBuffer->bufferSize = size;
    lexerBuffer->atLineStart = true;
    lexerBuffer->mode = LEXER_MODE_NORMAL;
    return lexerBuffer;
}

SLexerBuffer*
lex_CreateFileBuffer(FILE* fileHandle, uint32_t* checkSum) {
    char* fileContent;

    SLexerBuffer* lexerBuffer = (SLexerBuffer*) mem_Alloc(sizeof(SLexerBuffer));
    memset(lexerBuffer, 0, sizeof(SLexerBuffer));

    size_t size = fsize(fileHandle);

    fileContent = (char*) mem_Alloc(size);
    size = fread(fileContent, sizeof(uint8_t), size, fileHandle);

    if (checkSum != NULL && opt_Current->enableDebugInfo)
        *checkSum = crc32((uint8_t*) fileContent, size);

    lexerBuffer->buffer = (char*) mem_Alloc(size + 1);
    char* dest = lexerBuffer->buffer;

    char* mem = fileContent;

    while (mem < fileContent + size) {
        if ((mem[0] == 10 && mem[1] == 13) || (mem[0] == 13 && mem[1] == 10)) {
            *dest++ = '\n';
            mem += 2;
        } else if (mem[0] == 10 || mem[0] == 13) {
            *dest++ = '\n';
            mem += 1;
        } else {
            *dest++ = *mem++;
        }
    }

    *dest++ = '\n';
    lexerBuffer->bufferSize = dest - lexerBuffer->buffer;
    lexerBuffer->atLineStart = true;

    mem_Free(fileContent);
    return lexerBuffer;
}

void
lex_Init(void) {
    lex_ConstantsInit();
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
