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

typedef struct LexBuffer {
	char* pBufferStart;
	char* pBuffer;
	size_t bufferSize;
	bool_t atLineStart;
	ELexerState State;
} SLexBuffer;

typedef struct {
	union {
		char aString[MAXTOKENLENGTH + 1];
		int32_t nInteger;
	} Value;
	size_t TokenLength;
	union {
		EToken Token;
		uint32_t TargetToken;
	} ID;
} SLexToken;

typedef struct {
	SLexBuffer Buffer;
	SLexToken Token;
} SLexBookmark;

extern SLexToken g_CurrentToken;

extern void lex_FreeBuffer(SLexBuffer* buf);
extern SLexBuffer* lex_CreateFileBuffer(FILE* f);
extern void lex_SetBuffer(SLexBuffer* buf);
extern uint32_t lex_GetNextToken(void);
extern void lex_AddString(const char* pszName, uint32_t nToken);
extern void lex_AddStrings(SLexInitString* lex);
extern void lex_RemoveString(const char* pszName, uint32_t nToken);
extern void lex_RemoveStrings(SLexInitString* lex);
extern void lex_SetState(ELexerState i);
extern void lex_UnputString(const char* s);
extern void lex_UnputChar(char c);
extern void lex_SkipBytes(size_t count);
extern void lex_Init(void);
extern SLexBuffer* lex_CreateMemoryBuffer(const char* mem, size_t size);
extern void lex_RewindBytes(size_t count);

extern void lex_Bookmark(SLexBookmark* pBookmark);
extern void lex_Goto(SLexBookmark* pBookmark);

#endif    /*INCLUDE_LEXER_H*/
