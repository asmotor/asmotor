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

#include "asmotor.h"
#include "crc32.h"
#include "file.h"
#include "lists.h"
#include "mem.h"
#include "strbuf.h"

#include "errors.h"
#include "lexer.h"
#include "lexer_constants.h"
#include "lexer_variadics.h"
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
    dest->state = source->state;
}

INLINE void
unputChar(char ch) {
    g_currentBuffer->charStack.stack[g_currentBuffer->charStack.count++] = ch;
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
skip(size_t count) {
    if (g_currentBuffer->charStack.count > 0) {
        if (count >= g_currentBuffer->charStack.count) {
            count -= g_currentBuffer->charStack.count;
            g_currentBuffer->charStack.count = 0;
        } else {
            g_currentBuffer->charStack.count -= count;
            return;
        }
    }
    g_currentBuffer->index += count;
    if (g_currentBuffer->index > g_currentBuffer->bufferSize)
        g_currentBuffer->index = g_currentBuffer->bufferSize;
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
    char ch = lex_PeekChar(0);
    if (ch == '"' || ch == '\'') {
        lex_GetChar();
        expandStringIncluding(lex_Current.value.string, ch);
        lex_Current.length = strlen(lex_Current.value.string);
        lex_Current.value.string[--lex_Current.length] = 0;
        lex_Current.token = T_STRING;
        return true;
    }
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
atBufferEnd(void) {
    return lex_PeekChar(0) == 0;
}

static bool 
getMatches(bool lineStart, size_t* variadicLength, const SVariadicWordDefinition** variadicWord, size_t* constantLength, const SLexConstantsWord** constantWord) {
    if (atBufferEnd())
        return false;

    lex_VariadicMatchString(charsAvailable(), variadicLength, variadicWord);
    bool doNotTryConstantWord = ((*variadicWord) != NULL && (*variadicWord)->token == T_ID && lineStart && lex_PeekChar(*variadicLength) == ':');

    if (doNotTryConstantWord) {
        *constantLength = 0;
        *constantWord = NULL;
    } else {
        lex_ConstantsMatchWord(charsAvailable(), constantLength, constantWord);
    }
    return true;
}

static bool
matchNext(bool lineStart);

static bool
acceptVariadic(size_t variadicLength, const SVariadicWordDefinition* variadicWord, bool lineStart) {
    lex_Current.length = variadicLength;
    if (variadicWord->callback && !variadicWord->callback(lex_Current.length)) {
        return matchNext(lineStart);
    }

    if (variadicWord->token == T_ID && lineStart) {
        skip(lex_Current.length);
        lex_Current.token = T_LABEL;
        return true;
    } else {
        skip(lex_Current.length);
        lex_Current.token = variadicWord->token;
        return true;
    }

}

static bool
acceptConstantWord(size_t constantLength, const SLexConstantsWord* constantWord) {
    lex_Current.length = constantLength;
    lex_GetChars(lex_Current.value.string, lex_Current.length);
    lex_Current.token = constantWord->token;
    return true;
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
consumeComment(bool wasSpace, bool lineStart) {
    char ch = lex_PeekChar(0);

    if (ch == ';' || (ch == '*' && (wasSpace || lineStart))) {
        lex_GetChar();
        while (true) {
            ch = lex_PeekChar(0);
            if (ch == '\n' || ch == 0)
                break;
            lex_GetChar();
        }
        return true;
    }
    return wasSpace;
}

static bool
matchNext(bool lineStart) {
    bool wasSpace = skipUnimportantWhitespace();
    lineStart &= !wasSpace;

    wasSpace = consumeComment(wasSpace, lineStart);
    lineStart &= !wasSpace;

    size_t variadicLength;
    const SVariadicWordDefinition* variadicWord;

    size_t constantLength;
    const SLexConstantsWord* constantWord;

    if (!getMatches(lineStart, &variadicLength, &variadicWord, &constantLength, &constantWord))
        return false;

    if (constantWord != NULL && constantLength >= variadicLength) {
        return acceptConstantWord(constantLength, constantWord);
    } else if (variadicLength > 0) {
        return acceptVariadic(variadicLength, variadicWord, lineStart);
    } else {
        return acceptString() || acceptChar();
    }
}

static bool
stateNormal() {
    bool lineStart = g_currentBuffer->atLineStart;
    g_currentBuffer->atLineStart = false;

    for (;;) {
        if (matchNext(lineStart)) {
            return true;
        } else {
            if (fstk_ProcessNextBuffer()) {
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
        unputChar(terminator);
    }
}

static void
getStringUntilTerminator(char terminator) {
    char* lastChar = expandStringIncluding(lex_Current.value.string, terminator);
    lex_Current.length = lastChar - lex_Current.value.string;
    if (lex_Current.value.string[lex_Current.length - 1] == terminator) {
        lex_Current.value.string[--lex_Current.length] = 0;
        unputChar(terminator);
    }
}

static bool
stateMacroArguments() {
    if (lex_MatchChar(';')) {
        while (lex_PeekChar(0) != '\n')
            lex_GetChar();
    }

    while (isspace((unsigned char) lex_PeekChar(0)) && lex_PeekChar(0) != '\n') {
        lex_GetChar();
    }

    lex_Current.length = 0;

    if (lex_MatchChar('<')) {
        getStringUntilTerminator('>');
        lex_GetChar();
    } else {
        getStringUntilTerminators(",;}");
    }

    if (lex_Current.length > 0) {
        char ch = lex_PeekChar(0);
        if (ch == '\n' || ch == ';') {
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

char
lex_GetChar(void) {
    if (g_currentBuffer->charStack.count > 0) {
        return g_currentBuffer->charStack.stack[--(g_currentBuffer->charStack.count)];
    }
    if (g_currentBuffer->index < g_currentBuffer->bufferSize) {
        return g_currentBuffer->buffer[g_currentBuffer->index++];
    } else {
        return 0;
    }
}

size_t
lex_GetChars(char* dest, size_t length) {
    size_t copiedLines = 0;
    for (size_t i = 0; i < length; ++i) {
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
        unputChar(*(--str));
    }
}

void
lex_UnputString(const char* str) {
    lex_UnputStringLength(str, strlen(str));
}

void
lex_SetBuffer(SLexerBuffer* buffer) {
    if (buffer) {
        g_currentBuffer = buffer;
    } else {
        internalerror("Argument must not be NULL");
    }
}

void
lex_SetState(ELexerState state) {
    if (g_currentBuffer) {
        g_currentBuffer->state = state;
    } else {
        internalerror("g_pCurrentBuffer not initialized");
    }
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
    lexerBuffer->charStack.count = 0;
    lexerBuffer->index = 0;
    lexerBuffer->bufferSize = size;
    lexerBuffer->atLineStart = true;
    lexerBuffer->state = LEX_STATE_NORMAL;
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
    lex_VariadicInit();
}

bool
lex_GetNextToken(void) {
    switch (g_currentBuffer->state) {
        case LEX_STATE_NORMAL: {
            return stateNormal();
        }
        case LEX_STATE_MACRO_ARGUMENT0: {
            g_currentBuffer->state = LEX_STATE_MACRO_ARGUMENT;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
            if (stateMacroArgument0())
                return true;
#pragma GCC diagnostic pop

            // fall through
        }
        case LEX_STATE_MACRO_ARGUMENT: {
            return stateMacroArguments();
        }
    }

    internalerror("Abnormal error encountered");
    return 0;
}
