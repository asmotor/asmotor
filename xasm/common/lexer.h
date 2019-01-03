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

#ifndef XASM_COMMON_LEXER_H_INCLUDED_
#define XASM_COMMON_LEXER_H_INCLUDED_

#include <stdio.h>
#include <str.h>
#include "types.h"
#include "tokens.h"
#include "xasm.h"
#include "lexervariadics.h"

typedef struct {
    const char* name;
    uint32_t token;
} SLexerTokenDefinition;

typedef enum {
    LEX_STATE_NORMAL,
    LEX_STATE_MACRO_ARGUMENT0,
    LEX_STATE_MACRO_ARGUMENT
} ELexerState;

typedef struct CharStack {
    char stack[MAXSTRINGSYMBOLSIZE];
    size_t count;
} SCharStack;

typedef struct LexerBuffer {
    SCharStack charStack;
    char* buffer;
    size_t index;
    size_t bufferSize;
    bool atLineStart;
    ELexerState state;
} SLexerBuffer;

typedef struct {
    uint32_t token;
    size_t length;
    union {
        char string[MAXTOKENLENGTH + 1];
        int32_t integer;
    } value;
} SLexerToken;

typedef struct {
    SLexerBuffer Buffer;
    SLexerToken Token;
} SLexerBookmark;

extern SLexerToken lex_Current;

extern void
lex_Init(void);

extern void
lex_DefineToken(const char* name, uint32_t token);

extern void
lex_DefineTokens(const SLexerTokenDefinition* lex);

extern void
lex_UndefineToken(const char* name, uint32_t token);

extern void
lex_UndefineTokens(SLexerTokenDefinition* lex);

extern SLexerBuffer*
lex_CreateMemoryBuffer(const char* memory, size_t size);

extern SLexerBuffer*
lex_CreateFileBuffer(FILE* f);

extern void
lex_FreeBuffer(SLexerBuffer* buffer);

extern void
lex_SetBuffer(SLexerBuffer* buffer);

extern char
lex_PeekChar(size_t index);

extern char
lex_GetChar(void);

extern size_t
lex_GetChars(char* dest, size_t length);

INLINE string* lex_GetString(size_t length) {
    return str_CreateStream(lex_GetChar, length);
}

extern bool
lex_MatchChar(char ch);

extern bool
lex_CompareNoCase(size_t index, const char* str, size_t length);

extern bool
lex_StartsWithNoCase(const char* str, size_t length);

extern bool
lex_StartsWithStringNoCase(const string* str);

extern size_t
lex_SkipBytes(size_t count);

extern void
lex_UnputString(const char* str);

extern bool
lex_GetNextToken(void);

extern void
lex_SetState(ELexerState state);

extern void
lex_Bookmark(SLexerBookmark* bookmark);

extern void
lex_Goto(SLexerBookmark* bookmark);

#endif /* XASM_COMMON_LEXER_H_INCLUDED_ */
