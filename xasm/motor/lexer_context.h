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

#ifndef XASM_MOTOR_LEXER_CONTEXT_H_INCLUDED_
#define XASM_MOTOR_LEXER_CONTEXT_H_INCLUDED_

#include "lists.h"
#include "str.h"

#include "lexer_buffer.h"

typedef enum {
    CONTEXT_FILE,
    CONTEXT_REPT,
    CONTEXT_MACRO
} EContextType;

typedef enum {
    LEXER_MODE_NORMAL,
    LEXER_MODE_MACRO_ARGUMENT0,
    LEXER_MODE_MACRO_ARGUMENT
} ELexerMode;

typedef struct FileInfo {
    string* fileName;
    uint32_t fileId;
    uint32_t crc32;
} SFileInfo;

typedef struct {
    uint32_t id;
    size_t length;
    union {
        char string[MAX_TOKEN_LENGTH + 1];
        int32_t integer;
        long double floating;
    } value;
} SLexerToken;

struct Symbol;

typedef struct LexerContext {
    list_Data(struct LexerContext);

	SLexerToken token;

	EContextType type;
    ELexerMode mode;

	SLexerBuffer buffer;
    bool atLineStart;

    SFileInfo* fileInfo;
    uint32_t lineNumber;

    union {
        struct {
			struct LexerContext* bookmark;
            uint32_t remaining;
        } repeat;
        struct {
            struct Symbol* symbol;
        } macro;
    } block;
} SLexerContext;

extern SFileInfo**
lexctx_GetFileInfo(size_t* totalFileNames);

extern size_t
lexctx_GetMacroArgumentCount(void);

extern void
lexctx_AddMacroArgument(string* str);

extern void
lexctx_SetMacroArgument0(string* str);

extern void
lexctx_ProcessMacro(string* macroName);

extern void
lexctx_ProcessIncludeFile(string* filename);

extern void
lexctx_ProcessRepeatBlock(uint32_t count);

extern bool
lexctx_EndCurrentBuffer(void);

extern bool
lexctx_EndReptBlock(void);

extern bool
lexctx_ContextInit(string* filename);

extern void
lexctx_Cleanup(void);

extern string*
lexctx_Dump(void);

extern void
lexctx_ShiftMacroArgs(int32_t count);

extern SFileInfo*
lexctx_TokenFileInfo(void);

extern uint32_t
lexctx_TokenLineNumber(void);

extern void
lexctx_Copy(SLexerContext* dest, const SLexerContext* source);

extern void
lexctx_ShallowCopy(SLexerContext* dest, const SLexerContext* source);

extern SLexerContext*
lexctx_CreateMemoryContext(string* name, string* content, vec_t* arguments);

extern SLexerContext*
lexctx_CreateFileContext(FILE* f, string* name);

void
lexctx_Destroy(SLexerContext* context);

extern void
lexctx_FreeContext(SLexerContext* buffer);

extern SLexerContext *
lex_Context;

#endif /* XASM_MOTOR_LEXER_CONTEXT_H_INCLUDED_ */
