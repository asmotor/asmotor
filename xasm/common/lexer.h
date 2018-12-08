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

#ifndef INCLUDE_LEXER_H
#define INCLUDE_LEXER_H

#include <stdio.h>
#include <str.h>
#include "types.h"
#include "tokens.h"
#include "xasm.h"
#include "lexervariadics.h"

typedef struct {
	char* name;
	uint32_t token;
} SLexInitString;

typedef enum {
	LEX_STATE_NORMAL, LEX_STATE_MACRO_ARG0, LEX_STATE_MACRO_ARGS
} ELexerState;

typedef struct CharStack {
	char stack[MAXSTRINGSYMBOLSIZE];
	size_t count;
} SCharStack;

typedef struct LexBuffer {
	SCharStack charStack;
	char* buffer;
	size_t index;
	size_t bufferSize;
	bool atLineStart;
	ELexerState State;
} SLexBuffer;

typedef struct {
	uint32_t Token;
	size_t TokenLength;
	union {
		char aString[MAXTOKENLENGTH + 1];
		int32_t nInteger;
	} Value;
} SLexToken;

typedef struct {
	SLexBuffer Buffer;
	SLexToken Token;
} SLexBookmark;

extern SLexToken g_CurrentToken;

extern void lex_Init(void);

extern void lex_AddString(const char* pszName, uint32_t nToken);
extern void lex_AddStrings(SLexInitString* lex);
extern void lex_RemoveString(const char* pszName, uint32_t nToken);
extern void lex_RemoveStrings(SLexInitString* lex);

extern SLexBuffer* lex_CreateMemoryBuffer(const char* mem, size_t size);
extern SLexBuffer* lex_CreateFileBuffer(FILE* f);
extern void lex_FreeBuffer(SLexBuffer* buf);
extern void lex_SetBuffer(SLexBuffer* buf);

extern char lex_PeekChar(size_t index);
extern char lex_GetChar(void);
extern size_t lex_GetChars(char* dest, size_t length);
extern bool lex_MatchChar(char ch);
extern bool lex_CompareNoCase(size_t index, const char* str, size_t length);
extern bool lex_StartsWithNoCase(const char* str, size_t length);
extern bool lex_StartsWithStringNoCase(const string* str);

extern size_t lex_SkipBytes(size_t count);
extern void lex_RewindBytes(size_t count);

extern void lex_UnputString(const char* s);

extern uint32_t lex_GetNextToken(void);

extern void lex_SetState(ELexerState i);

extern void lex_Bookmark(SLexBookmark* pBookmark);
extern void lex_Goto(SLexBookmark* pBookmark);

#endif    /*INCLUDE_LEXER_H*/
