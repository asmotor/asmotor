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

#ifndef XASM_COMMON_LEXER_CONTEXT_H_INCLUDED_
#define XASM_COMMON_LEXER_CONTEXT_H_INCLUDED_

#include "lists.h"
#include "str.h"

#include "lexer.h"

typedef enum {
    CONTEXT_FILE,
    CONTEXT_REPT,
    CONTEXT_MACRO
} EContextType;

typedef struct FileInfo {
    string* fileName;
    uint32_t fileId;
    uint32_t crc32;
} SFileInfo;

struct Symbol;

typedef struct LexerContext {
    list_Data(struct LexerContext);

    EContextType type;
    string* name;
    SFileInfo* fileInfo;
    SLexerBuffer* lexBuffer;
    uint32_t lineNumber;

    union {
        struct {
			SLexerBookmark bookmark;
            uint32_t remaining;
        } repeat;
        struct {
            struct Symbol* symbol;
        } macro;
    } block;
} SLexerContext;

extern SFileInfo**
lex_GetFileInfo(size_t* totalFileNames);

extern size_t
lex_GetMacroArgumentCount(void);

extern void
lex_AddMacroArgument(string* str);

extern void
lex_SetMacroArgument0(string* str);

extern void
lex_ProcessMacro(string* macroName);

extern void
lex_ProcessIncludeFile(string* filename);

extern void
lex_ProcessRepeatBlock(uint32_t count);

extern bool
lex_EndCurrentBuffer(void);

extern bool
lex_ContextInit(string* filename);

extern void
lex_Cleanup(void);

extern string*
lex_Dump(void);

extern void
lex_ShiftMacroArgs(int32_t count);

extern SFileInfo*
lex_CurrentFileInfo();

extern uint32_t
lex_CurrentLineNumber();

extern SLexerContext *
lex_Context;

#endif /* XASM_COMMON_LEXER_CONTEXT_H_INCLUDED_ */
