/*  Copyright 2008-2022 Carsten Elton Sorensen and contributors

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
#include "lexer_buffer.h"
#include "lexer_context.h"
#include "tokens.h"

extern bool
lex_Init(string* filename);

extern void
lex_Exit(void);

extern void
lex_UnputChar(char ch);

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
lex_Bookmark(SLexerContext* bookmark);

extern void
lex_Goto(SLexerContext* bookmark);

extern string*
lex_TokenString(void);

#endif /* XASM_COMMON_LEXER_H_INCLUDED_ */
