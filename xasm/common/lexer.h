/*  Copyright 2008 Carsten Sørensen

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

#ifndef	INCLUDE_LEXER_H
#define	INCLUDE_LEXER_H

#include <stdio.h>

#include "types.h"
#include "tokens.h"
#include "xasm.h"

typedef	struct
{
	char*	pszName;
	ULONG	nToken;
} SLexInitString;

typedef	struct
{
	BOOL 	(*pCallback)(char* s, ULONG size);
	ULONG	nToken;
} SLexFloat;

typedef	enum
{
	LEX_STATE_NORMAL,
	LEX_STATE_MACROARG0,
	LEX_STATE_MACROARGS
} ELexerState;

typedef	struct LexBuffer
{
	char*	pBufferStart;
	char*	pBuffer;
	size_t	nBufferSize;
	BOOL	bAtLineStart;
	ELexerState	State;
} SLexBuffer;

typedef	struct
{
	union
	{
		char	aString[MAXTOKENLENGTH+1];
		SLONG	nInteger;
	} Value;
	SLONG	TokenLength;
	union
	{
		eToken	Token;
		int		TargetToken;
	}	ID;
} SLexToken;

typedef struct
{
	SLexBuffer	Buffer;
	SLexToken	Token;
} SLexBookmark;

extern	SLexToken	g_CurrentToken;

extern void	lex_FreeBuffer(SLexBuffer* buf);
extern SLexBuffer* lex_CreateFileBuffer(FILE* f);
extern void lex_SetBuffer(SLexBuffer* buf);
extern ULONG lex_GetNextToken(void);
extern void lex_AddString(char* pszName, int nToken);
extern void lex_AddStrings(SLexInitString* lex);
extern void lex_RemoveString(char* pszName, int nToken);
extern void lex_RemoveStrings(SLexInitString* lex);
extern void	lex_SetState(ELexerState i);
extern ULONG lex_FloatAlloc(SLexFloat* tok);
extern void	lex_FloatAddRange(ULONG id, UWORD start, UWORD end, SLONG charnumber);
extern void	lex_FloatAddRangeAndBeyond(ULONG id, UWORD start, UWORD end, SLONG charnumber);
extern void	lex_FloatRemoveAll(ULONG id);
extern void	lex_FloatSetSuffix(ULONG id, UBYTE ch);
extern void lex_UnputString(char* s);
extern void	lex_UnputChar(char c);
extern void	lex_SkipBytes(ULONG count);
extern void	lex_Init(void);
extern SLexBuffer* lex_CreateMemoryBuffer(char* mem, size_t size);
extern void	lex_RewindBytes(ULONG count);
extern void lex_Bookmark(SLexBookmark* pBookmark);
extern void	lex_Goto(SLexBookmark* pBookmark);

#endif	/*INCLUDE_LEXER_H*/