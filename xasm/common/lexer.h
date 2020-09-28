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

#include "xasm.h"
#include "str.h"
#include "types.h"

#include "charstack.h"
#include "filebuffer.h"
#include "tokens.h"

typedef enum {
    LEXER_MODE_NORMAL,
    LEXER_MODE_MACRO_ARGUMENT0,
    LEXER_MODE_MACRO_ARGUMENT
} ELexerMode;

typedef struct LexerBuffer {
	SFileBuffer fileBuffer;
    bool atLineStart;
    ELexerMode mode;
} SLexerBuffer;

typedef struct {
    uint32_t token;
    size_t length;
    union {
        char string[MAX_TOKEN_LENGTH + 1];
        int32_t integer;
        long double floating;
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
lex_UnputChar(char ch);

extern SLexerBuffer*
lex_CreateBookmarkBuffer(SLexerBookmark* bookmark);

extern SLexerBuffer*
lex_CreateMemoryBuffer(string* content, vec_t* arguments);

extern SLexerBuffer*
lex_CreateFileBuffer(FILE* f, uint32_t* checkSum);

extern void
lex_FreeBuffer(SLexerBuffer* buffer);

extern void
lex_SetBuffer(SLexerBuffer* buffer);

extern char
lex_GetChar(void);

extern void
lex_CopyUnexpandedContent(char* dest, size_t count);

extern size_t
lex_SkipBytes(size_t count);

extern void
lex_UnputString(const char* str);

extern void
lex_UnputStringLength(const char* str, size_t length);

extern bool
lex_GetNextDirective(void);

extern bool
lex_GetNextDirectiveUnexpanded(size_t* index);

extern bool
lex_GetNextToken(void);

extern void
lex_SetMode(ELexerMode mode);

extern void
lex_Bookmark(SLexerBookmark* bookmark);

extern void
lex_Goto(SLexerBookmark* bookmark);

extern string*
lex_TokenString(void);

extern void
lex_CopyBuffer(SLexerBuffer* dest, const SLexerBuffer* source);

#endif /* XASM_COMMON_LEXER_H_INCLUDED_ */
